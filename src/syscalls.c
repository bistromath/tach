/*
 * syscalls.c
 *
 *  Created on: Nov 13, 2008
 *      Author: bistromath
 */

#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "syscalls.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

/*
 * sbrk -- changes heap size. Get nbytes more
 *         RAM. We just increment a pointer in what's
 *         left of memory on the board.
 *         Modify this function to look for the stack pointer, to prevent stack collisions.
 *         Heap grows up, stack grows down.
 */

extern char __heap_start, __ram_end;
extern int errno;

extern void debug_write(char*, int);
caddr_t heap_ptr = NULL;

caddr_t _sbrk(size_t nbytes)
{
  caddr_t        base;
  caddr_t		 next;
  if(heap_ptr == NULL)
		heap_ptr = (caddr_t) &__heap_start;

  next = (caddr_t) (((unsigned int) (heap_ptr + nbytes) + 7) & ~7);

  if (next <= (caddr_t) &__ram_end) { //was RAMSIZE, now it says out of the way of the stack
    base = heap_ptr;
    heap_ptr = next;
    return (base);
  } else {
    errno = ENOMEM;
    return ((caddr_t)-1);
  }
}

int _write(int fd, char *buf, int count)
{
	if(fd == 0x01) { //fd 0x01 appears to be the stdout
		debug_write(buf, count);
		return 0;
	}
	else return -1;
}

void _exit(int a)
{
	while(1);
}

int _close(int fd)
{
	return 0;
}

caddr_t _lseek(int fd, caddr_t offset, int origin)
{
	return 0;
}

int _read(int fd, char *buf, unsigned int count)
{
	return 0;
}

int _getpid(void)
{
	return 0;
}

int _fstat(int fd, struct stat *buf)
{
	buf->st_mode = S_IFCHR;
	return 0;
}

int _stat(const char *file, struct stat *buf)
{
	buf->st_mode = S_IFCHR;
	return 0;
}

int _isatty(int fd)
{
	return 1; //yes everything is a tty
}

int _kill(int pid, int sig)
{
	while(1); //never used
	errno = EINVAL;
	return (-1);
}

int _open(const char *filename, int oflag, int pmode)
{
	return 0;
}

int _link(char *old, char *nu)
{
	errno = EMLINK;
	return -1;
}

//unlink
//Remove a file's directory entry. Minimal implementation:
int _unlink(char *name)
{
	errno = ENOENT;
	return -1;
}
