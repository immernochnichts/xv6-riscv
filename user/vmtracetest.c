#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int
main(void)
{
  int fd;

  if ((fd = open("vmtrace", O_WRONLY)) < 0) { // create a file-device for UART1
    mknod("vmtrace", VMTRACE, 0);
    fd = open("vmtrace", O_WRONLY);
  }

  // the logic:
  // check if there's events to send
  // if none, go sleep
  // if there are, acquire the lock on the buf and send them all and go back to sleep

  while (1) {
    
  }
}