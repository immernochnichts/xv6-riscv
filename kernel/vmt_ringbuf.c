#include "ring_buf.h"

void ring_buf_putchar(struct ring_buf* b, uint8_t c)
{
    b->buf[b->head] = c;
    b->head++;

    if (b->head == b->size)
    {
        b->head = 0;
    }
}

uint8_t ring_buf_getchar(struct ring_buf* b)
{
    uint8_t c = b->buf[b->tail];
    b->tail++;

    if (b->tail == b->size)
    {
        b->tail = 0;
    }

    return c;
}

uint8_t ring_buf_is_empty(struct ring_buf* b)
{
    return b->head == b->tail;
}

uint8_t ring_buf_is_full(struct ring_buf* b)
{
    uint8_t next_head = b->head + 1;

    if (next_head == b->size)
    {
        next_head = 0;
    }

    return next_head == b->tail;
}
