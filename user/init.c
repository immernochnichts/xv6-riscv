// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "user/user.h"
#include "kernel/fcntl.h"

char *argv[] = {"sh", 0};
char *argv_vmt [] = { "vmtracetest" };

int
start_sh(void)
{
  printf("init: starting sh\n");

  int pid_sh = fork();
  if (pid_sh < 0) {
    printf("init: fork failed\n");
    exit(1);
  }
  if (pid_sh == 0) {
    exec("sh", argv);
    printf("init: exec sh failed\n");
    exit(1);
  }

  return pid_sh;
}

int
start_vmtrace(void)
{
  printf("init: starting vmtrace\n");

  int pid_vmtrace = fork();
  if (pid_vmtrace < 0) {
    printf("init: fork failed\n");
    exit(1);
  }
  if (pid_vmtrace == 0) {
    exec("vmtracetest", argv_vmt);
    printf("init: exec vmtracetest failed\n");
    exit(1);
  }

  return pid_vmtrace;
}

int
main(void)
{
  int pid_sh, wpid;

  if (open("console", O_RDWR) < 0) {
    mknod("console", CONSOLE, 0);
    open("console", O_RDWR);
  }
  dup(0); // stdout
  dup(0); // stderr

  for (;;) {
    pid_sh = start_sh();
    #ifdef VMTRACEENABLE
    int pid_vmtrace = start_vmtrace();
    #endif

    for (;;) {
      // this call to wait() returns if the shell exits,
      // or if a parentless process exits.
      wpid = wait((int *)0);
      if (wpid == pid_sh) {
        // the shell exited; restart it.
        start_sh();
      }
      #ifdef VMTRACEENABLE
      else if (wpid == pid_vmtrace) {
        // vmtrace exited 
      }
      #endif
      else if (wpid < 0) {
        printf("init: wait returned an error\n");
        exit(1);
      } else {
        // it was a parentless process; do nothing.
      }
    }
  }
}
