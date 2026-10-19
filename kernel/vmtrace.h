#define VMTBUFSZ 16

int         vmtrace_isbufempty(void);
int         vmtrace_pushevent(struct vm_event* e, int wakeup);
int         vmtrace_popevent(struct vm_event* e);
// void        vmtrace_alloc(int pid, uint64 start, uint64 end, uint8 cause, int wkup);
// void        vmtrace_dealloc(int pid, uint64 start, uint64 end, uint8 cause, int wkup);
void        vmtrace_printeventk(struct vm_event* e);
int         vmtrace_pushcause(struct vm_event* e, uint8 cause);
uint8       vmtrace_popcause(struct vm_event* e);
uint8       vmtrace_peekcause(struct vm_event* e);
int         vmtrace_causestackempty(struct vm_event* e);
int         vmtrace_causestackcontains(struct vm_event *e, uint8 cause);