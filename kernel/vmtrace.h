int         vmtrace_isbufempty(void);
void        vmtrace_pushevent(struct vm_event* e);
void        vmtrace_alloc(int pid, uint64 start, uint64 end, char region_name[16], char func_name[16]);
void        vmtrace_dealloc(int pid, uint64 start, uint64 end, char region_name[16], char func_name[16]);