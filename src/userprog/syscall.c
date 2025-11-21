#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
//+SEA
#include "devices/shutdown.h"
#include "userprog/pagedir.h"

static void syscall_handler(struct intr_frame *);
// void exit (int status);
void s_exit();
void s_halt();
void s_write(void *sp);
static bool verify_user(const uint8_t *uaddr);

void syscall_init(void)
{
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
  // intr_register_int (SYS_EXIT, 3, INTR_ON, exit, "exit");
}

static void
syscall_handler(struct intr_frame *f UNUSED)
{
  printf("system call!\n");
  // Extract system call number from stack.
  if (!verify_user(f->esp))
  {
    printf("ERROR.  Bad user address.\n");
    thread_exit();
  }
  int call_num = (int)*(int *)(f->esp);
  printf("Call num %d\n", call_num);
  switch (call_num)
  {
  case SYS_HALT:
    printf("Sys halt\n");
    s_halt();
    break;
  case SYS_EXIT:
    printf("Sys exit detected.\n");
    printf("Status: %d\n", (int)*(int *)(f->esp + 4));
    f->eax = (int)*(int *)(f->esp + 4); // set return value
    s_exit();
    break;
  case SYS_EXEC:
    break;
  case SYS_WAIT:
    break;
  case SYS_CREATE:
    break;
  case SYS_REMOVE:
    break;
  case SYS_OPEN:
    break;
  case SYS_FILESIZE:
    break;
  case SYS_READ:
    break;
  case SYS_WRITE:
    printf("Sys write\n");
    s_write(f->esp);
    break;
  case SYS_SEEK:
    break;
  case SYS_TELL:
    break;
  case SYS_CLOSE:
    break;
  default:
    printf("Sys call is unknown!!\n");
  }
  thread_exit();
}

void s_exit()
{
  printf("got to sysexit\n");
  thread_exit();
}

void s_halt()
{
  printf("got to syshalt\n");
  shutdown_power_off();
}

void s_write(void *sp)
{
  char *buf = (char *)(sp + 12);
  int n = (int)(int *)(sp + 8);
  printf("num %d\n", n);

  // putbuf (buf,n);
}

/*
Virtual Memory Layout:

virtual memory in pintos devided in two regions: user virtual memory (0 up to PHYS_BASE (3GB)) and kernel virtual memory(PHYS_BASE up to 4GB). User virtual memory is per-process. when the kernel switches from one process to another, it also switches user virtual address spaces by changing processor's page directory base register. Kernel virtual memory is global. It is always mapped the same way, regardless of what user process or kernel thread is running. A user program can only access its own virtual memory, any attempt to access kernel virtual memory causes a page fault. Kernel can access both its own virtual memory and virtual memory of a running user process. However even in kernel an attempt to access memory of an unmapped user virtual address will cause page fault.



Need for checking:
As part a system call, the kernel must often access memory through pointers provided by the user program. The user could pass a null pointer, a pointer to unmapped virtual memory or a pointer to kernel virtual memory space. All mentioned pointers are harmful to kernel an could cause crush and pagefault, basically by checking we make sure that the pointers are valid and will not harm the kernel.

Pintos: chap3: 3.1.4 - 3.1.5

*/

/* Validate data user virtual address uaddr.
   UADDR must be below PHYS_BASE.
   It must not be null.
   It must be a mapped address.
   Returns true if successful, false if not.
   +SEA
*/
static bool
verify_user(const uint8_t *uaddr)
{
  int result;
  // if (uaddr == NULL || uaddr >= PHYS_BASE)
  //   return false;
  // is it mapped?  How do I check this?
  // to check if the address is mapped if not return false

  // to check pointer is not null
  if (uaddr == NULL)
  {
    return false;
  }
  // to check the VADDR is user virtual address
  if (!is_user_vaddr(uaddr))
    return false;
  // retuns false if the address in not mapped
  return pagedir_get_page(thread_current()->pagedir, uaddr) != NULL;
}
