#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/sched/signal.h>
#include <linux/sched/task.h>
#include <linux/fs.h>
#include <linux/reboot.h>
#include <linux/suspend.h>
#include <linux/signal.h>

static const char* PROC_NAME = "fastboot";

#ifdef CONFIG_PM_AUTOSLEEP

/* kernel/power/autosleep.c */
extern int pm_autosleep_init(void);
extern int pm_autosleep_lock(void);
extern void pm_autosleep_unlock(void);
extern suspend_state_t pm_autosleep_state(void);
extern int pm_autosleep_set_state(suspend_state_t state);

#else /* !CONFIG_PM_AUTOSLEEP */

static inline int pm_autosleep_init(void) { return 0; }
static inline int pm_autosleep_lock(void) { return 0; }
static inline void pm_autosleep_unlock(void) {}
static inline suspend_state_t pm_autosleep_state(void) { return PM_SUSPEND_ON; }

#endif /* !CONFIG_PM_AUTOSLEEP */


static void send_sig_all(int sig)
{
  struct task_struct *p;

  read_lock(&tasklist_lock);
  for_each_process(p) {
    if (p->flags & PF_KTHREAD)
            continue;
    if (is_global_init(p))
            continue;

    do_send_sig_info(sig, SEND_SIG_FORCED, p, true);
  }
  read_unlock(&tasklist_lock);
}

static int suspend_to_disk(void)
{
  int error;

  error = pm_autosleep_lock();
  if (error)
          goto out;

  if (pm_autosleep_state() > PM_SUSPEND_ON) {
          error = -EBUSY;
          goto out;
  }

  error = hibernate();

out:
  pm_autosleep_unlock();
  return error;
}

static int init_fastboot(void)
{
  int error;

  printk(KERN_INFO "start fastboot");

  send_sig_all(SIGTERM);
  send_sig_all(SIGKILL);
  emergency_sync();

  error = suspend_to_disk();
  if (error != 0) {
    printk(KERN_INFO "failed to initialize fastboot");
  }

  lockdep_off();
  local_irq_enable();
  emergency_restart();

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
    init_fastboot();
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