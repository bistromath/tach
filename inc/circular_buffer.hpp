//circular_buffer.hpp
//implements a really simple circular buffer for incoming and outgoing messages on the Zylight Killer
//no i am not using templates, all it works on is char, live with it
//it should be thread-safe, i.e., interrupt-safe
//Nick Foster, 2008

//things it needs to do:
//push
//pop
//read (pop) count
//write (push) count
//initialize to a given size

extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
}

const u8 CB_UNDERFLOW=-1;
const u8 CB_SUCCESS=0;

class circular_buffer {
    public:
        circular_buffer(volatile char *buf, unsigned int length)
        {
            //constructor
            size = length;
            end = begin = buffer = buf;
            read_count = write_count = 0;
        }

        char pop(volatile char *data)
        {
            //pop returns -1 if you blew it and underran the buffer, without exceptions there ain't much else i can do
            if(isempty()) return CB_UNDERFLOW;
            begin++;
            if(begin == (buffer + size)) begin -= size;
            *data = *begin;
            read_count++;
            if(end == begin) write_count = read_count; //if you popped it empty, well there ain't nothing left
            return CB_SUCCESS;
        }

        void push(char byte) //okay this will get shitty fast if you don't have a way to reset overflow counters
        {
            //push
        	end++;

            if(end == (buffer + size)) end -= size;
            *(end) = byte;

            write_count++;
            if((write_count - read_count) > (size-1)) begin++; //this is an overflow
            if(begin == (buffer + size)) begin -= size;
        }

        volatile char peeklast() { return *end; }
        volatile char peekfirst() { volatile char *newbegin = begin+1; if(newbegin == (buffer+size)) newbegin -= size; return *newbegin; }
        volatile char peek(u8 offset) {
        	volatile char *newbegin = begin + offset + 1;
        	if(newbegin >= (buffer+size))
        		newbegin -= size;
        	return *newbegin;
        }

        unsigned int unread()
        {
            return write_count - read_count;
        }

        bool isempty()
        {
            return write_count==read_count; //lots quicker than subtraction
        }

        bool isoverrun()
        {
            return (write_count - read_count) > (size-1);
        }

        bool isfull()
        {
            return (write_count - read_count) == (size-1);
        }

    private:
        unsigned int write_count;
        unsigned int read_count;
        unsigned int size;
        volatile char *begin;
        volatile char *end;
        volatile char *buffer;
};
