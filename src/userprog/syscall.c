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
void s_exit (int status);
void s_halt ();
void s_write(struct intr_frame *f);
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

  // Extract system call number from stack.
  if (!verify_user(f->esp)) {
    printf("ERROR.  Bad user address.\n");
    thread_exit();
  }
  int call_num = (int) *(int *)(f->esp);

  switch (call_num) {
    case SYS_HALT:
    printf("Sys halt\n");
    s_halt();
    break;
  case SYS_EXIT:

    f->eax = (int) *(int *)(f->esp + 4); // set return value
    s_exit (f->eax);
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
    s_write(f);
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
} // I removed the thread_exit() here. Wouldn't make sense if 

void s_exit (int status) {
  printf("%s: exit(%d)\n", thread_current()->name, status);
  thread_current()->exit_status = status;
  thread_exit();
}

void s_halt () {
  printf("got to syshalt\n");
  shutdown_power_off();
}

void s_write(struct intr_frame *f) {
  int fd = *(int *)(f->esp + 4);
  char *buf = *(char **)(f->esp + 8);
  int size = *(int *)(f->esp + 12);

  if (!verify_user((uint8_t *)buf)) {
    s_exit(fd);
  }

  if (fd == 1) {           // stdout
    putbuf(buf, size);
    f->eax = size;
    return; // Return here, not outside of if
  }

  f->eax = 0;
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
  return (uaddr != NULL && uaddr < PHYS_BASE);
}
