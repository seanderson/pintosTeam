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
int s_write(int pr_fd, char *pr_buf, int n);
static bool verify_user (const uint8_t *uaddr);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  //intr_register_int (SYS_EXIT, 3, INTR_ON, exit, "exit");
}

static void get_arg(struct intr_frame *f, int *args, int n) {
    int *sp = (int *) f->esp;

    for (int i = 0; i < n; i++) {
        args[i] = sp[i + 1];
    }
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
    int args[3];
     get_arg(f, args, 3);
     int fd  = args[0];
     char *buf = (char *) args[1];
     int n   = args[2];
     f->eax = s_write(fd, buf, n);
    //printf("Sys write\n");
    //s_write(f);
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
  //thread_exit ();
}

void s_exit () {
  printf("got to sysexit\n");
  thread_exit();
}

void s_halt () {
  printf("got to syshalt\n");
  shutdown_power_off();
}

int s_write(int pr_fd, char *pr_buf, int n) {
  // void *sp = f->esp;
  // char *buf = *(char **)(sp + 8);
  // int n = *(int *)(sp + 12);
  // printf("num %d\n",n);
  // int fd = *(int *)(sp + 4);
  // printf("fd %d\n", fd);

  if (pr_fd == 1) {
    putbuf(pr_buf, n);
    return n; 
  }

  return 0;
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
