#ifndef RING_BUF_H
#define RING_BUF_H

struct ring_buf
{
    uint8_t* buf;
    uint8_t size;
    uint8_t head;
    uint8_t tail;
};

void ring_buf_putchar(struct ring_buf* b, uint8_t c);

uint8_t ring_buf_getchar(struct ring_buf* b);

// 1 if empty, 0 if not
uint8_t ring_buf_is_empty(struct ring_buf* b);

// 1 if full, 0 if not
uint8_t ring_buf_is_full(struct ring_buf* b);

#endif