#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include <debug.h>
#include "syscall.h"
#include "process.h"
#include "devices/shutdown.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void check_user_vaddr(const void* vaddr)
{
  if(!is_user_vaddr(vaddr))
    sys_exit(-1);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{

  int syscall_num = *(int*)((f->esp));

  //hex_dump(f->esp, f->esp, 150, 1);

  switch(syscall_num)
  {
  case SYS_HALT:
    shutdown_power_off();
    NOT_REACHED();


  case SYS_EXIT:
  {
    check_user_vaddr(f->esp + sizeof(int));
    int status = *(int*)((f->esp) + sizeof(int));

    sys_exit(status);
    break;
  }

  case SYS_EXEC:
  {
    check_user_vaddr(f->esp + sizeof(char*));
    char* file_name = *(char**)((f->esp) + sizeof(char*));
    int exec_result = sys_exec(file_name);
    
    f->eax = exec_result;
    break;
  }

  case SYS_WAIT:
  {
    check_user_vaddr(f->esp + sizeof(pid_t));
    pid_t pid = *(pid_t*)((f->esp) + sizeof(pid_t));
    int wait_result = sys_wait(pid);

    f->eax = wait_result;
    break;
  } 


//////////

  case SYS_CREATE:


  case SYS_REMOVE:


  case SYS_OPEN:


  case SYS_FILESIZE:


  case SYS_READ:

  
  case SYS_WRITE:
    //simple code for debug
    check_user_vaddr(f->esp + 4);
    check_user_vaddr(f->esp + 8);
    check_user_vaddr(f->esp + 12);
    f->eax = write((int)*(uint32_t*)(f->esp+4), (void*)*(uint32_t*)(f->esp+8), (unsigned)*((uint32_t*)(f->esp+12)));
    break;

  case SYS_SEEK:


  case SYS_TELL:


  case SYS_CLOSE:


  default:
    break;
  }

  //printf ("system call!\n");
  //thread_exit ();
}

void
sys_exit(int status)
{
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_current()->exit_code = status;
  thread_exit();
}

int
sys_exec(char* cmdline)
{
  //printf("sys_exec called! \ncmdline: %s\n", cmdline);
  return process_execute(cmdline);
}

int
sys_wait(pid_t pid)
{
  //printf("sys_wait called!\n");
  return process_wait(pid);
}

//simple code for debug, not my own code
int write(int fd, void* buffer, unsigned size)
{
  if(fd == 1)
  {
    putbuf(buffer, size);
    return size;
  }
  return -1;
}
