// cause stack relies on cause_sp 255 to be reserved for empty state
#define VM_EVENT_CAUSE_STACK_SIZE 4

enum vm_event_cause
{
    NONE, // none if the cause is not set yet
    FORK,
    EXEC,
    MALLOC,
    SBRK,
    PROCINIT,
    ALLOCPROC,
    PROCPGTBL,
    MAPPAGES
};

struct vm_event
{
    uint64 timestamp;
    uint64 range_start;
    uint64 range_end;
    int pid; // highest order bit in pid is indicating whether it's allocation (0) or deallocation (1)
    uint8 cause_stack[VM_EVENT_CAUSE_STACK_SIZE]; // (cause_number(s))
    uint8 cause_sp;
    uint8 valid;
    uint8 error; // has error occured during a 'vm transaction'
};