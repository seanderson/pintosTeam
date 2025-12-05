#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
//+SEA
#include "devices/shutdown.h"
#include "userprog/pagedir.h"
#include "filesys/filesys.h"
#include "filesys/file.h"


static void syscall_handler (struct intr_frame *);
//void exit (int status);
void s_exit (int status);
void s_halt ();
int s_write(int pr_fd, char *pr_buf, int n);
static bool verify_user (const uint8_t *uaddr);
static bool verify_buf_ptr(const uint8_t *buffer, size_t size);
int check_invalid_string_error (const void *sbuf);
int user_to_kernel_ptr(const void *vaddr);

#define MAX_ARGS 5 // SEA
static void *s_args[MAX_ARGS]; // getting args from functions

struct fd {
    int index;
    struct file *file;
    struct list_elem elem;
};


static struct list fd_table; //fd table

// Next free fd index
static int next_fd = 2; // 0 is stdin, 1 is stdout, so start with 2


void syscall_init(void)
{
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
  // intr_register_int (SYS_EXIT, 3, INTR_ON, exit, "exit");
  list_init(&fd_table); // Initialize fd_table list
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
        // Verify the stack pointer address is valid 
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
  //printf("system call!\n");
  char *fname; // for filenames
  //int args[3];

  // Extract system call number from stack.
  if (!verify_user(f->esp))
  {
    printf("ERROR.  Bad user address.\n");
    thread_exit();
  }
  int call_num = (int)*(int *)(f->esp);
  //printf("Call num %d\n", call_num);
  switch (call_num)
  {
  case SYS_HALT:
    //printf("Sys halt\n");
    s_halt();
    break;
  case SYS_EXIT:
    //printf("Sys exit detected.\n");
    //printf("Status: %d\n", (int)*(int *)(f->esp + 4));
    f->eax = (int)*(int *)(f->esp + 4); // set return value
    s_exit(f->eax);
    break;
  case SYS_EXEC:
    get_arg(f,(void **)&s_args,1); // get entire exec string
    fname = (char *) s_args[0];
    check_invalid_string_error(fname);
    //printf("Running %s\n",fname);
    tid_t child_tid = process_execute (fname); // Here fname includes args
    f->eax = child_tid;
    break;
  case SYS_WAIT:
    break;
  case SYS_CREATE:
    
    get_arg(f, (int *)s_args, 2);
    fname = (char *)s_args[0];
    unsigned initial_size = (unsigned)s_args[1];

    // Check if the filename address is valid
    check_invalid_string_error(fname);

    // Create a file 
    f->eax = filesys_create(fname, initial_size);
    break;
  case SYS_REMOVE:
    break;
  case SYS_OPEN:
  
    // arr for storing 1 element
    get_arg(f, (int *)s_args, 1);
    fname = (char *)s_args[0];

    // Check if the filename address is valid
    check_invalid_string_error(fname);

    // if its empty exit == -1
    if (fname[0] == '\0'){
      f->eax = -1;
      break;
    }
    
    // Open the file 
    struct file *file_pt = filesys_open(fname);
    
    // to check the existence of file with given fileName
    if (file_pt == NULL){
      f->eax = -1;
      break;
    }

    // Assign the file to the free fd
    f->eax = -1;      

    // Search through file list
    struct list_elem *e;
    struct fd *cur;

    // Look for the first free fd
    for (e = list_begin(&fd_table); e != list_end(&fd_table); e = list_next(e)) {
        cur = list_entry(e, struct fd, elem);

        if (cur->file == NULL) {    // free fd found
            cur->file = file_pt;
            f->eax = cur->index;
            return;
        }
    }

    // create new entry if no free fd found
    struct fd *new_fd = malloc(sizeof(struct fd));
    new_fd->index = next_fd++;
    new_fd->file  = file_pt;

    list_push_back(&fd_table, &new_fd->elem);
    f->eax = new_fd->index;


    

    break;
  case SYS_FILESIZE:
    break;
  case SYS_READ:
    get_arg(f, (int *)s_args, 3);
    int read_fd = (int)s_args[0];
    char *read_buf = (char *)s_args[1];
    int read_size = (int)s_args[2];

    // Check if buffer works
    if (!verify_buf_ptr((uint8_t *)read_buf, read_size)) {
          s_exit(-1);
    }

    if (read_fd == 1){
       f->eax = -1;
       break;
    }

    // Read from stdin
    if (read_fd == 0) {
      for (int i = 0; i < read_size; i++) {
          read_buf[i] = input_getc();
      }
      f->eax = read_size;
      break;
    }

    // Search and read from file
    if (read_fd >= 2) {
    struct list_elem *e; // For struct fd
    for (e = list_begin(&fd_table); e != list_end(&fd_table); e = list_next(e)) {
        struct fd *cur = list_entry(e, struct fd, elem); // current fd
        if (cur->index == read_fd) {
            // if file exists, read it
            f->eax = (cur->file != NULL)
                     ? file_read(cur->file, read_buf, read_size)
                     : -1;
            return;   
        }
    }
}


    

    
   
    break;

  case SYS_WRITE:
    
    // Extract fd, buffer, size arguments from user stack
     get_arg(f, (int *)s_args, 3);
     int fd  = (int)s_args[0];
     char *buf = (char *) s_args[1];
     int n   = (int)s_args[2];

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

  // Verify buffer is valid
  if (!verify_buf_ptr((uint8_t *)pr_buf, n)){
    
    s_exit(-1);
  }

  // Write to stdout
  if (pr_fd == 1) {
    putbuf(pr_buf, n); 
    return n; 
  }
  
  // Write to file if fd is valid
  if (pr_fd > 1) {
    struct list_elem *e;
    for (e = list_begin(&fd_table); e != list_end(&fd_table); e = list_next(e)) {
      struct fd *cur = list_entry(e, struct fd, elem);
      if (cur->index == pr_fd) { // Check if the entry is the fd we are looking for
        if (cur->file != NULL) { // If the current file is not empty
          return file_write(cur->file, pr_buf, n);
        }
      }
    }
  }

  // Invalid fd
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

/*
  Check valid string and valid pointers in it.
*/
int check_invalid_string_error (const void *sbuf)
{
  char *ptr = (char *) sbuf;
  int strlen = 0;
  if (ptr == NULL) s_exit(ERROR); // null pointer for name
  while (ptr < (char *) sbuf + MAXFILENAMELEN) {
    user_to_kernel_ptr(ptr); // check valid and mapped
    if (*ptr == '\0') break;
    ptr++; strlen++;
  }
  
  return strlen;
}

/*
  Get address in page.
*/
int user_to_kernel_ptr(const void *vaddr)
{
  //check_invalid_ptr_error(vaddr);
  if (! verify_user( vaddr)) {
    s_exit(ERROR);
  }

  void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
  if (!ptr)
    {
      s_exit(ERROR);
    }
  return (int) ptr;
}
