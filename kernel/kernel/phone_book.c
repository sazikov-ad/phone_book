#include <linux/export.h>
#include <linux/phone_book.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

phone_book_get_user_t phone_book_get_user_impl = NULL;
phone_book_add_user_t phone_book_add_user_impl = NULL;
phone_book_del_user_t phone_book_del_user_impl = NULL;

EXPORT_SYMBOL(phone_book_get_user_impl);
EXPORT_SYMBOL(phone_book_add_user_impl);
EXPORT_SYMBOL(phone_book_del_user_impl);

#define MODULE_NOT_LOADED_MESSAGE(syscall)                                     \
	printk(KERN_INFO "=== phone_book_" #syscall                            \
			  " syscall: phone_book module is not loaded\n")
#define CHECK_MODULE_LOAD(syscall)                                             \
	if (phone_book_##syscall##_impl == NULL) {                             \
		MODULE_NOT_LOADED_MESSAGE(syscall);                            \
		return -EFAULT;                                                \
	}

#define ALLOC(variable, type, count)                                           \
	variable = (type *)kmalloc(sizeof(type) * count, GFP_KERNEL)

#define ALLOC_AND_VERIFY(variable, type, count)                                \
	ALLOC(variable, type, count);                                          \
                                                                               \
	if (variable == NULL) {                                                \
		printk(KERN_ERR "===<< could not allocate memory\n");	       \
		return -EFAULT;                                                \
	}

#define ALLOC_AND_VERIFY_NEW(variable, type, count)                            \
	ALLOC(type *variable, type, count);                                    \
                                                                               \
	if (variable == NULL) {                                                \
		printk(KERN_ERR "===<< could not allocate memory\n");	       \
		return -EFAULT;                                                \
	}

#define PASS_VA_ARGS(x) x
#define MACRO_IMPL_DISPATCHER4(_0, _1, _2, _3, IMPL, ...) IMPL

#define DEALLOC_IMPL1(p) kfree(p)
#define DEALLOC_IMPL2(p1, p2)                                                  \
	kfree(p1);                                                             \
	kfree(p2)
#define DEALLOC_IMPL3(p1, p2, p3)                                              \
	kfree(p1);                                                             \
	kfree(p2);                                                             \
	kfree(p3)
#define DEALLOC_IMPL4(p1, p2, p3, p4)                                          \
	kfree(p1);                                                             \
	kfree(p2);                                                             \
	kfree(p3);                                                             \
	kfree(p4)

#define DEALLOC(...)                                                           \
	PASS_VA_ARGS(MACRO_IMPL_DISPATCHER4(__VA_ARGS__, DEALLOC_IMPL4,        \
					    DEALLOC_IMPL3, DEALLOC_IMPL2,      \
					    DEALLOC_IMPL1)(__VA_ARGS__))

SYSCALL_DEFINE2(phone_book_get_user, const char __user *, surname,
		phone_book_user_data_t __user *, output)
{
	CHECK_MODULE_LOAD(get_user);

	size_t surname_length = strnlen_user(surname, 1000);
	ALLOC_AND_VERIFY_NEW(surname_buf, char, surname_length)

	if (copy_from_user(surname_buf, surname, surname_length)) {
		DEALLOC(surname_buf);
		return -EFAULT;
	}

	phone_book_internal_user_data_t user_data;
	long result = phone_book_get_user_impl(surname_buf, &user_data);

	DEALLOC(surname_buf);

	if (result == 0) {
		phone_book_user_data_t user_data_output;

		if (copy_from_user(&user_data_output, output,
				   sizeof(phone_book_user_data_t))) {
			DEALLOC(user_data.surname, user_data.name,
				user_data.email, user_data.phone);
			return -EFAULT;
		}

		if (copy_to_user(user_data_output.surname, user_data.surname,
				 strlen(user_data.surname)) ||
		    copy_to_user(user_data_output.name, user_data.name,
				 strlen(user_data.name)) ||
		    copy_to_user(user_data_output.email, user_data.email,
				 strlen(user_data.email)) ||
		    copy_to_user(user_data_output.phone, user_data.phone,
				 strlen(user_data.phone))) {
			DEALLOC(user_data.surname, user_data.name,
				user_data.email, user_data.phone);
			return -EFAULT;
		}

		if (copy_to_user(output, &user_data_output,
				 sizeof(phone_book_user_data_t))) {
			DEALLOC(user_data.surname, user_data.name,
				user_data.email, user_data.phone);
			return -EFAULT;
		}

		DEALLOC(user_data.surname, user_data.name, user_data.email,
			user_data.phone);
	}

	return result;
}

SYSCALL_DEFINE1(phone_book_add_user, const phone_book_user_data_t __user *,
		user)
{

	CHECK_MODULE_LOAD(add_user);

	phone_book_user_data_t user_buffer;

	if (copy_from_user(&user_buffer, user,
			   sizeof(phone_book_user_data_t))) {
		return -EFAULT;
	}

	size_t surname_len = strnlen_user(user_buffer.surname, 1000);
	size_t name_len = strnlen_user(user_buffer.name, 1000);
	size_t email_len = strnlen_user(user_buffer.email, 1000);
	size_t phone_len = strnlen_user(user_buffer.phone, 1000);

	phone_book_internal_user_data_t user_data;

	ALLOC_AND_VERIFY(user_data.surname, char, surname_len);
	ALLOC_AND_VERIFY(user_data.name, char, name_len);
	ALLOC_AND_VERIFY(user_data.email, char, email_len);
	ALLOC_AND_VERIFY(user_data.phone, char, phone_len);

	if (copy_from_user(user_data.surname, user_buffer.surname,
			   surname_len) ||
	    copy_from_user(user_data.name, user_buffer.name, name_len) ||
	    copy_from_user(user_data.phone, user_buffer.phone, phone_len) ||
	    copy_from_user(user_data.email, user_buffer.email, email_len)) {
		DEALLOC(user_data.surname, user_data.name, user_data.email,
			user_data.phone);
		return -EFAULT;
	}

	long result = phone_book_add_user_impl(&user_data);
	
	DEALLOC(user_data.surname, user_data.name, user_data.email,
		user_data.phone);

	return result;
}

SYSCALL_DEFINE1(phone_book_del_user, const char __user *, surname)
{
	CHECK_MODULE_LOAD(del_user);

	size_t surname_len = strnlen_user(surname, 1000);

	ALLOC_AND_VERIFY_NEW(surname_buf, char, surname_len)

	long result = phone_book_del_user_impl(surname_buf);

	DEALLOC(surname_buf);

	return result;
}

