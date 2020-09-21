#include "book.h"

#include <linux/list.h>
#include <linux/slab.h>

typedef struct book_record {
	user_data_t user;
	struct book_record *next;
} book_record_t;

static book_record_t *root = NULL;

static book_record_t *allocate_book_record(void)
{
	book_record_t *res = (book_record_t *)kmalloc(sizeof(*res), GFP_KERNEL);
	memset(&res->user, 0, sizeof(user_data_t));
	res->next = NULL;
	return res;
}

static char *clone_string(const char *string)
{
	unsigned int len = strlen(string);
	char *res = (char *)kmalloc(sizeof(char) * (len + 1), GFP_KERNEL);
	memcpy(res, string, len + 1);
	return res;
}

static void clone_user_data_to(const user_data_t *src, user_data_t *dst)
{
	dst->surname = clone_string(src->surname);
	dst->name = clone_string(src->name);
	dst->phone = clone_string(src->phone);
	dst->email = clone_string(src->email);
	dst->age = src->age;
}

static user_data_t *clone_user_data(const user_data_t *user_data)
{
	user_data_t *copy = (user_data_t *)kmalloc(sizeof(*copy), GFP_KERNEL);
	clone_user_data_to(user_data, copy);
	return copy;
}

void book_init(void)
{
}

void book_exit(void)
{
	book_record_t *cur = root;

	while (cur != NULL) {
		book_record_t *next = cur->next;
		kfree(cur);
		cur = next;
	}
}

long book_add_user(const user_data_t *user)
{
	book_record_t *new_node = allocate_book_record();

	clone_user_data_to(user, &new_node->user);

	new_node->next = root;
	root = new_node;

	return 0;
}

long book_get_user(const char *surname, user_data_t **user)
{
	book_record_t *cur = root;

	while (cur != NULL) {
		if (strcmp(cur->user.surname, surname) == 0) {
			user_data_t *copy = clone_user_data(&cur->user);
			*user = copy;
			return 0;
		}
		cur = cur->next;
	}

	return 1;
}

long book_del_user(const char *surname)
{
	book_record_t *prev = NULL, *cur = root;

	while (cur != NULL) {
		if (strcmp(cur->user.surname, surname) == 0) {
			if (prev == NULL) {
				root = cur->next;
			} else {
				prev->next = cur->next;
			}

			kfree(cur->user.surname);
			kfree(cur->user.name);
			kfree(cur->user.email);
			kfree(cur->user.age);
			kfree(cur);

			return 0;
		}

		prev = cur;
		cur = cur->next;
	}

	return 1;
}
