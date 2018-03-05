#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

static const char* PROC_NAME = "fastboot";

static int start_fastboot(void)
{
  printk(KERN_INFO "start fastboot");

  return 0;
}

static ssize_t proc_write_callback(struct file *file, const char *buffer, size_t count, loff_t *data)
{
  // cmd should be 0 or 1.
  char cmd[2];

  if (count != (sizeof(char) + 1))
    return -EFAULT;

  if (copy_from_user(&cmd, buffer, 2)) {
    return -EFAULT;
  }

  printk(KERN_INFO "fastboot read %s", cmd);

  if (strcmp(cmd, "1\0")) {
    start_fastboot();
  }

  return count;
}

static const struct file_operations proc_file_fops = {
  .owner = THIS_MODULE,
  .write = proc_write_callback
};

static int fastboot_module_init(void)
{
  struct proc_dir_entry *proc_file_entry = proc_create(PROC_NAME, 0, NULL, &proc_file_fops);
  if (proc_file_entry == NULL)
    return -ENOMEM;

  return 0;
}

static void fastboot_module_exit(void)
{

}

module_init(fastboot_module_init);
module_exit(fastboot_module_exit);

MODULE_AUTHOR("wangyaohua@deepin.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Mimic the Windows fastboot feature in linux kernel.");