#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
  // check whether an argument exists
  if (argc <= 1) {
    fprintf(2, "sleep: missing operand\n");
    exit(1);
  }
  // call sleep syscall
  for (int i = 1; i < argc; i++)
  {
    int sleep_time = atoi(argv[i]);
    sleep(sleep_time);
  }
  // exit
  exit(0);
}
