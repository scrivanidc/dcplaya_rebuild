
#ifndef _JMPBUF_H_
#define _JMPBUF_H_

typedef char jmp_buf[13*4];


int longjmp(jmp_buf buf, int what_is_that);
int setjmp(jmp_buf buf);

#endif
