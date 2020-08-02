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

#define MAX_INPUT_BUFFER 50

static int dev_open_count;
static int dev_output_done;
static unsigned interrupt_counter;
static char last_reset_date[50];

static dev_t first;
static struct class *cl;
static struct cdev char_dev;

static void reset_counter(void);
static ssize_t device_read(struct file*, char*, size_t, loff_t*);
static ssize_t device_write(struct file*, const char*, size_t, loff_t*);
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
    int i;
    char *temp;
    char ch;

    printk(KERN_INFO "ioctl action\n");
    switch (ioctl_num)
    {
    case _IOW(240, 0, char *):
        temp = (char *) ioctl_param;

        get_user(ch, temp);
        for (i=0; ch && i<30; i++, temp++)
            get_user(ch, temp);

        device_write(file, (char *) ioctl_param, i, 0);
        break;

    case _IOR(240, 1, char *):
        i = device_read(file, (char *) ioctl_param, 99, 0);
        put_user('\0', (char *) ioctl_param + i);
        break;

//    case QUERY_GET_RESET_COUNT:
//        break;
//    case QUERY_GET_RESET_DATE:
//        break;
//    case QUERY_RESET_COUNTER:
//        break;
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

    sprintf(last_reset_date, "Last reset date (UTC): %d/%d/%ld %02d:%02d:%02d\n",
            tm_val.tm_mday, tm_val.tm_mon + 1, 1900 + tm_val.tm_year,
            tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec);
}

static ssize_t device_read(struct file *f, char *buffer, size_t length, loff_t* l)
{
    if (dev_output_done)
        return 0;

    int bytes_read = 0;
    char message[100];

    strcpy(message, last_reset_date);
    sprintf(message + strlen(last_reset_date), "Interrupts: %u\n", interrupt_counter);

    char *message_ptr = message;
    while (length && *message_ptr)
    {
        put_user(*(message_ptr++), buffer++);
        --length;
        ++bytes_read;
    }

    dev_output_done = 1;
    return bytes_read;
}

static ssize_t device_write(struct file *f, const char *buffer, size_t length, loff_t* l)
{
    char request[MAX_INPUT_BUFFER];
    int i;
    for (i = 0; i < length && i < MAX_INPUT_BUFFER; ++i)
        get_user(request[i], buffer + i);
    request[MAX_INPUT_BUFFER - 1] = '\0';

    int is_reset = strncmp("reset", request, 5);
    if (is_reset == 0)
    {
        reset_counter();
        printk(KERN_INFO "Counter reseted\n");
    }
    else
        printk(KERN_ALERT "Unknown request: %s", request);

    return i;
}

static int device_open(struct inode *i, struct file *f)
{
    if (dev_open_count)
        return -EBUSY;

    ++dev_open_count;
    dev_output_done = 0;
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
