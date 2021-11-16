#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include <debug.h>
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/off_t.h"
#include "devices/shutdown.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "lib/string.h"
#include "devices/input.h"
#include "userprog/pagedir.h"

struct lock filesys_lock;

struct file
{
  struct inode *inode;
  off_t pos;
  bool deny_write;
};

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  lock_init(&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  void* argup = f->esp;
  uint32_t syscall_num = *(uint32_t*)argup;

  if(!is_user_vaddr(argup))
    sys_exit(-1);
  
  switch(syscall_num)
  {
  case SYS_HALT:
    shutdown_power_off();
    NOT_REACHED();
    break;
  case SYS_EXIT:
    pointer_address_checking(argup, 1);
    sys_exit(*(uint32_t*)(argup+4));
    break;
  case SYS_EXEC:
    pointer_address_checking(argup, 1);
    f->eax = sys_exec((const char*)*(uint32_t*)(argup+4));
    break;
  case SYS_WAIT:
    pointer_address_checking(argup, 1);
    f->eax = sys_wait((pid_t)*(uint32_t*)(argup+4));
    break; 
  case SYS_CREATE:
    pointer_address_checking(argup, 2);
    f->eax = sys_create((const char *)*(uint32_t*)(argup+4), (unsigned)*(uint32_t*)(argup+8));
    break;
  case SYS_REMOVE:
    pointer_address_checking(argup, 1);
    f->eax = sys_remove((const char*)*(uint32_t*)(argup+4));
    break;
  case SYS_OPEN:
    pointer_address_checking(argup, 1);
    f->eax = sys_open((const char*)*(uint32_t*)(argup+4));
    break;
  case SYS_FILESIZE:
    pointer_address_checking(argup, 1);
    f->eax = sys_filesize((int)*(uint32_t*)(argup+4));
    break;
  case SYS_READ:
    pointer_address_checking(argup, 3);
    f->eax = sys_read((int)*(uint32_t*)(argup+4), (void*)*(uint32_t*)(argup+8), (unsigned)*(uint32_t*)(argup+12));
    break;
  case SYS_WRITE:
    pointer_address_checking(argup, 3);
    f->eax = sys_write((int)*(uint32_t*)(argup+4), (void*)*(uint32_t*)(argup+8), (unsigned)*(uint32_t*)(argup+12));
    break;
  case SYS_SEEK:
    pointer_address_checking(argup, 2);
    sys_seek((int)*(uint32_t*)(argup+4), (unsigned)*(uint32_t*)(argup+8));
    break;
  case SYS_TELL:
    pointer_address_checking(argup, 1);
    f->eax = sys_tell((int)*(uint32_t*)(argup+4));
    break;
  case SYS_CLOSE:
    pointer_address_checking(argup, 1);
    sys_close((int)*(uint32_t*)(argup+4));
    break;
  default:
    break;
  }
}

void
sys_exit(int status)
{
  printf("%s: exit(%d)\n", thread_name(), status);
  struct thread* cur = thread_current();
  cur->exit_code = status;
  int i;
  for(i=3;i<128;i++)
  {
    if(thread_current()->fd[i] != NULL)
      sys_close(i);
  }
  
thread_exit();
}

pid_t
sys_exec(const char* cmdline)
{
  return process_execute(cmdline);
}

int
sys_wait(pid_t pid)
{
  return process_wait(pid);
}

bool
sys_create(const char* file, unsigned initial_size)
{
  if(file==NULL)
    sys_exit(-1);
  return filesys_create(file, initial_size);
}

bool
sys_remove(const char* file)
{
  if(file==NULL)
    sys_exit(-1);
  return filesys_remove(file);
}

int
sys_open(const char* file)
{
  if(file == NULL)
    sys_exit(-1);
  lock_acquire(&filesys_lock);
  struct file* ret_file = filesys_open(file);
  int ret = -1;
  
  if(ret_file != NULL)
  {
    int i = 0;
    for(i=3;i<128;i++)
    {
      if(strcmp(thread_name(), file) == 0)
        file_deny_write(ret_file);

      if(thread_current()->fd[i]==NULL)
      {
        thread_current()->fd[i] = ret_file;

        ret = i;
        break;
      }
    }
  }
  lock_release(&filesys_lock);
  return ret;  
}

int
sys_filesize(int fd)
{
  struct file* fd_file = thread_current()->fd[fd];
  if (fd_file == NULL)
    sys_exit(-1);
  else
    return file_length(fd_file);

  NOT_REACHED();
}

int
sys_read(int fd, void *buffer, unsigned size)
{
  if(!is_user_vaddr(buffer))
    sys_exit(-1);
  int ret = -1;
  lock_acquire(&filesys_lock);
  if(fd == 0)
  {
    unsigned int i = 0;
    for(i = 0;i < size; i++)
    {
      if(((char*)buffer)[i]=='\0')
        break;
    }
    
    ret = i;
  } 
  else if(fd > 2)
  {
    struct file* fd_file = thread_current()->fd[fd];
    if(fd_file == NULL)
    {
      lock_release(&filesys_lock);
      sys_exit(-1);
    }
    ret = file_read(fd_file, buffer, size);
  }
  lock_release(&filesys_lock);
  return ret;
}

int
sys_write(int fd, void *buffer, unsigned size)
{
  if(!is_user_vaddr(buffer))
    sys_exit(-1);
  int ret = -1;
  lock_acquire(&filesys_lock);
  if(fd == 1)
  {
    putbuf(buffer, size);
    ret = size;
  }
  else if(fd > 2)
  { 
    struct file* fd_file = thread_current()->fd[fd];
    if(fd_file == NULL)
    {
      lock_release(&filesys_lock);
      sys_exit(-1);
    }	
    ret = file_write(fd_file, buffer, size);
  }
  lock_release(&filesys_lock);
  return ret;
}

void
sys_seek(int fd, unsigned position)
{
  struct file* fd_file = thread_current()->fd[fd];
  if(fd <= 2 || fd_file == NULL)
    sys_exit(-1);
  else
    file_seek(fd_file, position);
}

int
sys_tell(int fd)
{
  struct file* fd_file = thread_current()->fd[fd];
  if(fd <= 2 || fd_file == NULL)
    sys_exit(-1);
  else
    return file_tell(fd_file);

  NOT_REACHED();
}

void
sys_close(int fd)
{
  struct file* fd_file = thread_current()->fd[fd];
  if(fd <= 2 || fd_file == NULL)
    sys_exit(-1);
  else
  { 
    file_close(fd_file);
    thread_current()->fd[fd] = NULL;
    //fd_file = NULL;
  }
}

void
pointer_address_checking(void* esp, int argc)
{
   int i = 0;
   for(i = 1; i <= argc ; i++)
   {
     if(!is_user_vaddr(esp + 4*i))
       sys_exit(-1);
   }
   void* pagep = pagedir_get_page(thread_current()->pagedir, esp);
   if(pagep == NULL)
     sys_exit(-1);
}

