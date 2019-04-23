#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#define MAXCLIENTS 5

int main(int argc,char **argv) {
  int sock,csock[MAXCLIENTS], asock;
  struct sockaddr_in svr;
  struct sockaddr_in clt;
  int clen;
  char buf[1024];
  char sendbuf[1024];
  char namebuf[100];
  char username[MAXCLIENTS][100];
  int reuse;
  int k = 0;
  int maxfd;
  int nbytes;

  fd_set rfds, srfds;/* select()で用いるファイル記述子集合*/
  struct timeval tv;   /* select()が返ってくるまでの待ち時間を指定する変数*/

  //state1
  /*ソケットの生成*/
  if ((sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0) {
    perror("socket");
    exit(1);
  }
  /*ソケットアドレス再利用の指定*/
  reuse=1;
  if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse))<0) {
    perror("setsockopt");
    exit(1);
  }
  /* client受付用ソケットの情報設定*/
  bzero(&svr,sizeof(svr));
  svr.sin_family=AF_INET;
  svr.sin_addr.s_addr=htonl(INADDR_ANY);
  /*受付側のIPアドレスは任意*/
  svr.sin_port=htons(10140);  /*ポート番号10140を介して受け付ける*/
  /*ソケットにソケットアドレスを割り当てる*/
  if(bind(sock,(struct sockaddr *)&svr,sizeof(svr))<0) {
    perror("bind");
    exit(1);
  }
  /*待ち受けクライアント数の設定*/
  if (listen(sock,MAXCLIENTS)<0) {  /*待ち受け数にを指定*/
    perror("listen");
    exit(1);
  }
  
  for(int i=0; i<MAXCLIENTS; i++){
    csock[i] = -1;
  }

  while(1){
    //state2
    FD_ZERO(&rfds);/* rfdsを空集合に初期化*/
    FD_SET(sock,&rfds);  /*クライアントを受け付けたソケット*/
    for(int i=0; i<MAXCLIENTS; i++){
      if(csock[i] != -1){
	FD_SET(csock[i],&rfds);
      }
    }

    /*監視する待ち時間を1秒に設定*/
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    maxfd = sock;
    for(int i=0; i<MAXCLIENTS; i++){
      if(maxfd < csock[i]){
	maxfd = csock[i];
      }
    }

    //state3
    if(select(maxfd+1,&rfds,NULL,NULL,&tv)>0) {
      if(FD_ISSET(sock,&rfds)){
	//state4
	clen = sizeof(clt);
	if((asock = accept(sock,(struct sockaddr *)&clt,&clen) ) <0 ) {
	   perror("accept");
	   exit(2);
	}
	if(k < MAXCLIENTS){
	  write(asock, "REQUEST ACCEPTED\n", 17);
	  csock[k] = asock;
	}else{
	  write(asock, "REQUEST REJECTED\n", 17);
	  close(asock);
	  write(1, "rejected\n", 9);
	  continue;
	}
	//state5
	read(csock[k], namebuf, sizeof(namebuf));
	
	for(int i=0; ; i++){
	  if(namebuf[i] == '\n'){
	    namebuf[i] = '\0';
	    break;
	  }
	}

	bool flag = false;
	for(int i=0; namebuf[i]!='\0'; i++){
	  if(!isalnum(namebuf[i])){
	    if(namebuf[i] != '-' && namebuf[i] != '_'){
	      write(csock[k], "USERNAME REJECTED\n", 18);
	      write(1, "rejected : ", 11);
	      write(1, namebuf, strlen(namebuf));
	      write(1, "\n", 1);
	      close (csock[k]);
	      csock[k] = -1;
	      flag = true;
	      break;
	    }
	  }
	}

	if(flag){
	  continue;
	}
	
	for(int i=0; i<k; i++){
	  if(strcmp(namebuf, username[i]) == 0){
	    write(csock[k], "USERNAME REJECTED\n", 18);
	    write(1, "rejected : ", 11);
	    write(1, namebuf, strlen(namebuf));
	    write(1, "\n", 1);
	    close (csock[k]);
	    csock[k] = -1;
	    flag = true;
	    break;
	  }
	}
	if(flag){
	  continue;
	}
	strcpy(username[k], namebuf);
	write(csock[k], "USERNAME REGISTERED\n", 20);
	write(1, "accepted : ", 11);
	write(1, namebuf, strlen(namebuf));
	write(1, "\n", 1);
	k++;
      }
      for(int i=0; i<k; i++){
	if(FD_ISSET(csock[i],&rfds)){
	  //state6
	  nbytes = read(csock[i], buf, sizeof(buf));
	  for(int j=0; ; j++){
	    if(buf[j] == '\n'){
	      buf[j+1] = '\0';
	      break;
	    }
	  }
	  if(nbytes == 0){
	    //state7
	    close(csock[i]);
	    write(1, "disconnected : ", 15);
	    write(1, username[i], strlen(username[i]));
	    write(1, "\n", 1);
	    for(int j=0; j<k-i; j++){
	      csock[i+j] = csock[i+1+j];
	      strcpy(username[i+j], username[i+1+j]);
	    }
	    k--;
	    csock[k] = -1;	    
	  }else{
	    if(!strcmp(buf, "/list\n")){
	      write(csock[i], "-----user list-----\n", 20);
	      for(int j=0; j<k; j++){
		write(csock[i], username[j], strlen(username[j]));
		write(csock[i], "\n", 1);
	      }
	      write(csock[i], "-------------------\n", 20);
	    }else if(!strncmp(buf, "/send ", 6)){
	      int j=6;
	      int m=0;
	      while(buf[j] == ' '){
		j++;
	      }
	      while(buf[j] != ' '){
		namebuf[m] = buf[j];
		j++;
		m++;
	      }
	      while(buf[j] == ' '){
		j++;
	      }
	      namebuf[m] = '\0';
	      m = j;
	      while(buf[m] != '\n'){
		m++;
	      }
	      buf[m] = '\0';
	      for(int n=0; n<k; n++){
		if(!strcmp(namebuf, username[n])){
		  strcpy(sendbuf, username[i]);
		  strcat(sendbuf, " >");
		  strcat(sendbuf, &buf[j]);
		  strcat(sendbuf, " > ");
		  strcat(sendbuf, username[n]);
		  strcat(sendbuf, "\n");
		  write(csock[n], sendbuf, strlen(sendbuf));
		}
	      }
	    }else{
	      strcpy(sendbuf, username[i]);
	      strcat(sendbuf, " >");
	      strcat(sendbuf, buf);
	      for(int j=0; j<k; j++){
		write(csock[j], sendbuf, strlen(sendbuf));
	      }
	    }
	  }
	}
      }
    }
  }
}
