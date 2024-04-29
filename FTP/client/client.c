#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <string.h>
#include <memory.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <stdlib.h>

// ��server������Ϣ
int recieve_from_server(int sockfd, char *sentence, int is_file)
{
	// printf("recieve\n");
	int p = 0;

	if (!is_file)
	{
		while (1)
		{
			// printf("1\n");
			int n = read(sockfd, sentence + p, 8192 - p);
			// printf("%s\n", sentence);
			if (n < 0)
			{
				printf("Error read(): %s(%d)\n", strerror(errno), errno);
				return 1;
			}
			else if (n == 0)
			{
				printf("n=0\n");
				break;
			}
			else
			{
				p += n;
				if (sentence[p - 1] == '\n' || sentence[p - 1] == '\r')
				{
					break;
				}
			}
		}
		sentence[p - 1] = '\0';
		printf(">>>SERVER:%s\n", sentence);
		return 0;
	}
	else
	{
		int n = read(sockfd, sentence, 8192);
		return n;
	}
}

// ��server������Ϣ
int send_to_server(int sockfd, char *sentence, int len)
{
	int p = 0;
	// len = strlen(sentence);
	// printf("len=%d\n", len);
	while (p < len)
	{
		int n = write(sockfd, sentence + p, len + 1 - p); // write��������֤���е�����д�꣬������;�˳�
		if (n < 0)
		{
			printf("Error write(): %s(%d)\n", strerror(errno), errno);
			return 1;
		}
		else
		{
			p += n;
		}
	}
	printf(">>>CLIENT:%s\n", sentence);
	return 0;
}

int main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in addr;
	char sentence[8192]; // ��һ���������Ϳ���
	int len;
	int p;

	int port = 21; // Ĭ�϶˿ں�Ϊ21
	//char ip[16] = "166.111.83.113";
	// char ip[16]="192.168.116.129";
	char ip[16]="127.0.0.1";

	char verb[5];	 // �洢ָ��
	char info[8192]; // �洢request����Ϣ����
	int code;		 // ������

	int listenfd = -1; // �����ļ�ʱ�ȴ�server�����������ϴ��ļ�ʱ��������
	int connfd = -1;   // �����ļ�ʱ�����ļ����ݣ��ϴ�ʱ�����ļ�����

	int connect_mode = 0; // 0��ʾ����ģʽ(active mode),1��ʾ����ģʽ(passive mode)
	int connected = 0;

	int file_exist = 0; // 0��ʾԭ�������ڸ��ļ�
	FILE *file;

	int response_len;
	int input_len;

	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "-port") == 0)
		{
			port = atoi(argv[i + 1]);
			printf("port=%d\n",port);
		}
		else if (strcmp(argv[i], "-ip") == 0)
		{
			if (i + 1 < argc)
			{
				strcpy(ip, argv[i + 1]);
			}
		}
	}

	// ����socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	// ����Ŀ��������ip��port
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(21);
	if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0)
	{ // ת��ip��ַ:���ʮ����-->������
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	// ������Ŀ����������socket��Ŀ���������ӣ�-- ��������
	if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		printf("Error connect(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	// printf("connected\n");

	// ���ܷ�������ʼ��Ϣ
	recieve_from_server(sockfd, sentence, 0);

	while (1)
	{
		// ��ȡ��������
		fgets(sentence, 8192, stdin);
		len = strlen(sentence);
		sentence[len] = '\n';
		sentence[len + 1] = '\0';
		// �Ѽ�������д��socket
		send_to_server(sockfd, sentence, len);

		// ����request
		sscanf(sentence, "%s %s", verb, info);

		if (strcmp(verb, "PORT") == 0)
		{
			// ����Ѿ��ڼ�����رռ���
			if (listenfd != -1)
			{
				close(listenfd);
				listenfd = -1;
			}

			// ����IP��ַ�Ͷ˿ں�
			int h1, h2, h3, h4, p1, p2;
			sscanf(info, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
			sprintf(ip, "%d.%d.%d.%d", h1, h2, h3, h4); // ��ű�׼IP��ַ
			port = p1 * 256 + p2;						// �����Ķ˿ں�
			printf("ip=%s\n", ip);
			printf("port=%d\n", port);

			// �������ӣ�׼�������ļ�����
			// ����socket
			if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
			{
				printf("Error socket(): %s(%d)\n", strerror(errno), errno);
				return 1;
			}
			// ����Ŀ��������ip��port
			int option = 1;
			setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)); // ������ʹ�ù��Ķ˿�

			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			addr.sin_addr.s_addr = htonl(INADDR_ANY);

			if (listenfd == -1)
			{
				return 1;
			}
			if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
			{
				printf("Error bind(): %s(%d)\n", strerror(errno), errno);
				return 1;
			}
			if (listen(listenfd, 10) == -1)
			{
				printf("Error listen(): %s(%d)\n", strerror(errno), errno);
				return 1;
			}
			connect_mode = 0; // active mode
			connected = 1;
			printf("listenfd(port)=%d\n", listenfd);
		}
		if (strcmp(verb, "PASV") == 0)
		{
			// �����������ر�����
			if (connfd != -1)
			{
				close(connfd);
				connfd = -1;
			}

			// ��server������Ϣ
			recieve_from_server(sockfd, sentence, 0);
			// printf("SERVER: %s\n", sentence);

			// ����IP��ַ�Ͷ˿ں�
			int h1, h2, h3, h4, p1, p2;
			sscanf(sentence, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);
			sprintf(ip, "%d.%d.%d.%d", h1, h2, h3, h4);
			// printf("h1=%d,h2=%d,h3=%d,h4=%d,p1=%d,p2=%d\n",h1,h2,h3,h4,p1,p2);
			port = p1 * 256 + p2;
			// printf("ip=%s\n", ip);
			// printf("port=%d\n", port);

			// �������ӣ�׼�������ļ�����
			// ����socket
			if ((connfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
			{
				printf("Error socket(): %s(%d)\n", strerror(errno), errno);
				return 1;
			}
			// ����Ŀ��������ip��port
			int option = 1;
			setsockopt(connfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)); // ������ʹ�ù��Ķ˿�
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			addr.sin_addr.s_addr = inet_addr(ip);
			connect_mode = 1; // passive mode
			connected = 1;
			continue; // ֲ��de����һ��
		}
		if (strcmp(verb, "RETR") == 0)
		{
			// ��server������Ϣ
			// recieve_from_server(sockfd, sentence, 0);
			// printf("SERVER: %s\n", sentence);
			if (connected == 1)
			{
				// printf("0\n");
				if (connect_mode == 0)
				{
					// printf("0\n");
					// printf("listenfd=%d\n", listenfd);
					if ((connfd = accept(listenfd, NULL, NULL)) == -1)
					{
						printf("Error accept(): %s(%d)\n", strerror(errno), errno);
						return 1;
					}
				}
				else
				{
					// printf("1\n");
					if (connect(connfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
					{
						printf("Error connect(): %s(%d)\n", strerror(errno), errno);
						return 1;
					}
					// printf("2\n");
				}
			}
			// ��server������Ϣ
			recieve_from_server(sockfd, sentence, 0);
			// printf("SERVER: %s\n", sentence);

			// ������յ�226��ʾ�������
			int buff_len = strlen(sentence);
			if (buff_len >= 3)
			{
				for (int i = 0; i < buff_len - 3; i++)
				{
					if (sentence[i] == '2' && sentence[i + 1] == '2' && sentence[i + 2] == '6')
					{
						printf("successfully written\n");
						break;
					}
				}
			}

			// �����ظ�
			sscanf(sentence, "%d %s", &code, sentence);

			if (code == 150)
			{
				connected = 0; // �ļ����俪ʼ�����ӿ��Թر�
			}

			if (file_exist == 0)
			{ // ����ļ���������Ҫ�½�
				file = fopen(info, "wb");
				while ((response_len = recieve_from_server(connfd, sentence, 1)))
				{
					fwrite(sentence, sizeof(char), response_len, file);
				}
				fclose(file);
			}
			else
			{ // ����Ѵ�����ֱ�Ӹ���
				file = fopen(info, "ab");
				while ((response_len = recieve_from_server(connfd, sentence, 1)))
				{
					fwrite(sentence, sizeof(char), response_len, file);
				}
				file_exist = 0;
				fclose(file);
			}

			if (listenfd != -1)
			{
				close(listenfd);
				listenfd = -1;
			}
			if (connfd != -1)
			{
				close(connfd);
				connfd = -1;
			}
		}
		if (strcmp(verb, "STOR") == 0)
		{

			if (connected == 1)
			{
				// printf("0\n");
				if (connect_mode == 0)
				{
					// printf("0\n");
					if ((connfd = accept(listenfd, NULL, NULL)) == -1)
					{
						printf("Error accept(): %s(%d)\n", strerror(errno), errno);
						return 1;
					}
					// printf("1\n");
				}
				else
				{
					// printf("1\n");
					if (connect(connfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
					{
						printf("Error connect(): %s(%d)\n", strerror(errno), errno);
						return 1;
					}
					// printf("2\n");
				}
			}

			// ��server������Ϣ
			recieve_from_server(sockfd, sentence, 0);
			// printf("SERVER: %s\n", sentence);

			// ������յ�226��ʾ�������
			int buff_len = strlen(sentence);
			if (buff_len >= 3)
			{
				for (int i = 0; i < buff_len - 3; i++)
				{
					if (sentence[i] == '2' && sentence[i + 1] == '2' && sentence[i + 2] == '6')
					{
						printf("successfully written\n");
						break;
					}
				}
			}

			// �����ظ�
			sscanf(sentence, "%d %s", &code, sentence);

			if (code == 150)
			{
				connected = 0; // �ļ����俪ʼ�����ӿ��Թر�
			}

			// if (file_exist == 0)
			// { // ����ļ���������Ҫ�½�
			file = fopen(info, "rb");
			if (file == NULL)
			{
				printf("File doesn't exist");
			}
			else
			{
				// printf("file exist\n");
				while ((response_len = fread(sentence, sizeof(char), 8192, file)))
				{
					// printf("here1\n");
					if (send(connfd, sentence, response_len, 0) < 0)
					{
						printf("error write\n");
					}
				}
				fclose(file);
			}
			// }
			// else
			// { // ����Ѵ�����ֱ�Ӹ���
			// 	file = fopen(info, "rb");
			// 	if (file == NULL)
			// 	{
			// 		printf("File doesn't exist");
			// 	}
			// 	else
			// 	{
			// 		fseek(file, file_exist, SEEK_SET);
			// 		while ((input_len = fread(sentence, sizeof(char), 8192, file)))
			// 			send_to_server(connfd, sentence, input_len);
			// 		fclose(file);
			// 	}
			// }

			if (listenfd != -1)
			{
				close(listenfd);
				listenfd = -1;
			}
			if (connfd != -1)
			{
				close(connfd);
				connfd = -1;
			}
		}
		if (strcmp(verb, "LIST") == 0)
		{
			if (connected == 1)
			{
				if (connect_mode == 0)
				{
					if ((connfd = accept(listenfd, NULL, NULL)) == -1)
					{
						printf("Error accept(): %s(%d)\n", strerror(errno), errno);
						return 1;
					}
				}
				else
				{
					if (connect(connfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
					{
						printf("Error connect(): %s(%d)\n", strerror(errno), errno);
						return 1;
					}
				}
			}
			recieve_from_server(sockfd, sentence, 0);

			recieve_from_server(connfd, sentence, 0);

			connected = 0;

			if (connfd != -1)
			{
				close(connfd);
				connfd = -1;
			}
		}
		if (strcmp(verb, "QUIT") == 0 || strcmp(verb, "ABRT") == 0)
		{
			break;
		}

		recieve_from_server(sockfd, sentence, 0);
	}

	close(sockfd);
	if (listenfd != -1)
	{
		close(listenfd);
	}

	return 0;
}