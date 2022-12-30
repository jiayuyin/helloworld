#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

static struct proc_dir_entry *proc_dir_test = NULL;
static struct proc_dir_entry *proc_file_test = NULL;

static ssize_t test_write_proc(struct file *file, const char __user *ubuf, size_t count, loff_t *pos)
{
	char temp[4] = {0};
	int test = 0;

	if (count > 2)
		return -1;

	if (copy_from_user(temp, ubuf, count))
		return -1;

	temp[count] = '\0';

	sscanf(temp, "%d", &test);

	printk("hello world!\n");

	return count;
}

static const struct file_operations proc_test_fops = {
	.owner = THIS_MODULE,
	.open = NULL,
	.read = NULL,
	.write = test_write_proc,
	.llseek = NULL,
	.release = NULL,
};


static int __init proc_test_init(void)
{
	proc_dir_test = proc_mkdir("proc_dir", NULL);
	if(proc_dir_test == NULL)
	{
		printk("create proc dir failed!\n");
		return -1;
	}
	proc_file_test = proc_create("proc_file_test", S_IRUSR, proc_dir_test, &proc_test_fops);
	if(proc_file_test == NULL)
	{
		printk("create proc file failed!\n");
		return -1;
	}
	return 0;
}

static int __exit proc_test_exit(void)
{
	remove_proc_entry("proc_file_test", proc_dir_test);
	remove_proc_entry("proc_dir", NULL);
	printk("rm proc_dir/*\n");
	return 0;
}

module_init(proc_test_init);
module_exit(proc_test_exit);

MODULE_AUTHOR("jiayu");
MODULE_DESCRIPTION("proc test");
MODULE_LICENSE("GPL");