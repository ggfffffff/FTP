#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	int listenfd, connfd; // 监听socket和连接socket不一样，后者用于数据传输
	struct sockaddr_in addr;
	char sentence[8192];
	int p;
	int len;

	int is_login = 0; // 标记是否已登录
	int port = 21;
	char root[50] = "/tmp"; // 默认服务器的根目录

	// 解析是否有指定port和root参数
	for (int i = 0; i < argc - 1; i++)
	{
		if (strcmp(argv[i], "-port") == 0)
		{ // 有参数port
			port = atoi(argv[i + 1]);
		}
		if (strcmp(argv[i], "-root") == 0) // 有参数root
		{
			strcpy(root, argv[i + 1]);
			chdir(root); 
		}
	}
	// 创建socket
	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	// 设置本机的ip和port
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = 6789;
	addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听"0.0.0.0"

	// 将本机的ip和port与socket绑定
	if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	// 开始监听socket
	if (listen(listenfd, 10) == -1)
	{
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	// 持续监听连接请求
	while (1)
	{
		// 等待client的连接 -- 阻塞函数
		if ((connfd = accept(listenfd, NULL, NULL)) == -1)
		{
			printf("Error accept(): %s(%d)\n", strerror(errno), errno);
			continue;
		}

		// 发送初始欢迎消息
		// 36是消息的字符数（包括终止的\0字符）
		write(connfd, "220 ftp.ssast.org FTP server ready.\r\n", 36);
		printf("SERVER: 220 ftp.ssast.org FTP server ready.\r\n");

		// 榨干socket传来的内容
		p = 0;
		while (1)
		{
			int n = read(connfd, sentence + p, 8191 - p);
			printf("n=%d\n", n);
			if (n < 0)
			{
				printf("Error read(): %s(%d)\n", strerror(errno), errno);
				close(connfd);
				continue;
			}
			else if (n == 0)
			{
				break;
			}
			else
			{
				p += n;
				if (sentence[p - 1] == '\n')
				{
					break;
				}
			}
		}
		// socket接收到的字符串并不会添加'\0'
		sentence[p] = '\0';
		len = p;
		printf("CLIENT: %s\n", sentence);

		// 字符串处理
		for (p = 0; p < len; p++)
		{
			sentence[p] = toupper(sentence[p]);
		}

		// 如果用户已登录，处理命令
		if (is_login)
		{
			// TODO
		}
		else
		{
			printf("SENTENCE:%s\n", sentence);
			// 检查是否匿名登录
			if (strstr(sentence, "USER anonymous") != NULL)
			{
				// 返回请求密码
				write(connfd, "331 Guest login ok, send your complete e-mail address as password.\r\n", 63);
				printf("SERVER:331 Guest login ok, send your complete e-mail address as password.\r\n");
				// 接收密码信息
				p = 0;
				while (p < len)
				{
					int n = read(connfd, sentence + p, 8191 - p); // n是从connfd中读到的字节数
					if (n < 0)
					{
						printf("Error read(): %s(%d)\n", strerror(errno), errno);
						close(connfd);
						continue;
					}
					else if (n == 0)
					{
						break;
					}
					else
					{
						p += n;
						if (sentence[p - 1] == '\n')
						{
							break;
						}
					}
				}
				// socket接收到的字符串并不会添加'\0'
				sentence[p] = '\0';
				len = p;
				printf("sentence:%s\n", sentence);
				// 判断密码是否是邮箱地址
				if (strstr(sentence, "PASS ") != NULL)
				{
					char *email = strstr(sentence, "PASS ") + 5;
					// TODO 验证邮箱地址的有效性
					// 假设邮箱地址有效，发送欢迎信息
					write(connfd, "230 Guest login ok, access restrictions apply.\r\n", 51);
					is_login = 1;
				}
			}
			else
			{
				// 非匿名用户或无效命令
				write(connfd, "530 Login incorrect.\r\n", 23);
			}
		}

		// 发送字符串到socket
		// p = 0;
		// while (p < len)
		// {
		// 	int n = write(connfd, sentence + p, len + 1 - p);
		// 	if (n < 0)
		// 	{
		// 		printf("Error write(): %s(%d)\n", strerror(errno), errno);
		// 		return 1;
		// 	}
		// 	else
		// 	{
		// 		p += n;
		// 	}
		// }

		close(connfd);
	}

	close(listenfd);
}