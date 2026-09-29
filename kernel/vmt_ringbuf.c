#include "types.h"
#include "spinlock.h"
#include "vm_event.h"
#include "vmt_ringbuf.h"


void ring_buf_putevent(struct ring_buf* b, struct vm_event* e)
{
    b->buf[b->head] = *e;
    b->head++;

    if (b->head == b->size)
    {
        b->head = 0;
    }
}

void ring_buf_getevent(struct ring_buf* b, struct vm_event* e)
{
    struct vm_event ev = b->buf[b->tail];
    b->tail++;

    if (b->tail == b->size)
    {
        b->tail = 0;
    }

    *e = ev;
}

int ring_buf_is_empty(struct ring_buf* b)
{
    return b->head == b->tail;
}

// one slot is reserved, so the actual buffer size is 1 less
int ring_buf_is_full(struct ring_buf* b)
{
    uint8 next_head = b->head + 1;

    if (next_head == b->size)
    {
        next_head = 0;
    }

    return next_head == b->tail;
}
