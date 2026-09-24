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

  if ((fd = open("vmtrace", O_RDWR)) < 0) { // create a file-device for UART1
    mknod("vmtrace", VMTRACE, 0);
    fd = open("vmtrace", O_RDWR);
  }

  printf("fd value: %d\n", fd);
  fprintf(fd, "%s", "Hello from the xv6 user process!\n");
}