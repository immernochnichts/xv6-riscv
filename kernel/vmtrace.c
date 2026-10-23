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
#include "vmtrace.h"
#include "vmt_ringbuf.h"

static struct vm_event events[VMTBUFSZ];
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
  vmebuf.size = VMTBUFSZ;
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

void vmtrace_printeventk(struct vm_event* e)
{
  char cause_string[96];
  int cause_string_idx = 0;

  printk("\n--vm_event--\ntstamp:%lu\nrange_start:0x%lx\nrange_end:0x%lx\npid:%d\n", e->timestamp, e->range_start, e->range_end, e->pid);

  for (uint8 i = 0; i < e->cause_sp; i++)
  {
    switch (e->cause_stack[i])
    {
    case NONE:
      strncpy(&cause_string[cause_string_idx], "NONE", 4);
      cause_string_idx += 4;
      break;
    case FORK:
      strncpy(&cause_string[cause_string_idx], "FORK", 4);
      cause_string_idx += 4;
      break;
    case EXEC:
      strncpy(&cause_string[cause_string_idx], "EXEC", 4);
      cause_string_idx += 4;
      break;
    case MALLOC:
      strncpy(&cause_string[cause_string_idx], "MALLOC", 6);
      cause_string_idx += 6;
      break;
    case SBRK:
      strncpy(&cause_string[cause_string_idx], "SBRK", 4);
      cause_string_idx += 4;
      break;
    case PROCINIT:
      strncpy(&cause_string[cause_string_idx], "PROCINIT", 8);
      cause_string_idx += 8;
      break;
    case ALLOCPROC:
      strncpy(&cause_string[cause_string_idx], "ALLOCPROC", 9);
      cause_string_idx += 9;
      break;
    case PROCPGTBL:
      strncpy(&cause_string[cause_string_idx], "PROCPGTBL", 9);
      cause_string_idx += 9;
      break;
    case MAPPAGES:
      strncpy(&cause_string[cause_string_idx], "MAPPAGES", 8);
      cause_string_idx += 8;
      break;
    default:
      break;
    }

    if (i < e->cause_sp - 1) {
      strncpy(&cause_string[cause_string_idx], " -> ", 4);
      cause_string_idx += 4;
    }
  }
  cause_string[cause_string_idx] = '\0';
  
  printk("cause_sp:%d\ncause_stack:%s\nvalid:%d\nerror:%d\n\n", e->cause_sp, cause_string, e->valid, e->error);
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

// no process locks must be held when
// calling this function
// send event to vmebuf
// sets event's valid flag to 0
// 0 on success, -1 on failure
int vmtrace_pushevent(struct vm_event* e, int wkup)
{
  #ifndef VMTRACEENABLE
  return -1;
  #endif

  acquire(&vmebuf.lock);
  if (ring_buf_is_full(&vmebuf))
  {
    vmebuf.events_lost++;
    //int t = vmebuf.events_lost;
    release(&vmebuf.lock);
    //printk("vmebuf full; events lost: %d\n", t);
    e->valid = 0;
    return -1;
  }

  ring_buf_putevent(&vmebuf, e);
  release(&vmebuf.lock); // accidentally deleted this line so kernel crashed

  if (wkup)
    wakeup(vmebuf_chan);

  e->valid = 0;
  return 0;
}

// void vmtrace_alloc(int pid, uint64 start, uint64 end, uint8 cause, int wkup)
// {
//   #ifndef VMTRACEENABLE
//   return;
//   #endif

//   struct vm_event e;
//   e.cause = cause; // VME_ALLOC
//   e.pid = pid;
//   e.range_start = start;
//   e.range_end = end;

//   vmtrace_pushevent(&e, wkup);
// }
// void vmtrace_dealloc(int pid, uint64 start, uint64 end, uint8 cause, int wkup)
// {
//   #ifndef VMTRACEENABLE
//   return;
//   #endif

//   struct vm_event e;
//   e.cause = cause;
//   e.pid = pid & 0x80000000;
//   e.range_start = start;
//   e.range_end = end;

//   vmtrace_pushevent(&e, wkup);
// }

// return 0 if successfull -1 if stack full

int vmtrace_causestackempty(struct vm_event* e)
{
  return e->cause_sp == 0;
}

// returns 1 if successful otherwise 0
int vmtrace_pushcause(struct vm_event* e, uint8 cause)
{
  if (e->cause_sp < VM_EVENT_CAUSE_STACK_SIZE) {
    e->cause_stack[e->cause_sp] = cause;
    e->cause_sp++;
    return 1;
  }
  
  return 0;
}

// returns NONE if stack's empty
uint8 vmtrace_popcause(struct vm_event* e)
{
  if (vmtrace_causestackempty(e)) {
    return NONE;
  }

  e->cause_sp--;
  uint8 cause = e->cause_stack[e->cause_sp];
  return cause;
}

// returns NONE if stack's empty
uint8 vmtrace_peekcause(struct vm_event* e)
{
  if (vmtrace_causestackempty(e)) {
    return NONE;
  }

  return e->cause_stack[e->cause_sp - 1];
}

int vmtrace_causestackcontains(struct vm_event *e, uint8 cause)
{
  for (uint8 i = 0; i < e->cause_sp; i++) {
    if (e->cause_stack[i] == cause) {
      return 1;
    }
  }
  
  return 0;
}