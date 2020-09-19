#include "book.h"
#include "device.h"

#include <linux/miscdevice.h>

typedef enum command { GET_USER, ADD_USER, DEL_USER, NONE } command_t;

#define ERROR_R "1"
#define OK_R "0"

static command_t last_command = NONE;
static char *last_command_output = NULL;

static size_t int_to_string(char *buf, int value)
{
	while (value > 999) {
		value /= 10;
	}

	if (value >= 100) {
		buf[0] = '0' + (value / 100) % 10;
		buf[1] = '0' + (value / 10) % 10;
		buf[2] = '0' + value % 10;
		return 3;
	} else if (value > 10) {
		buf[0] = '0' + (value / 10) % 10;
		buf[1] = '0' + value % 10;
		return 2;
	} else {
		buf[0] = '0' + value % 10;
		return 1;
	}

	return 0;
}

static char *clone_string(const char *string_literal)
{
	size_t len = strlen(string_literal);
	char *res = (char *)kmalloc(sizeof(char) * (len + 1), GFP_KERNEL);
	memcpy(res, string_literal, len * sizeof(char));
	res[len] = '\0';
	return res;
}

static void reset_last_command(void)
{
	last_command = NONE;

	if (last_command_output != NULL) {
		kfree(last_command_output);
		last_command_output = NULL;
	}
}

static void set_last_command(command_t cmd, char *output)
{
	reset_last_command();

	last_command = cmd;
	last_command_output = clone_string(output);
}

static ssize_t device_read(struct file *file, char *__user buffer, size_t count,
			   loff_t *offset)
{
	if (last_command == NONE) {
		return -EINVAL;
	}

	printk(KERN_INFO "=== read: %ld\n", count);
	size_t len = strlen(last_command_output);

	if (count < len) {
		return -EINVAL;
	}

	if (*offset != 0) {
		printk(KERN_INFO "=== read return: 0\n");
		return 0;
	}

	if (copy_to_user(buffer, last_command_output, len)) {
		return -EINVAL;
	}

	*offset = len;

	printk(KERN_INFO "=== read return: %ld\n", len);
	return len;
}

static ssize_t device_write(struct file *file, const char *__user buffer,
			    size_t count, loff_t *offset)
{
	printk(KERN_INFO "=== write: %ld\n", count);
	printk(KERN_INFO "=== write offset: %lld\n", *offset);

	if (*offset != 0) {
		printk(KERN_INFO "=== write return: 0\n");
		return 0;
	}

	// expecting format: <command> <arg1> <arg2> ...

	if (count < 4) {
		return -EINVAL;
	}

	char command[4];
	command[3] = '\0';
	if (copy_from_user(&command, buffer, 3)) {
		return -EINVAL;
	}

	printk(KERN_INFO "=== write command: %s\n", command);

	size_t read_left = count - 5;
	char *buff =
		(char *)kmalloc(sizeof(char) * (read_left + 1), GFP_KERNEL);

	if (buff == NULL) {
		return -EINVAL;
	}

	buff[read_left] = '\0';
	if (copy_from_user(buff, buffer + 4, read_left)) {
		kfree(buff);
		return -EINVAL;
	}

	printk(KERN_INFO "=== write got: %s\n", buff);

	if (strcmp(command, "add") == 0) {
		printk(KERN_INFO "=== add command\n");

		char *tokens[5];
		int cnt = 0;

		char *tok;
		const char *delim = " ";
		while (tok = strsep(&buff, delim)) {
			printk(KERN_INFO "=== add command token: %s\n", tok);
			if (cnt < 5) {
				tokens[cnt] = tok;
			} else {
				break;
			}
			++cnt;
		}

		if (cnt < 5) {
			kfree(buff);
			return -EINVAL;
		}

		user_data_t user;

		user.surname = tokens[0];
		user.name = tokens[1];

		if (kstrtol(tokens[2], 10, (long *)&user.age) != 0) {
			kfree(buff);
			return -EINVAL;
		}

		user.email = tokens[3];
		user.phone = tokens[4];

		if (book_add_user(&user) == 0) {
			set_last_command(ADD_USER, OK_R);
		} else {
			set_last_command(ADD_USER, ERROR_R);
		}
	} else if (strcmp(command, "get") == 0) {
		printk(KERN_INFO "=== get command\n");

		user_data_t *user = NULL;

		if (book_get_user(buff, &user) == 0) {
			char age[3];

			size_t s_l = strlen(user->surname);
			size_t n_l = strlen(user->name);
			size_t a_l = int_to_string((char *)&age, user->age);
			size_t e_l = strlen(user->email);
			size_t p_l = strlen(user->phone);

			size_t total = s_l + n_l + a_l + e_l + p_l + 5;

			char *out = (char *)kmalloc(sizeof(char) * total,
						    GFP_KERNEL);

			if (out == NULL) {
				kfree(buff);
				return -EINVAL;
			}

			memset(out, ' ', total);
			out[total - 1] = '\0';
			memcpy(out, user->surname, s_l);
			memcpy(out + sizeof(char) * (s_l + 1), user->name, n_l);
			memcpy(out + sizeof(char) * (s_l + n_l + 2), &age, a_l);
			memcpy(out + sizeof(char) * (s_l + n_l + a_l + 3),
			       user->email, e_l);
			memcpy(out + sizeof(char) * (s_l + n_l + a_l + e_l + 4),
			       user->phone, p_l);

			set_last_command(GET_USER, out);

			kfree(out);
			kfree(user->surname);
			kfree(user->name);
			kfree(user->email);
			kfree(user->phone);
			kfree(user);
		} else {
			set_last_command(GET_USER, ERROR_R);
		}
	} else if (strcmp(command, "del") == 0) {
		printk(KERN_INFO "=== del command\n");
		if (book_del_user(buff) == 0) {
			set_last_command(DEL_USER, OK_R);
		} else {
			set_last_command(DEL_USER, ERROR_R);
		}
	} else {
		kfree(buff);
		return -EINVAL;
	}

	*offset = count;

	printk(KERN_INFO "=== write return: %ld\n", count);
	kfree(buff);

	return count;
}

static int minor = 0;
module_param(minor, int, S_IRUGO);

static const struct file_operations misc_fops = { .owner = THIS_MODULE,
						  .read = device_read,
						  .write = device_write };

static struct miscdevice misc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "phone_book",
	.fops = &misc_fops,
	.mode = 0666,
};

static int __init dev_init(void)
{
	printk(KERN_INFO "=== Hello\n");
	int ret;

	if (minor != 0) {
		misc_dev.minor = minor;
	}

	ret = misc_register(&misc_dev);

	if (ret) {
		printk(KERN_ERR "=== Unable to register misc device\n");
	} else {
		book_init();
		printk(KERN_INFO "====== Misc device registered ======\n");
	}

	return ret;
}

static void __exit dev_exit(void)
{
	book_exit();
	misc_deregister(&misc_dev);
	printk(KERN_INFO "====== Misc device unregistered ======\n");
}
