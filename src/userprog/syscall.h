#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/user/syscall.h"

typedef int pid_t;

void syscall_init (void);

void sys_exit(int status);
pid_t sys_exec(const char* cmdline);
int sys_wait(pid_t pid);
bool sys_create (const char* file, unsigned initial_size);
bool sys_remove (const char* file);
int sys_open(const char* file);
int sys_filesize(int fd);
int sys_read(int fd, void* buffer, unsigned size);
int sys_Write(int fd, void* buffer, unsigned size);
void sys_seek(int fd, unsigned position);
int sys_tell(int fd);
void sys_close(int fd);
void pointer_address_checking(void* esp, int argc);

#endif /* userprog/syscall.h */
