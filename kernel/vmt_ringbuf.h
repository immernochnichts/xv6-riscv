struct ring_buf
{
    struct spinlock lock;
    struct vm_event* buf;
    int events_lost;
    uint8 size;
    uint8 head;
    uint8 tail;
};

void ring_buf_putevent(struct ring_buf* b, struct vm_event* e);

void ring_buf_getevent(struct ring_buf* b, struct vm_event* e);

// 1 if empty, 0 if not
int ring_buf_is_empty(struct ring_buf* b);

// 1 if full, 0 if not
int ring_buf_is_full(struct ring_buf* b);
