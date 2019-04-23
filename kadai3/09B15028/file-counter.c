#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/wait.h>
#define NUMPROCS 4

int count1(ct)
     FILE *ct;
{
  int count;
  
  if ((ct=fopen("counter", "r"))==NULL) exit(1);
  fscanf(ct, "%d\n", &count);
  count++;
  fclose(ct);
  if ((ct=fopen("counter", "w"))==NULL) exit(1);
  fprintf(ct, "%d\n", count);
  fclose(ct);
  return count;
}

int main()
{
  int i, count, pid, status;
  FILE *ct;
  int sid;
  key_t key;
  struct sembuf sb;
  
  setbuf(stdout, NULL); /* set stdout to be unbufferd */

  if ((key = ftok(".", 1)) == -1){
    fprintf(stderr,"ftok path does not exist.\n");
    exit(1);
  }
  if ((sid=semget(key, 1, 0666 | IPC_CREAT)) == -1) {
    perror("semget error.");
    exit(1);
  }
  sb.sem_num = 0;
  sb.sem_op  = 0;
  sb.sem_flg = 0;
  semctl(sid, 0, SETVAL, 1); /*獲得した集合の最初のセマフォの値を1に*/

  count = 0;
  if ((ct=fopen("counter", "w"))==NULL) exit(1);
  fprintf(ct, "%d\n", count);
  fclose(ct);
  for (i=0; i<NUMPROCS; i++) {
    sb.sem_op  = -1;
    if (semop(sid, &sb, 1) == -1) {
      perror("sem_wait semop error.");
      exit(1);
    }
    if ((pid=fork())== -1) {
      perror("fork failed.");
      exit(1);
    }
    if (pid == 0) { /* Child process */
      count = count1(ct);
      printf("count = %d\n", count);

      sb.sem_op  = 1;
      if (semop(sid, &sb, 1) == -1) {
	perror("sem_signal semop error.");
	exit(1);
      }
      exit(0);
    }
  }
  for (i=0; i<NUMPROCS; i++) {
    wait(&status);
  }
  semctl(sid, 0, IPC_RMID, 1);
}
