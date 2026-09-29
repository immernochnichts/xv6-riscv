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
#include "vm_event.h"
#include "vmtrace.h"
#include "vmt_ringbuf.h"

static struct vm_event events[128];
static struct ring_buf vmebuf;
void* vmebuf_chan;

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

  vmebuf.buf = events;
  vmebuf.size = 128;
  vmebuf.head = 0;
  vmebuf.tail = 0;
  initlock(&vmebuf.lock, "vmebuf");

  vmebuf_chan = &vmebuf;
}

int vmtrace_isbufempty()
{
  acquire(&vmebuf.lock);
  int f = ring_buf_is_empty(&vmebuf);
  release(&vmebuf.lock);
  return f;
}

// 0 on success, -1 on failure
int vmtrace_popevent(struct vm_event* e)
{
  #ifndef VMTRACEENABLE
  return -1;
  #endif

  acquire(&vmebuf.lock);
  if (ring_buf_is_empty(&vmebuf))
  {
    release(&vmebuf.lock);
    return -1;
  }

  ring_buf_getevent(&vmebuf, e);
  release(&vmebuf.lock);
  return 0;
}

// 0 on success, -1 on failure
int vmtrace_pushevent(struct vm_event* e)
{
  #ifndef VMTRACEENABLE
  return -1;
  #endif

  acquire(&vmebuf.lock);
  if (ring_buf_is_full(&vmebuf))
  {
    vmebuf.events_lost++;
    release(&vmebuf.lock);
    printk("vmebuf is full\n");
    return -1;
  }

  ring_buf_putevent(&vmebuf, e);
  release(&vmebuf.lock);

  wakeup(vmebuf_chan);
  return 0;
}

void vmtrace_alloc(int pid, uint64 start, uint64 end, const char* region_name, const char* func_name)
{
  #ifndef VMTRACEENABLE
  return;
  #endif

  struct vm_event e;
  e.type = 0; // VME_ALLOC
  e.subject_pid = pid;
  e.range_start = start;
  e.range_end = end;

  int i = 0;
  while (i < 16 && region_name[i] != '\0') {
    e.region_name[i] = region_name[i];
    i++;
  }

  i = 0;
  while (i < 16 && func_name[i] != '\0') {
    e.subject_func[i] = func_name[i];
    i++;
  }

  vmtrace_pushevent(&e);
}
void vmtrace_dealloc(int pid, uint64 start, uint64 end, const char* region_name, const char* func_name)
{
  #ifndef VMTRACEENABLE
  return;
  #endif

  struct vm_event e;
  e.type = 1; // VME_DEALLOC
  e.subject_pid = pid;
  e.range_start = start;
  e.range_end = end;

  int i = 0;
  while (i < 16 && region_name[i] != '\0' && i < 16) {
    e.region_name[i] = region_name[i];
    i++;
  }

  i = 0;
  while (i < 16 && func_name[i] != '\0') {
    e.subject_func[i] = func_name[i];
    i++;
  }

  vmtrace_pushevent(&e);
}