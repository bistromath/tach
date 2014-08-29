/*
 * syscalls.h
 *
 *  Created on: Nov 13, 2008
 *      Author: bistromath
 */

#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#include <sys/types.h>
#include <sys/stat.h>

#define RAMSIZE (caddr_t)0x20005000
#define PAGESIZE 2048
#define malloc_getpagesize 2048

caddr_t _sbrk(size_t nbytes);
int _write(int fd, char *buf, int count);
void _exit(int a);
int _close(int fd);
int _open(const char *filename, int oflag, int pmode);
caddr_t _lseek(int fd, caddr_t offset, int origin);
int _read(int fd, char *buf, unsigned int count);
int _getpid(void);
int _fstat(int fd, struct stat *buf);
int _stat(const char *file, struct stat *buf);
int _isatty(int fd);
int _kill(int pid, int sig);
int _link(char *old, char *nu);
int _unlink(char *name);

//void free(void *); //can't f'in overload functions in C!

#endif /* SYSCALLS_H_ */
