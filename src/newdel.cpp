/*
 * newdel.cpp
 *
 *  Created on: Nov 18, 2008
 *      Author: bistromath
 *
 */

#include <sys/types.h>
#include <stdlib.h>

void *operator new(size_t arg)
{
    return malloc(arg);
}

void *operator new[](size_t arg)
{
    return malloc(arg);
}

void operator delete(void *arg)
{
    free(arg);
}

void operator delete[](void *arg)
{
    free(arg);
}
