#ifndef _LINUX_PHONE_BOOK_H
#define _LINUX_PHONE_BOOK_H

typedef struct phone_book_user_data {
	int age;
	char __user *surname;
	char __user *name;
	char __user *email;
	char __user *phone;
} phone_book_user_data_t;

typedef struct phone_book_internal_user_data {
	int age;
	char *surname;
	char *name;
	char *email;
	char *phone;
} phone_book_internal_user_data_t;

typedef long (*phone_book_get_user_t)(const char *,
				      phone_book_internal_user_data_t *);
typedef long (*phone_book_add_user_t)(const phone_book_internal_user_data_t *);
typedef long (*phone_book_del_user_t)(const char *);

#endif // _LINUX_PHONE_BOOK_H
