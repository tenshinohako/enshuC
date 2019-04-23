#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#define PORT 10130
#define TIMEOUT 10

volatile sig_atomic_t flag = 0;

void myalarm(int sec) {
  //alarm(sec);

  static int pid;
  int p_pid;

  if(pid > 0){
    kill(pid, SIGTERM);
  }

  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = SA_NOCLDWAIT;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    exit(1);
  }
  
  if ((pid=fork())== -1) {
    perror("fork failed.");
    exit(1);
  }

  if (pid == 0) { /* Child proces */
    p_pid = getppid();
    sleep(sec);
    kill(p_pid, SIGALRM);
    exit(0);
  } else {
    
  }
  
}

void timeout()
{
  flag = 1;
}

int main(int argc,char **argv)
{
  int sock;
  int nbytes;
  char buf[1024];
  char returnbuf[1024];
  struct sockaddr_in host;
  struct hostent *hp;

  fd_set rfds;/* select()で用いるファイル記述子集合*/
  struct timeval tv;   /* select()が返ってくるまでの待ち時間を指定する変数*/
  
  if (argc != 2) {
    fprintf(stderr,"Usage: %s hostname\n",argv[0]);
    exit(1);
  }
  /*ソケットの生成*/
  if ((sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0) {
    perror("socket");
    exit(1);
  }
  /* host(ソケットの接続先)の情報設定*/
  bzero(&host,sizeof(host));
  host.sin_family=AF_INET;
  host.sin_port=htons(PORT);
  if ( ( hp = gethostbyname(argv[1]) ) == NULL ) {
    fprintf(stderr,"unknown host %s\n",argv[1]);
    exit(1);
  }
  bcopy(hp->h_addr,&host.sin_addr,hp->h_length);
  /*hostと接続*/
  if (connect(sock,(struct sockaddr*)&host,sizeof(host))<0) {
    perror("connect");
    exit(1);
  }else{
    printf("connected\n");
  }

  if(signal(SIGALRM,timeout) == SIG_ERR) {
    perror("signal failed.");
    exit(1);
  }

  myalarm(TIMEOUT);
  do{
    /*入力を監視するファイル記述子の集合を変数rfdsにセットする*/
    FD_ZERO(&rfds);/* rfdsを空集合に初期化*/
    FD_SET(0,&rfds);    /*標準入力*/
    FD_SET(sock,&rfds);  /*クライアントを受け付けたソケット*/
    /*監視する待ち時間を1秒に設定*/
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    if(flag){
      close(sock);
      printf("This program is timeout.\n");
      exit(0);
    }

    /*準入力とソケットからの受信を同時に監視する*/
    if(select(sock+1,&rfds,NULL,NULL,&tv)>0) {
      myalarm(TIMEOUT);
      if(FD_ISSET(0,&rfds)) {  /*標準入力から入力があったなら*/
	nbytes=read(0,buf,sizeof(buf));
	if(nbytes == 0){
	  printf("\nclose\n");
	  break;
	}else{
	  /*標準入力から読み込みクライアントに送信*/
	  write(sock,buf,nbytes);	
	}
      }
      if(FD_ISSET(sock,&rfds)) { /*ソケットから受信したなら*/
	/*ソケットから読み込み端末に出力*/
	nbytes=read(sock,buf,sizeof(buf));
	if(nbytes == 0){
	  printf("\nserver closed\n");
	  break;
	}else{
	  write(1,argv[1],sizeof(argv[1])-1);
	  write(1," : ",3);
	  write(1,buf,nbytes);
	}
      }
    }
  }while(1);
  close(sock);
}
