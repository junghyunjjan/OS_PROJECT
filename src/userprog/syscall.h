#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

typedef int pid_t;

void syscall_init (void);

void sys_halt(void);
void sys_exit(int status);
int sys_exec(char* cmdline);
int sys_wait(pid_t pid);
int write(int fd, void* buffer, unsigned size);

#endif /* userprog/syscall.h */
