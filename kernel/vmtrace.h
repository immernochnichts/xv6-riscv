#include "types.h"

struct vm_event
{
    vm_event_type type;
    int subject_pid;
    uint64 range_start;
    uint64 range_end;
};

enum vm_event_type : uint8
{
    VME_PROCESS_CREATED,
    VME_PROCESS_KILLED,
    VME_ALLOC,
    VME_DEALLOC
};