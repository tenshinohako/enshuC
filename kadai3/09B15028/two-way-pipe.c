#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#define BUFSIZE 256

int main(int argc, char *argv[])
{
  char bufP[BUFSIZE], bufC[BUFSIZE];
  int fdCTP[2], fdPTC[2];
  int pid, msglenP, msglenC, status;
  if (argc != 3) {
    printf("bad argument.\n");
    exit(1);
  }
  if (pipe(fdCTP) == -1) {
    perror("pipe(Child to Parent) failed.");
    exit(1);
  }
  if (pipe(fdPTC) == -1) {
    perror("pipe(Parent to Child) failed.");
    exit(1);
  }
  if ((pid=fork())== -1) {
    perror("fork failed.");
    exit(1);
  }
  if (pid == 0) { /* Child process */
    close(fdPTC[1]);
    close(fdCTP[0]);
    msglenC = strlen(argv[1]) + 1;
    if (write(fdCTP[1], argv[1], msglenC) == -1) {
      perror("pipe write.(Child)");
      exit(1);
    }
    if (read(fdPTC[0], bufC, BUFSIZE) == -1) {
      perror("pipe read.(Child)");
      exit(1);
    }
    printf("Message from parent process: \n");
    printf("\t%s\n",bufC);
    exit(0);
  } else { /* Parent process */
    close(fdPTC[0]);
    close(fdCTP[1]);
    msglenP = strlen(argv[2]) + 1;
    if (write(fdPTC[1], argv[2], msglenP) == -1) {
      perror("pipe write.(Parent)");
      exit(1);
    }
    if (read(fdCTP[0], bufP, BUFSIZE) == -1) {
      perror("pipe read.(Parent)");
      exit(1);
    }
    printf("Message from child process: \n");
    printf("\t%s\n",bufP);
    wait(&status);
  }
}
