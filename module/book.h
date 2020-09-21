#ifndef __BOOK_H
#define __BOOK_H

typedef struct user_data {
	int age;
	char *surname;
	char *name;
	char *phone;
	char *email;
} user_data_t;

long book_get_user(const char *surname, user_data_t **user_data);
long book_add_user(const user_data_t *user_data);
long book_del_user(const char *surname);

void book_init(void);
void book_exit(void);

#endif // __BOOK_H