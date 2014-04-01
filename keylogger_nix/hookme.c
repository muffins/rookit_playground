#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/version.h>
#include <linux/syscalls.h>
#include <linux/security.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/dcache.h>

//include for the read/write semaphore
#include <linux/rwsem.h>

//needed for set_memory_rw
#include <asm/cacheflush.h>

#include "offsets.h"

//These two definitions and the one for set_addr_rw and ro are from
// http://stackoverflow.com/questions/2103315/linux-kernel-system-call-hooking-example
#define GPF_DISABLE() write_cr0(read_cr0() & (~0x10000))
#define GPF_ENABLE() write_cr0(read_cr0() | 0x10000)

//sudo grep sys_call_table /proc/kallsyms
//c1513160 R sys_call_table
static long* sys_call_table = (long*)SYS_CALL_TABLE_ADDR;

typedef asmlinkage long (* sys_open_func_ptr)(const char __user* filename, int flags, int mode);

sys_open_func_ptr sys_open_orig = NULL;


typedef asmlinkage long (* sys_read_func_ptr)(unsigned int fd, char __user* buf, size_t count);

sys_read_func_ptr sys_read_orig = NULL;


static struct rw_semaphore myrwsema; 

//don't forget the asmlinkage declaration. This is a particular calling convention
asmlinkage long my_sys_open(const char __user* filename, int flags, int mode)
{
  long ret = 0;
  char* tmp = NULL;

  //add in another reader to the semaphore
  down_read(&myrwsema);

  ret = sys_open_orig(filename, flags, mode);

  tmp = getname(filename); //this is needed because filename is in userspace
  printk(KERN_INFO "The file [%s] is being opened\n", tmp);

  //release the reader lock (or one of them) right before the return
  // to limit the possibility of unloading the module
  // when there is still code to be executed
  up_read(&myrwsema);
  return (ret);
}

asmlinkage long my_sys_read(unsigned int fd, char __user* buf, size_t count)
{
  long ret = 0;
  down_read(&myrwsema);
  
  ret = sys_read_orig(fd, buf, count);

  up_read(&myrwsema);
  return (ret);
}

int set_addr_rw(unsigned long addr)
{
  unsigned int level; 
  pte_t* pte = lookup_address(addr, &level);
  if (pte == NULL)
  {
    return (-1);
  }

  pte->pte |= _PAGE_RW;

  return (0);
}

int set_addr_ro(unsigned long addr)
{
  unsigned int level; 
  pte_t* pte = lookup_address(addr, &level);
  if (pte == NULL)
  {
    return (-1);
  }

  pte->pte = pte->pte & ~_PAGE_RW;

  return (0);
}

int init_module(void)
{
  //sys_close is exported, so we can use it to make sure we have the
  // right address for sys_call_table
  //printk(KERN_INFO "sys_close is at [%p] == [%p]?.\n", sys_call_table[__NR_close], &sys_close);
  if (sys_call_table[__NR_close] != (long)(&sys_close))
  {
    printk(KERN_INFO "Seems like we don't have the right addresses [0x%08lx] vs [%p]\n", sys_call_table[__NR_close], &sys_close);
    return (-1); 
  }

  //initialize the rw semahore
  init_rwsem(&myrwsema);

  //make sure the table is writable
  set_addr_rw( (unsigned long)sys_call_table);
  //GPF_DISABLE();

  printk(KERN_INFO "Saving sys_open @ [0x%08lx]\n", sys_call_table[__NR_open]);
  sys_open_orig = (sys_open_func_ptr)(sys_call_table[__NR_open]);
  sys_call_table[__NR_open] = (long)&my_sys_open;

  printk(KERN_INFO "Saving sys_read @ [0x%08lx]\n", sys_call_table[__NR_read]);
  sys_read_orig = (sys_read_func_ptr)(sys_call_table[__NR_read]);
  sys_call_table[__NR_read] = (long)&my_sys_read;

  set_addr_ro( (unsigned long)sys_call_table);
  //GPF_ENABLE();

  return (0);
}

void cleanup_module(void)
{
  if (sys_open_orig != NULL)
  {
    set_addr_rw( (unsigned long)sys_call_table);

    printk(KERN_INFO "Restoring sys_open\n");
    sys_call_table[__NR_open] = (long)sys_open_orig; 

    printk(KERN_INFO "Restoring sys_read\n");
    sys_call_table[__NR_read] = (long)sys_read_orig; 

    set_addr_ro( (unsigned long)sys_call_table);
  }

  //after the system call table has been restored - we will need to wait
  printk(KERN_INFO "Checking the semaphore as a write ...\n");
  down_write(&myrwsema);
  
  printk(KERN_INFO "Have the write lock - meaning all read locks have been released\n");
  printk(KERN_INFO " So it is now safe to remove the module\n");
}

MODULE_LICENSE("GPL");
