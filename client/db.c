#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define maxline_length 1024

int subst(char *str, char c1, char c2);
int split(char *str, char *ret[], char sep, int max);
void cmd_read(int sockfd, char *filename);
void cmd_write(int sockfd, char *filename);

int main(int argc, char* argv[])
{
  int PORT_NO, fd;
  FILE *fp;
  int sockfd, i, len, n, m, rec_num = 1, sen_num, count = 0;
  char *host;
  char buf[1024], message[1024], tmp[1024], line[1024];
  struct hostent *hp = NULL;
  struct sockaddr_in sa;

  char *ret[2] = {0};

  host = "localhost";
  hp = gethostbyname(host);

  PORT_NO = atoi(argv[1]);
  
  if (hp == NULL) {
    printf("host get error : %s\n", argv[1]);
    return 1;
  }

  memset((char *)&sa, 0, sizeof(sa));
  sa.sin_family = hp->h_addrtype; // host address type
  sa.sin_port = htons(PORT_NO); // port number
  bzero((char *) &sa.sin_addr, sizeof(sa.sin_addr));
  memcpy((char *)&sa.sin_addr, (char *)hp->h_addr, hp->h_length);
  
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("socket error\n");
    return -1;
  }
  if ((i = connect(sockfd, (struct sockaddr *)&sa, sizeof(sa))) < 0) {
    printf("connect error\n");
    return -1;
  }
  while(1) {
    memset((char *)&buf, '\0', sizeof(buf));
    memset((char *)&message, '\0', sizeof(message));
    printf(" > ");
    fflush(stdout);
    if ((len = read(0, message, 1024)) == -1) {
      printf("read error\n");
      return -1;
    }
    subst(message, '\n', '\0');

    if (*(message+1) == 'Q'){
      close(sockfd);
      break;
    } else if (*(message+1) == 'C') {
      if ((send(sockfd, message, len, 0)) == -1){
	printf("send error\n");
	close(sockfd);
	break;
      }
      printf("send message : %s\n", message);
      if ((recv(sockfd, buf, 1024, 0)) == -1) {
	printf("recv error\n");
	close(sockfd);
	break;
      }
      printf("recv message : %s\n", buf);
    } else if (*(message+1) == 'P') {
      if ((send(sockfd, message, len, 0)) == -1){
	printf("send error\n");
	close(sockfd);
	break;
      }
      printf("send message : %s\n", message);
      if ((recv(sockfd, buf, 1024, 0)) == -1) {
	printf("recv error\n");
	close(sockfd);
	break;
      }
    } else if (*(message+1) == 'R') {
      cmd_read(sockfd, message + 3);
    } else if (*(message+1) == 'W') {
      if ((send(sockfd, message, len, 0)) == -1){
	printf("send error\n");
	close(sockfd);
	break;
      }
      cmd_write(sockfd, message + 3);
    } else {
      if ((send(sockfd, message, len, 0)) == -1){
	printf("send error\n");
	close(sockfd);
	break;
      }
     printf("send message : %s\n", message);
     if (recv(sockfd, buf, sizeof(buf), 0) == -1) {
	printf("recv error\n");
	close(sockfd);
	break;
      }
     printf("recv message : %s\n", buf);
    }
    
    if (*buf == '#'){
      strcpy(tmp, buf);
      n = split(tmp + 1, ret, ' ', 2);
      rec_num = atoi(ret[1]);
      
      for (m = 0; m < rec_num; m++) {
	 memset((char *)&buf, '\0', sizeof(buf));
	 if ((recv(sockfd, buf, 1024, 0)) == -1) {
	   printf("recv error\n");
	   close(sockfd);
	   break;
	 }
	 printf("recv message : %s\n", buf);
      }
    }

  }
  close(sockfd);
  return 0;
}

int subst(char *str, char c1, char c2)
{
  int i=0;
  for(;*str != '\0'; str++ ){
    if(*str == c1){
      *str = c2;
      i++;
    }
  }
  return i;
}

int split(char *str, char *ret[], char sep, int max)
{
  int n = 0;

  ret[n] = str;
  while(*str != '\0' && n<max){
    if(*str == sep) {
      *str = '\0';
      n++;
      ret[n] = str + 1;
    }
    str++;
  }
  return n;
}

void cmd_read(int sockfd, char *filename)
{
  int fd, pos = 0, tmp;
  char buf[1];
  char message[1025];

  if((fd = open(filename, O_RDONLY)) == -1){
    printf("open error\n");
    return;
  }

  while(1) { 
    if ((tmp = read(fd, buf, 1)) == -1) {
      printf("cmd_read read error\n");
      return;
    } else if (tmp == 0) break;

    if (buf[0] == '\n') {
      if (send(sockfd, message, pos, 0) == -1) {
	printf("send error\n");
	return;
      }
      if (recv(sockfd, message, pos, 0) == -1) {
	printf("recv error\n");
	return;
      }
      message[2] = '\0';
      pos = 0;
    } else {
      message[pos] = buf[0];
      pos++;
    }
  }
  close(fd);
}
void cmd_write(int sockfd, char *filename)
{
  char buf[1024];
  char *end_msg;
  int len, fd, end_flg = 0;
  if((fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU)) == -1){
    printf("open error\n");
    return;
  }
  
  while(!end_flg) {
    if ((len = recv(sockfd, buf, 1024, 0)) == -1) {
      printf("recv error\n");
      break;
    }
    if ((end_msg = strchr(buf, '\a')) != NULL) {
      end_flg = 1;
      *end_msg = '\0';
      len = end_msg - buf;
    }
    if(write(fd, buf, len) == -1){
      printf("write error\n");
      break;
    }
  }
  close(fd);
}
