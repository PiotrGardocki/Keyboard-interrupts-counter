#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/cdev.h>

#include "module-interface.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Piotr Gardocki");
MODULE_DESCRIPTION("A simple module to count keyboard interrupts");

static int dev_open_count;
static unsigned interrupt_counter;
static char last_reset_date[MAX_IO_BUFFER];

static dev_t first;
static struct class *cl;
static struct cdev char_dev;

static void reset_counter(void);
static void write_reset_count(char*);
static void write_reset_date(char*);
static void copy_buffer_to_user(const char*, char*);

static int device_open(struct inode*, struct file*);
static int device_release(struct inode*, struct file*);

static irq_handler_t keyboard_handler(int, void*, struct pt_regs*);

long device_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);

static struct file_operations fops =
{
    .open = device_open,
    .release = device_release,
    .unlocked_ioctl = device_ioctl
};

static int __init init(void)
{
    interrupt_counter = 0;
    dev_open_count = 0;

    if (alloc_chrdev_region(&first, 0, 1, DEVICE_NAME) < 0)
    {
        return -1;
    }
    printk(KERN_INFO "<Major, Minor>: <%d, %d>\n", MAJOR(first), MINOR(first));
    if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL)
    {
        printk(KERN_INFO "class_create() failed");
        unregister_chrdev_region(first, 1);
        return -1;
    }
    if (device_create(cl, NULL, first, NULL, DEVICE_NAME) == NULL)
    {
        printk(KERN_INFO "device_create() failed");
        class_destroy(cl);
        unregister_chrdev_region(first, 1);
        return -1;
    }

    cdev_init(&char_dev, &fops);
    if (cdev_add(&char_dev, first, 1) == -1)
    {
        device_destroy(cl, first);
        class_destroy(cl);
        unregister_chrdev_region(first, 1);
        return -1;
    }

    int irq = request_irq(1, (irq_handler_t)keyboard_handler, IRQF_SHARED, "keyboard-irq-handler", (void*)keyboard_handler);
    if (irq != 0)
    {
        printk(KERN_ALERT "Failure at requesting interrupt handler\n");
        return irq;
    }

    reset_counter();
    printk(KERN_INFO "Module key-handler successfully loaded\n");
    return 0;
}

static void __exit cleanup(void)
{
    free_irq(1, (void*)keyboard_handler);

    device_destroy(cl, first);
    class_destroy(cl);
    unregister_chrdev_region(first, 1);

    printk(KERN_INFO "Module key-handler successfully unloaded\n");
}

module_init(init);
module_exit(cleanup);

long device_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
    printk(KERN_INFO "ioctl action\n");
    switch (ioctl_num)
    {
    case QUERY_GET_RESET_COUNT:
        write_reset_count((char*)ioctl_param);
        break;
    case QUERY_GET_RESET_DATE:
        write_reset_date((char*)ioctl_param);
        break;
    case QUERY_RESET_COUNTER:
        reset_counter();
        break;
    }

    return 0;
}

static void reset_counter(void)
{
    interrupt_counter = 0;

    struct timespec64 now;
    struct tm tm_val;
    ktime_get_real_ts64(&now);
    time64_to_tm(now.tv_sec, 0, &tm_val);

    sprintf(last_reset_date, "%d/%d/%ld %02d:%02d:%02d",
            tm_val.tm_mday, tm_val.tm_mon + 1, 1900 + tm_val.tm_year,
            tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec);
}

static void write_reset_count(char *buffer)
{
    char temp_buffer[MAX_IO_BUFFER];
    sprintf(temp_buffer, "%u", interrupt_counter);
    copy_buffer_to_user(temp_buffer, buffer);
}

static void write_reset_date(char *buffer)
{
    copy_buffer_to_user(last_reset_date, buffer);
}

static void copy_buffer_to_user(const char *buffer, char *user_buffer)
{
    while (*buffer)
        put_user(*(buffer++), user_buffer++);
    put_user('\0', user_buffer);
}

static int device_open(struct inode *i, struct file *f)
{
    if (dev_open_count)
        return -EBUSY;

    ++dev_open_count;
    try_module_get(THIS_MODULE);
    return 0;
}

static int device_release(struct inode *i, struct file *f)
{
    --dev_open_count;
    module_put(THIS_MODULE);
    return 0;
}

static irq_handler_t keyboard_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    ++interrupt_counter;
    return (irq_handler_t)IRQ_HANDLED;
}
