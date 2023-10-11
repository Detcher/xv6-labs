/*
** For checking stack frame of xv6 using gdb
*/
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

#pragma GCC push_options
#pragma GCC optimize (0)

int foo3( int c, int d, int e, int f, int g, int h, int i ) {
    int add_on3 = 12;
    return add_on3 + (c + d + e + f + g + h + i);
}

int foo2( int b, int c, int d, int e, int f, int g, int h, int i ) {
    int add_on2 = 11;
    return add_on2 + b + foo3( c, d, e, f, g, h, i );
}

int foo1( int a, int b, int c, int d, int e, int f, int g, int h, int i ) {
    int add_on1 = 10;
    return add_on1 + a + foo2( b, c, d, e, f, g, h, i );
}

int dummymain() {
    int answer1 = foo1( 1, 2, 3, 4, 5, 6, 7, 8, 9 );
    printf("main: %d\n", answer1);
    return 0;
}

#pragma GCC pop_options