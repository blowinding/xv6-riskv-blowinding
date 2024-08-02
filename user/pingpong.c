#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

const char* byte = "b";

int main(int argc, char *argv[])
{
  // create pipe
  int p[2];
  if (pipe(p) != 0) {
    fprintf(2, "pingpong: pipe create failed");
    exit(1);
  }
  // fork
  if (fork() != 0) {
    // parent
    close(p[1]);
    char buf;
    read(p[0], (void*)&buf, 1);
    fprintf(1, "%d: received pong\n", getpid());
    close(p[0]);
  } else {
    // child
    close(p[0]);
    fprintf(1, "%d: received ping\n", getpid());
    write(p[1], byte, 1);
    close(p[1]);
  }
  exit(0);
}
