#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void prime(int fd) {
  int target;
  if (read(fd, &target, sizeof(int)) == 0) {
    exit(0);
  }
  printf("prime %d\n", target);
  int p[2];
  if (pipe(p) != 0) {
    fprintf(2, "prime: pipe create failed");
    exit(1);
  }
  if (fork() == 0) {
    close(p[1]);
    prime(p[0]);
  } else {
    close(p[0]);
    int n;
    while (read(fd, &n, sizeof(int))) {
      if (n % target != 0) {
        write(p[1], &n, sizeof(int));
      }
    }
    close(p[1]);
    wait(0);
  }
}

int main(int argc, char *argv[])
{
  int p[2];
  if (pipe(p) != 0) {
    fprintf(2, "prime: pipe create failed");
    exit(1);
  }
  if (fork() == 0) {
    close(p[1]);
    prime(p[0]);
  } else {
    // parent
    close(p[0]);
    printf("prime 2\n");
    for (int i = 2; i <= 35; i++)
    {
      if (i % 2 != 0) {
        write(p[1], &i, sizeof(int));
      }
    }
    close(p[1]);
    wait(0);
  }
  exit(0);
}
