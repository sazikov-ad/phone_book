#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>

#include <asm/uaccess.h>

MODULE_LICENSE("BSD");
MODULE_AUTHOR("Andrey Sazikov <sazikov.ad@phystech.edu>");
MODULE_VERSION("1.0");

static int __init dev_init(void);
static void __exit dev_exit(void);

module_init(dev_init);
module_exit(dev_exit);
