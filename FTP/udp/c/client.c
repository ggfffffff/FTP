#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     /* defines STDIN_FILENO, system calls,etc */
#include <sys/types.h>  /* system data type definitions */
#include <sys/socket.h> /* socket specific definitions */
#include <netinet/in.h> /* INET constants and stuff */
#include <arpa/inet.h>  /* IP address conversion stuff */
#include <netdb.h>      /* gethostbyname */

#define MAXBUF 10*1024

int main() {
  int sk;
  struct sockaddr_in server;
  struct hostent *hp;
  char buf[MAXBUF];

  /* create a socket
     IP protocol family (PF_INET)
     UDP (SOCK_DGRAM)
  */

  if ((sk = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    printf("Problem creating socket\n");
    exit(1);
  }

  //指定服务器地址结构server中的地址族字段(address family)
  //AF_INET 代表IPv4地址族
  server.sin_family = AF_INET;
  //hp为指向主机信息的指针，获取"localhost"的主机信息
  hp = gethostbyname("localhost");

  /* copy the IP address into the sockaddr
     It is already in network byte order
  */
  //将从主机信息结构中获取的IP地址复制到 server.sin_addr.s_addr，以便后续的套接字操作能够使用这个IP地址来建立连接或发送数据
  //memcpy用于将内存中的数据从一个位置复制到另一个位置
  //参数：目标地址，源地址，要复制的字节数
  //服务器地址结构 server 中的成员 sin_addr.s_addr 的地址
  //h_addr 是主机信息结构中存储主机IP地址的成员
  memcpy(&server.sin_addr.s_addr, hp->h_addr, hp->h_length);

  /* establish the server port number - we must use network byte order! */
  //设置服务器的端口号为9876，并使用 htons 函数将其转换为网络字节序
  server.sin_port = htons(9876);

  for(int i=0;i<=50;i++)
  {
    //用于将消息格式化为带有数字的字符串，然后存储到 buf 缓冲区中
    sprintf(buf, "%d", i);
    //printf(buf, "%d");
    //printf("*\n");
    //计算数据的长度
    size_t buf_len = strlen(buf);
    /* send it to the echo server */
    //使用 sendto 函数将数据发送到服务器
    int sent_len = sendto(sk, buf, buf_len, 0,
                  (struct sockaddr*) &server, sizeof(server));

    //发送失败，程序将打印错误消息并退出
    if (sent_len < 0) {
      perror("Problem sending data");
      exit(1);
    }

    //检查实际发送的字节数是否与期望的字节数相匹配
    if (sent_len != buf_len) {
      printf("Sendto sent %d bytes\n", sent_len);
    }

    /* Wait for a reply (from anyone) */
    //使用 recvfrom 函数等待来自服务器的响应数据
    int n_read = recvfrom(sk, buf, MAXBUF, 0, NULL, NULL);
    //如果接收失败，程序将打印错误消息并退出
    if (n_read < 0) {
      perror("Problem in recvfrom");
      exit(1);
    }

    /* send what we got back to stdout */
    //将接收到的数据写入标准输出
    if (write(STDOUT_FILENO, buf, n_read) < 0) {
      perror("Problem writing to stdout");
      exit(1);
    }
  }
  
  close(sk);
  return 0;
}
