#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
  char* pass_argv[MAXARG];
  // arguments after pipe
  int i = 0;
  if (argc > 1) {
    for (; i < argc - 1; i++)
    {
      pass_argv[i] = (char*)malloc(sizeof(char) * (strlen(argv[i+1]) + 1));
      strcpy(pass_argv[i], argv[i+1]);
      // printf("pass_argv %s\n", pass_argv[i]);
    }
  } else {
    pass_argv[i] = (char*)malloc(5);
    strcpy(pass_argv[i++], "echo");
  }
  for (; i < MAXARG; i++)
  {
    int j = 0;
    char buf[512];
    while (read(0, buf + j, 1)) {
      if (*(buf + j) == '\n') {
        break;
      }
      j++;
    }
    if (j == 0) {
      break;
    }
    *(buf + j) = 0;
    pass_argv[i] = (char*)malloc(sizeof(char) * (strlen(buf) + 1));
    strcpy(pass_argv[i], buf);
    // printf("pass_argv %s\n", pass_argv[i]);
  }
  pass_argv[i--] = (char*)0;
  if (fork() == 0) {
    // printf("will exec %s\n", pass_argv[0]);
    exec(pass_argv[0], (char**)pass_argv);
    fprintf(2, "xargs: exec failed\n");
  } else {
    wait(0);
  }
  for (; i >= 0; i--)
  {
    free(pass_argv[i]);
  }
  exit(0);
}
