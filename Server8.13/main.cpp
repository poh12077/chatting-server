#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include<iostream>
#include<list>

#define BUF_SIZE 100
#define EPOLL_SIZE 50 //epoll 사이즈 선언  
void error_handling(char* buf);
#define port 9000

using namespace std;

int main()
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t adr_sz;
	int str_len, i;
	char buf[BUF_SIZE];

	struct epoll_event* ep_events; //epoll 구조체 선언 
	struct epoll_event event; //epoll 구조체 선언
	int epfd, event_cnt;
	int fd_number = 0;
	int count = 0;

	list<int> fd_list;
	list<int>::iterator iter;


	/*if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}*/

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(port);

	if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");

	epfd = epoll_create(EPOLL_SIZE); //epoll 파일 디스크립터 정의 
	ep_events = (epoll_event*)malloc(sizeof(struct epoll_event) * EPOLL_SIZE); //epoll event 메모리 할당 

	event.events = EPOLLIN; //리스닝 소켓을 epoll에 등록한다. EPOLLIN을 대상으로 등록한다. 
	event.data.fd = serv_sock;	//epoll 파일 디스크립터를 서버 소켓으로 설정 
	int test = epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event); //epoll에 서버 소켓 파일 디스크립터 추가 
	while (1)

	{
		event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, 1000); //epoll event를 기다림

		if (event_cnt == -1)
		{
			puts("epoll_wait() error");
			break;
		}

		for (i = 0; i < event_cnt; i++)
		{
			if (ep_events[i].data.fd == serv_sock) //서버 이벤트 : 연결요청 수락 후 디스크립터 등록 과정을 거친다.  
			{
				adr_sz = sizeof(clnt_adr);
				clnt_sock =
					accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);

				fd_list.push_back(clnt_sock);		//FD 리스트에 소켓 fd 번호 추가
								
				event.events = EPOLLIN;
				event.data.fd = clnt_sock;
				epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
				printf("connected client: %d \n", clnt_sock);
			}
			else	//클라이언트 이벤트 : Read / disconnect
			{
				str_len = read(ep_events[i].data.fd, buf, BUF_SIZE);
				if (str_len == 0)    // close request! 
				{ //종료 요청의 경우 디스크립터 해제 과정을 거친다. 
					epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL); //epoll 감시대상에서 소켓번호를 제거함.
					close(ep_events[i].data.fd);	//클라이언트 소켓 닫기
					printf("closed client: %d \n", ep_events[i].data.fd);

					fd_list.remove(ep_events[i].data.fd);
				}

				else // 클라이언트 이벤트 : read 
				{
					for (iter = fd_list.begin(); iter != fd_list.end(); iter++)
					{
						if (*iter != ep_events[i].data.fd)
						{
							write(*iter, buf, str_len);
						}
					}					
					//	write(ep_events[i].data.fd, buf, str_len);    // echo!				
				}
			}
		}

	}

	close(serv_sock);		//서버 소켓 닫기
	close(epfd);
	return 0;
}

void error_handling(char* buf)
{
	fputs(buf, stderr);
	fputc('\n', stderr);
	exit(1);
}