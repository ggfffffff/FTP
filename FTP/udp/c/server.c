#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>     /* defines STDIN_FILENO, system calls,etc */
#include <sys/types.h>  /* system data type definitions */
#include <sys/socket.h> /* socket specific definitions */
#include <netinet/in.h> /* INET constants and stuff */
#include <arpa/inet.h>  /* IP address conversion stuff */
#include <netdb.h>      /* gethostbyname */
#include <cstring>

void uppercase(char *p) {
  for ( ; *p; ++p) *p = toupper(*p);
}
//接受套接字描述符sd(已经创建并绑定到指定地址的UDP套接字)，用于处理数据的接收和回送
void echo(int sd) {
    //创建缓冲区bufin 用于存储接收到的数据
    char buf_input[ 1024*1024];
    char buf_output[ 1024*1024];
    struct sockaddr_in remote;
    int buf_length;

    /* need to know how big address struct is, len must be set before the
       call to recvfrom!!! */
    socklen_t len = sizeof(remote);

    int sequence_number = 0;  // 初始化序列号为0
    while (1) {
      /* read a datagram from the socket (put result in bufin) */
      //使用 recvfrom 函数接收数据，将其存储在 bufin 中，同时记录发送者的地址信息
      int n = recvfrom(sd, buf_input, MAXBUF, 0, (struct sockaddr *) &remote, &len);
      
      if (n < 0) {
        perror("Error receiving data");
      } else {
        sequence_number++;//增加序列号
        
        buf_length=snprintf(buf_output,sizeof(buf_input), "%d %s\n", sequence_number,buf_input);//更改字符串
        
        uppercase(buf_output);
        /* Got something, just send it back */
        //使用 sendto 函数将转换后的数据回送给发送者
        sendto(sd, buf_output, buf_length, 0, (struct sockaddr *)&remote, len);
      }
    }
}

/* server main routine */

int main() {
  int ld;
  struct sockaddr_in skaddr;
  socklen_t length;

  /* create a socket
     IP protocol family (PF_INET)
     UDP protocol (SOCK_DGRAM)
  */

  if ((ld = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    printf("Problem creating socket\n");
    exit(1);
  }

  /* establish our address
     address family is AF_INET
     our IP address is INADDR_ANY (any of our IP addresses)
     the port number is 9876
  */
  //设置服务器的地址信息并绑定到套接字
  skaddr.sin_family = AF_INET;
  skaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  skaddr.sin_port = htons(9876);
  //使用 bind 函数将套接字绑定到指定的地址和端口
  if (bind(ld, (struct sockaddr *) &skaddr, sizeof(skaddr)) < 0) {
    printf("Problem binding\n");
    exit(0);
  }

  /* find out what port we were assigned and print it out */
  //获取服务器套接字的绑定信息，并将这些信息存储在 skaddr 结构中
  length = sizeof(skaddr);
  if (getsockname(ld, (struct sockaddr *) &skaddr, &length) < 0) {
    printf("Error getsockname\n");
    exit(1);
  }

  /* Go echo every datagram we get */
  echo(ld);
  return 0;
}
