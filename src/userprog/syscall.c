#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
//+SEA
#include "devices/shutdown.h"
#include "userprog/pagedir.h"

static void syscall_handler (struct intr_frame *);
//void exit (int status);
void s_exit (int status);
void s_halt ();
int s_write(int pr_fd, char *pr_buf, int n);
static bool verify_user (const uint8_t *uaddr);
static bool verify_buf_ptr(const uint8_t *buffer, size_t size);

void syscall_init(void)
{
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
  // intr_register_int (SYS_EXIT, 3, INTR_ON, exit, "exit");
}

/* Gets n arguments from the user stack and stores them in the args array.

// Stack layout for write syscall:
// sp[0] SYS_WRITE
// sp[1] fd
// sp[2] buffer
// sp[3] size
*/ 
static void get_arg(struct intr_frame *f, int *args, int n) {
    // Get the stack pointer
    int *sp = (int *)f->esp;

    for (int i = 0; i < n; i++) {
        // Verify the address is in valid user memory
        if (!verify_user(((uint8_t *)&sp[i + 1]))){
          f->eax = (int)*(int *)(f->esp + 4); 
           s_exit(f->eax);
        }
        
        // Get the argument and store it
        args[i] = sp[i + 1];
    }
}

static void
syscall_handler(struct intr_frame *f UNUSED)
{

  // Extract system call number from stack.
  if (!verify_user(f->esp))
  {

    thread_exit();
  }
  int call_num = (int)*(int *)(f->esp);

  switch (call_num)
  {
  case SYS_HALT:

    s_halt();
    break;
  case SYS_EXIT:


    f->eax = (int)*(int *)(f->esp + 4); // set return value
    s_exit(f->eax);
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

    // Extract fd, buffer, size arguments from user stack
     get_arg(f, args, 3);
     int fd  = args[0];
     char *buf = (char *) args[1];
     int n   = args[2];



     // Set the return value that the user program will see
     f->eax = s_write(fd, buf, n);


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

void s_exit(int status)
{

  printf("%s: exit(%d)\n", thread_current()->name, status);
  thread_current()->exit_status = status;
  thread_exit();
}

void s_halt()
{

  shutdown_power_off();
}

/*
Handles the write syscall and currently only supports writing to stdout.
Returns how many bytes were written, or -1 if the file descriptor is invalid.
*/
int s_write(int pr_fd, char *pr_buf, int n) {
  // void *sp = f->esp;
  // char *buf = *(char **)(sp + 8);
  // int n = *(int *)(sp + 12);
  // int fd = *(int *)(sp + 4);


  // Verify buffer is in valid user memory
  if (!verify_buf_ptr((uint8_t *)pr_buf, n)){
    
    s_exit(-1);
  }

  // Write to stdout
  if (pr_fd == 1) {
    putbuf(pr_buf, n); 
    return n; 
  }

  // Invalid file descriptor or not implemented yet
  return -1;
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

/*
 Validate that all memory addresses in the buffer is valid
 and ensures it comes from user memory space and mapped to
 a valid physical space. RETURN true if all elements in buffer
 comes from user memory space and mapped.RETURN false if that
 condition is not meet.
 */
static bool
verify_buf_ptr(const uint8_t *buffer, size_t size)
{
  for (size_t i = 0; i < size; i++)
  {
    if ( !verify_user(buffer + i)/*!is_user_vaddr(buffer + i)*/)
    {
      return false;
    }
  }

  return true;
}
