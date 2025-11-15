#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
//+SEA
#include "devices/shutdown.h"

static void syscall_handler (struct intr_frame *);
//void exit (int status);
void s_exit ();
void s_halt ();
void s_write(void *sp);
static bool verify_user (const uint8_t *uaddr);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  //intr_register_int (SYS_EXIT, 3, INTR_ON, exit, "exit");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  // Extract system call number from stack.
  if (!verify_user(f->esp)) {
    printf("ERROR.  Bad user address.\n");
    thread_exit();
  }
  int call_num = (int) *(int *)(f->esp);
  printf("Call num %d\n",call_num);
  switch (call_num) {
    case SYS_HALT:
    printf("Sys halt\n");
    s_halt();
    break;
  case SYS_EXIT:
    printf ("Sys exit detected.\n");
    printf("Status: %d\n", (int) *(int *)(f->esp + 4));
    f->eax = (int) *(int *)(f->esp + 4); // set return value
    s_exit ();
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
  thread_exit ();
}

void s_exit () {
  printf("got to sysexit\n");
  thread_exit();
}

void s_halt () {
  printf("got to syshalt\n");
  shutdown_power_off();
}

void s_write(void *sp) {
  char *buf = (char *)(sp + 12);
  int n = (int)(int *)(sp + 8);
  printf("num %d\n",n);

  //putbuf (buf,n);
}

/* Validate data user virtual address uaddr.
   UADDR must be below PHYS_BASE.
   It must not be null.
   It must be a mapped address.
   Returns true if successful, false if not.
   +SEA
*/
static bool 
verify_user (const uint8_t *uaddr)
{
  int result;
  if (uaddr == NULL || uaddr >= PHYS_BASE) return false;
  // is it mapped?  How do I check this?
  return true;
}
