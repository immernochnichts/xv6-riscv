enum vm_event_type
{
    VME_ALLOC,
    VME_DEALLOC
};

struct vm_event
{
    uint32 subject_pid;
    uint32 type;
    uint64 range_start;
    uint64 range_end;
    char region_name[16];
    char subject_func[16];
};