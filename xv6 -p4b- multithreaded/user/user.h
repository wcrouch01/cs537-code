#ifndef _USER_H_
#define _USER_H_
#define PGROUNDUP(sz)  (((sz)+4096-1) & ~(4096-1))
struct stat;
// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(char*, int);
int mknod(char*, short, short);
int unlink(char*);
int fstat(int fd, struct stat*);
int link(char*, char*);
int mkdir(char*);
int chdir(char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int clone(void(*fcn)(void *, void *), void *arg1, void *arg2, void *stack);
int join(void **stack);
// user library functions (ulib.c)
int stat(char*, struct stat*);
char* strcpy(char*, char*);
void *memmove(void*, void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, char*, ...);
char* gets(char*, int max);
uint strlen(char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
int thread_create(void (*start_routine)(void *, void *), void *arg1, void *arg2);
int thread_join();
void lock_acquire(lock_t *);
void lock_release(lock_t *);
void lock_init(lock_t *);
#endif // _USER_H_

