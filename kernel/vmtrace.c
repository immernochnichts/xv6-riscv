#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"


// override the write() syscall to go here on major device 2
int
vmtracewrite(int user_src, uint64 src, int n)
{
  char buf[32]; // move batches from user space to uart.
  int i = 0;

  while (i < n) {
    int nn = sizeof(buf);
    if (nn > n - i)
      nn = n - i;
    if (either_copyin(buf, user_src, src + i, nn) == -1)
      break;
    uartwrite_x(buf, nn, 1);
    i += nn;
  }

  return i;
}

void
vmtraceinit(void) // uartinit is called in consoleinit (console.c)
{
    devsw[VMTRACE].write = vmtracewrite;
}
