#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>

#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <time.h>

char verb[5];	 // �洢ָ��
char info[1000]; // �洢request����Ϣ����
char ip[16] = "127.0.0.1";
char root[50] = "/tmp"; // �ļ���Ŀ¼

// �ӿͻ��˽�����Ϣ
int recieve_from_client(int connfd, char *sentence)
{
	// ե��socket����������
	int p = 0;
	int len;
	while (1)
	{
		int n = read(connfd, sentence + p, 8191 - p);
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
	// socket���յ����ַ�������������'\0'
	sentence[p - 1] = '\0';
	len = p - 1;
	printf(">>>CLIENT:%s\n", sentence);
}
// ������Ϣ���ͻ���
int send_to_client(int connfd, char *sentence, int len)
{
	int p = 0;
	// len = strlen(sentence);
	// printf("len=%d\n", len);
	while (p < len)
	{
		int n = write(connfd, sentence + p, len - p);
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
	printf(">>>SERVER:%s", sentence);
	return 0;
}
// �߳�
void *thread(void *arg)
{
	chdir(root);
	int listenfd = -1;			// ����socket
	int connfd = *((int *)arg); // ����socket,�������ݴ���
	int filefd = -1;
	struct sockaddr_in addr;
	char sentence[8192];
	int p;
	int len;
	int port = 21;
	int connect_mode = -1; // 0��ʾ����ģʽPORT(active mode),1��ʾ����ģʽPASV(passive mode)
	// int connected = 0; // connected_mode = -1����ʾconnected = 0
	int is_login = 0; // 0��ʾδ��¼��1��ʾ�ȴ��������룬2��ʾ�ѵ�¼

	// printf("strat\n");
	// ���ͳ�ʼ��Ϣ
	send_to_client(connfd, "220 Anonymous FTP server ready.\r\n", strlen("220 Anonymous FTP server ready.\r\n"));

	while (1)
	{
		recieve_from_client(connfd, sentence);
		// ��������
		sscanf(sentence, "%s %s", verb, info);

		if (strcmp(verb, "USER") == 0)
		{
			if (strcmp(info, "anonymous") == 0)
			{
				is_login = 1;
				send_to_client(connfd, "331 Guest login ok, send your complete e-mail address as password.\r\n", strlen("331 Guest login ok, send your complete e-mail address as password.\r\n"));
			}
			else
			{
				send_to_client(connfd, "430 Invalid username.\r\n", strlen("430 Invalid username.\r\n"));
			}
		}
		else if (strcmp(verb, "PASS") == 0)
		{
			if (is_login == 0)
			{
				send_to_client(connfd, "530 Not logged in, password error.\r\n", strlen("530 Not logged in, password error.\r\n"));
				continue;
			}
			else if (is_login == 1)
			{
				send_to_client(connfd, "230 login success.\r\n", strlen("230 login success.\r\n"));
				is_login = 2;
				continue;
			}
			else if (is_login == 2)
			{
				send_to_client(connfd, "503 Bad sequence of commands.\r\n", strlen("503 Bad sequence of commands.\r\n"));
				continue;
			}
		}
		else if (strcmp(verb, "ABRT") == 0 || strcmp(verb, "QUIT") == 0)
		{
			send_to_client(connfd, "221 GoodBye.\r\n", strlen("221 GoodBye.\r\n"));
			close(listenfd);
			close(filefd);
			close(connfd);
			listenfd = -1;
			filefd = -1;
			connfd = -1;
			return 0;
		}
		else if (is_login == 2)
		{
			if (strcmp(verb, "PORT") == 0)
			{

				if (filefd != -1)
				{
					close(filefd);
					filefd = -1;
				}

				// ����IP��ַ�Ͷ˿ں�
				// printf("fajkl\n");
				int h1, h2, h3, h4, p1, p2;
				sscanf(info, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
				sprintf(ip, "%d.%d.%d.%d", h1, h2, h3, h4); // ��ű�׼IP��ַ
				port = p1 * 256 + p2;						// �����Ķ˿ں�
				// printf("ip=%s\n", ip);
				// printf("port=%d\n", port);

				// �������ӣ�׼�������ļ�����
				// ����socket
				if ((filefd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
				{
					printf("Error socket(): %s(%d)\n", strerror(errno), errno);
					filefd = -1;
				}
				// // ����Ŀ��������ip��port
				// int option = 1;
				// setsockopt(filefd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)); // ������ʹ�ù��Ķ˿�

				memset(&addr, 0, sizeof(addr));
				addr.sin_family = AF_INET;
				addr.sin_port = htons(port);

				addr.sin_addr.s_addr = inet_addr(ip);

				connect_mode = 0;

				if (filefd == -1)
				{
					send_to_client(connfd, "425 Connected Error()\r\n", strlen("425 Connected Error()\r\ns"));
				}
				else
				{
					// printf("111\n");
					send_to_client(connfd, "200 PORT command successful\r\n", strlen("200 PORT command successful\r\n"));
				}
			}
			else if (strcmp(verb, "PASV") == 0)
			{
				if (is_login != 2)
				{
					send_to_client(connfd, "530 Please login with USER and PASS first.\r\n", strlen("530 Please login with USER and PASS first.\r\n"));
				}
				else if (is_login == 2)
				{
					if (listenfd != -1)
					{
						close(listenfd);
						listenfd = -1;
					}
					// �漴����20000-65535�Ķ˿ں�
					srand(time(NULL));
					port = rand() % 45535 + 20000;
					int port_st = port / 256;
					int port_nd = port % 256;
					sprintf(ip, "127.0.0.1");

					// �������ӣ�׼�������ļ�����
					// ����socket
					if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
					{
						printf("Error socket(): %s(%d)\n", strerror(errno), errno);
						listenfd = -1;
					}
					// ����Ŀ��������ip��port
					int option = 1;
					setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)); // ������ʹ�ù��Ķ˿�

					memset(&addr, 0, sizeof(addr));
					addr.sin_family = AF_INET;
					addr.sin_port = htons(port);
					addr.sin_addr.s_addr = htonl(INADDR_ANY);
					if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
					{
						sprintf(sentence, "Error bind(): %s(%d)\r\n", strerror(errno), errno);
						send_to_client(connfd, sentence, strlen(sentence));
						continue;
					}
					if (listen(listenfd, 10) == -1)
					{
						sprintf(sentence, "Error listen(): %s(%d)\r\n", strerror(errno), errno);
						send_to_client(connfd, sentence, strlen(sentence));
						continue;
					}
					connect_mode = 1;
					sprintf(sentence, "227 Entering Passive Mode (127,0,0,1,%d,%d)\r\n", port_st, port_nd);
					send_to_client(connfd, sentence, strlen(sentence));
				}
			}
			else if (strcmp(verb, "SYST") == 0)
			{
				send_to_client(connfd, "215 UNIX Type: L8\r\n", strlen("215 UNIX Type: L8\r\n"));
			}
			else if (strcmp(verb, "TYPE") == 0)
			{
				if (strcmp(info, "I") == 0)
				{
					send_to_client(connfd, "200 Type set to I.\r\n", strlen("200 Type set to I.\r\n"));
				}
				else
				{
					send_to_client(connfd, "501 Type Error.\r\n", strlen("501 Type Error.\r\n"));
				}
			}
			else if (strcmp(verb, "RETR") == 0)
			{
				if (connect_mode != -1)
				{
					if (connect_mode == 0)
					{
						if (connect(filefd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
						{
							printf("Error connect(): %s(%d)\n", strerror(errno), errno);
							send_to_client(connfd, "425 Data Connection Failed to connect.\r\n", strlen("425 Data Connection Failed to connect.\r\n"));
							if (listenfd != -1)
							{
								close(listenfd);
								listenfd = -1;
							}
							if (filefd != -1)
							{
								close(filefd);
								filefd = -1;
							}
							continue;
						}
						else
						{
							// printf("connect \n");
						}
					}
					else if (connect_mode == 1)
					{
						if ((filefd = accept(listenfd, NULL, NULL)) == -1)
						{
							printf("Error accept(): %s(%d)\n", strerror(errno), errno);
							send_to_client(connfd, "425 Data Connection Failed to connect.\r\n", strlen("425 Data Connection Failed to connect.\r\n"));
							if (listenfd != -1)
							{
								close(listenfd);
								listenfd = -1;
							}
							if (filefd != -1)
							{
								close(filefd);
								filefd = -1;
							}
							continue;
						}
					}
					FILE *file;
					file = fopen(info, "rb");
					if (file == NULL)
					{
						send_to_client(connfd, "551 File not found.\r\n", strlen("551 File not found.\r\n"));
						continue;
					}
					else
					{
						fseek(file, 0, SEEK_END);
						int file_size = ftell(file);
						fseek(file, 0, SEEK_SET);
						sprintf(sentence, "150 Opening BINARY mode data connection for %s,(%d bytes)\r\n", info, file_size);
						send_to_client(connfd, sentence, strlen(sentence));
						int len;
						while ((len = fread(sentence, sizeof(char), 8192, file)))
						{
							// printf("1\n");
							if (send(filefd, sentence, len, 0) < 0)
							{
								printf("Error write(): %s(%d)\n", strerror(errno), errno);
							}
							// printf("2\n");
						}
						fclose(file);
					}

					connect_mode = -1;
					if (listenfd != -1)
					{
						close(listenfd);
						listenfd = -1;
					}
					if (filefd != -1)
					{
						close(filefd);
						filefd = -1;
					}
					send_to_client(connfd, "226 Transfer complete.\r\n", strlen("226 Transfer complete.\r\n"));
				}
				else
				{
					send_to_client(connfd, "503 Bad sequence of commands.\r\n", strlen("503 Bad sequence of commands.\r\n"));
				}
			}
			else if (strcmp(verb, "STOR") == 0)
			{
				if (connect_mode != -1)
				{
					if (connect_mode == 0)
					{
						if (connect(filefd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
						{
							printf("Error connect(): %s(%d)\n", strerror(errno), errno);
							send_to_client(connfd, "425 Data Connection Failed to connect.\r\n", strlen("425 Data Connection Failed to connect.\r\n"));
							if (listenfd != -1)
							{
								close(listenfd);
								listenfd = -1;
							}
							if (filefd != -1)
							{
								close(filefd);
								filefd = -1;
							}
							continue;
						}
						else
						{
							printf("connect \n");
						}
					}
					else if (connect_mode == 1)
					{
						if ((filefd = accept(listenfd, NULL, NULL)) == -1)
						{
							printf("Error accept(): %s(%d)\n", strerror(errno), errno);
							send_to_client(connfd, "425 Data Connection Failed to connect.\r\n", strlen("425 Data Connection Failed to connect.\r\n"));
							if (listenfd != -1)
							{
								close(listenfd);
								listenfd = -1;
							}
							if (filefd != -1)
							{
								close(filefd);
								filefd = -1;
							}
							continue;
						}
					}

					FILE *file;
					file = fopen(info, "wb");
					int file_size;
					double speed;

					if (file == NULL)
					{
						send_to_client(connfd, "551 File created error.\r\n", strlen("551 File created error.\r\n"));
						continue;
					}
					else
					{
						struct timespec start_time, end_time;
						double time;
						int len;

						sprintf(sentence, "150 Opening BINARY mode data connection for %s\r\n", info);
						send_to_client(connfd, sentence, strlen(sentence));
						clock_gettime(CLOCK_MONOTONIC, &start_time);
						while ((len = recv(filefd, sentence, 8192, 0)) > 0)
						{
							fwrite(sentence, sizeof(char), len, file);
						}
						if (len < 0)
						{
							printf("Error receive file().\r\n");
							continue;
						}
						clock_gettime(CLOCK_MONOTONIC, &end_time);
						time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
						fseek(file, 0, SEEK_END);
						file_size = ftell(file);
						fseek(file, 0, SEEK_SET);
						fclose(file);
						speed = (double)file_size / time / 1024;
					}

					connect_mode = -1;
					if (listenfd != -1)
					{
						close(listenfd);
						listenfd = -1;
					}
					if (filefd != -1)
					{
						close(filefd);
						filefd = -1;
					}
					sprintf(sentence, "226 Transfer complete. Transferred:%d Bytes. Average speed:%.2lf KB/s.\r\n", file_size, speed);
					send_to_client(connfd, sentence, strlen(sentence));
				}
				else
				{
					send_to_client(connfd, "503 Bad sequence of commands.\r\n", strlen("503 Bad sequence of commands.\r\n"));
				}
			}
			else if (strcmp(verb, "MKD") == 0)
			{
				if (mkdir(info, 0777) == -1)
				{ // 0777��ʾĿ¼Ȩ��
					send_to_client(connfd, "504 Failed to create directory.\r\n", strlen("504 Failed to create directory.\r\n"));
					continue;
				}
				chdir(info);
				send_to_client(connfd, "257 Directory created successfully\r\n", strlen("257 Directory created successfully\r\n"));
			}
			else if (strcmp(verb, "CMD") == 0)
			{
				char path_before[50];
				char path_after[50];
				getcwd(path_before, 50); // ��ǰĿ¼
				if (chdir(info) == -1)
				{
					send_to_client(connfd, "550 CWD command failed: directory not found.\r\n", strlen("550 CWD command failed: directory not found.\r\n"));
					continue;
				}
				getcwd(path_after, 50); // ���ĺ��Ŀ�?
				if (strncmp(path_after, root, strlen(root)))
				{ // ���ĺ��Ŀ¼���ڸ�Ŀ¼��?
					chdir(path_before);
					send_to_client(connfd, "504 Invalid directory.\r\n", strlen("504 Invalid directory.\r\n"));
					continue;
				}
				sprintf(sentence, "250 Change directory success. Now in '%s'.\r\n", path_after);
				send_to_client(connfd, sentence, strlen(sentence));
			}
			else if (strcmp(verb, "PWD") == 0)
			{
				char cur_dic[50];
				if (getcwd(cur_dic, 50) != NULL)
				{
					snprintf(sentence, sizeof(sentence), "257 \"%s\" is the current directory.\r\n", cur_dic);
					send_to_client(connfd, sentence, strlen(sentence));
				}
				else
				{
					send_to_client(connfd, "550 Failed to get current directory.\r\n", strlen("550 Failed to get current directory.\r\n"));
				}
			}
			else if (strcmp(verb, "LIST") == 0)
			{
				if (connect_mode == -1)
				{
					send_to_client(connfd, "503 Invalid command. Please select mode.\r\n", strlen("503 Invalid command. Please select mode.\r\n"));
					continue;
				}
				else if (connect_mode == 0)
				{ // PORT
					if (connect(filefd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
					{
						printf("Error connect(): %s(%d)\n", strerror(errno), errno);
						send_to_client(connfd, "425 Connection failed.\r\n", strlen("425 Connection failed.\r\n"));
						if (listenfd != -1)
						{
							close(listenfd);
							listenfd = -1;
						}
						if (filefd != -1)
						{
							close(filefd);
							filefd = -1;
						}
						continue;
					}
				}
				else if (connect_mode == 1)
				{ // PASV
					if ((filefd = accept(listenfd, NULL, NULL)) == -1)
					{
						printf("Error accept(): %s(%d)\n", strerror(errno), errno);
						send_to_client(connfd, "425 Connection failed.\r\n", strlen("425 Connection failed.\r\n"));
						if (listenfd != -1)
						{
							close(listenfd);
							listenfd = -1;
						}
						if (filefd != -1)
						{
							close(filefd);
							filefd = -1;
						}
						continue;
					}
				}
				FILE *file;
				file = popen("ls -l", "r");
				if (file == NULL)
				{
					printf("error open file\n");
					continue;
				}
				if (filefd == -1)
				{
					send_to_client(connfd, "425 No connection was established.\r\n", strlen("425 No connection was established.\r\n"));
					continue;
				}
				send_to_client(connfd, "150 Opening data channel for directory list.\r\n", strlen("150 Opening data channel for directory list.\r\n"));
				int len;
				while ((len = fread(sentence, sizeof(char), 8192, file)) > 0)
				{
					if (send(filefd, sentence, len, 0) < 0)
					{
						printf("Error send(): %s(%d)\n", strerror(errno), errno);
						send_to_client(connfd, "426 Connection was broken.\r\n", strlen("426 Connection was broken.\r\n"));
						continue;
					}
				}
				pclose(file);
				close(listenfd);
				close(filefd);

				connect_mode = -1;
				if (listenfd != -1)
				{
					close(listenfd);
					listenfd = -1;
				}
				if (filefd != -1)
				{
					close(filefd);
					filefd = -1;
				}
				send_to_client(connfd, "226 List complete.\r\n", strlen("226 List complete.\r\n"));
			}
			else if (strcmp(verb, "RMD") == 0)
			{
				if (rmdir(info) == 0)
				{ // �ɹ�ɾ��
					send_to_client(connfd, "250 Directory deleted successfully.\r\n", strlen("250 Directory deleted successfully.\r\n"));
				}
				else
				{
					sprintf(sentence, "550 Delete directory failed. %s\r\n", strerror(errno));
					send_to_client(connfd, sentence, strlen(sentence));
				}
			}
			else if (strcmp(verb, "RNFR") == 0)
			{
				char name_old[8192];
				strcpy(name_old, info);
				if (access(info, F_OK) == 0)
				{
					send_to_client(connfd, "350 File/folder exists, ready for destination name.\r\n", strlen("350 File/folder exists, ready for destination name.\r\n"));
				}
				else
				{
					send_to_client(connfd, "550 File not found.\r\n", strlen("550 File not found.\r\n"));
				}
				recieve_from_client(connfd, sentence);
				// ��������
				sscanf(sentence, "%s %s", verb, info);
				if (strcmp(verb, "RNTO") == 0)
				{
					if (access(info, F_OK) != -1)
					{
						send_to_client(connfd, "550 File/folder already exists.\r\n", strlen("550 File/folder already exists.\r\n"));
						continue;
					}
					if (rename(name_old, info) == 0)
					{
						send_to_client(connfd, "250 File/folder renamed successfully.\r\n", strlen("250 File/folder renamed successfully.\r\n"));
					}
					else
					{
						send_to_client(connfd, "504 Rename failed.\r\n", strlen("504 Rename failed.\r\n"));
					}
				}
			}
			else
			{
				send_to_client(connfd, "501 Invalid command.\r\n", strlen("501 Invalid command.\r\n"));
			}
		}
		else
		{
			send_to_client(connfd, "530 Invalid command. Please login.\r\n", strlen("530 Invalid command. Please login.\r\n"));
		}
	}
}

int main(int argc, char **argv)
{

	int listenfd, connfd; // ����socket������socket��һ���������������ݴ���
	int port = 21;
	struct sockaddr_in addr;
	srand((unsigned)time(0));
	for (int i = 0; i < argc - 1; i++)
	{
		if (strcmp(argv[i], "-port") == 0)
		{
			port = atoi(argv[i + 1]);
			printf("port=%d\n",port);
		}
		else if (strcmp(argv[i], "-root"))
		{
			strcpy(root, argv[i + 1]);
			// chdir(root); // ���������ĵ�ǰ����Ŀ¼����Ϊ�µ��ļ���Ŀ¼
		}
	}

	// ����socket
	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	// ���ñ�����ip��port
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY); // ����"0.0.0.0"

	// ��������ip��port��socket��
	if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	// ��ʼ����socket
	if (listen(listenfd, 10) == -1)
	{
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	// ����������������
	while (1)
	{
		// Handle new connection
		// Accept the connection, create a new socket connfd
		// �ȴ�client������ -- ��������
		if ((connfd = accept(listenfd, NULL, NULL)) == -1)
		{
			printf("Error accept(): %s(%d)\n", strerror(errno), errno);
			continue;
		}
		pthread_t thread_id;
		// printf("server connected.\n");
		//  �������߳�
		if (pthread_create(&thread_id, NULL, thread, &connfd))
			printf("thread create error\n");
		pthread_detach(thread_id);
	}

	close(listenfd);
	return 0;
}