#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

struct data
{
    char type;
    uint64 address;
};

int
main(void)
{
  struct data f = { 'X', 0x6161616161616161 };
  int s = sizeof(struct data);
  printf("size of struct data: %d\n", s);
  write(1, &f, s);
}