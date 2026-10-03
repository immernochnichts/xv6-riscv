#define VMTBUFSZ 16

int         vmtrace_isbufempty(void);
int         vmtrace_pushevent(struct vm_event* e);
int         vmtrace_popevent(struct vm_event* e);
void        vmtrace_alloc(int pid, uint64 start, uint64 end, const char* region_name, const char* func_name);
void        vmtrace_dealloc(int pid, uint64 start, uint64 end, const char* region_name, const char* func_name);