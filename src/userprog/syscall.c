#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include <debug.h>

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int syscall_num = *(int*)((f->esp));

  switch(syscall_num)
  {
  case SYS_HALT:
    shutdown_power_off();
    NOT_REACHED();


  case SYS_EXIT:
    int status = *(int*)((f->esp) + 4);
    printf("%s: exit(%d)\n", thread_name(), status);

    sys_exit(status);
    break;


  case SYS_EXEC:
    char* file_name = *(char**)((f->esp) + 4);
    sys_exec(file_name);
    break;


  case SYS_WAIT:
    pid_t pid = *(pid_t*)((f->esp) + 4);
    sys_wait(pid);
    break;
    


//////////

  case SYS_CREATE:


  case SYS_REMOVE:


  case SYS_OPEN:


  case SYS_FILESIZE:


  case SYS_READ:

  
  case SYS_WRITE:


  case SYS_SEEK:


  case SYS_TELL:


  case SYS_CLOSE:


  default:
  }

  //printf ("system call!\n");
  //thread_exit ();
}

void
sys_exit(int status)
{
  thread_current()->exit_code = status;
  thread_exit();
}

void
sys_exec(char* cmdline)
{
  return process_execute(cmdline);
}

void
sys_wait(pid_t pid)
{
  return process_wait(pid);
}
