#include "TCPServer.h"

#define SERVERPORT 9000

MainCharacter p1, p2;
BossMonster bossMob;

ThreadParams p1ThreadParams, p2ThreadParams;
// THREAD HANDLE
HANDLE hP1Thread, hP2Thread;
HANDLE hUpdateThread;

// EVENT HANDLE
HANDLE hPlayer1Input, hPlayer2Input;
HANDLE hPlayer1Update, hPlayer2Update;

DWORD WINAPI NetworkThread(LPVOID arg)
{
	ThreadParams threadParams = *(ThreadParams*)arg;

	int retval;
	SOCKET client_sock = threadParams.socket;
	struct sockaddr_in clientaddr;
	int addrlen;

	// 클라이언트 정보 얻기
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);

	// 접속한 클라이언트 정보 출력
	char addr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
	printf("[클라이언트 접속 IP: %s, 포트 번호: %d]", addr, ntohs(clientaddr.sin_port));

	// BITMAP WIDTH, HEIGHT값 수신
	// tbd

	// 초기화 작업
	// tbd
	CreateMainChar(&p1);

	// INIT 데이터를 송신
	// tbd

	int len;
	char buf[256];
	while (1)
	{
		// INPUT 데이터를 받는다
		// 임시로 빈 버퍼를 받는다.
		retval = recv(client_sock, (char*)&len, sizeof(int), MSG_WAITALL);
		if (retval == SOCKET_ERROR) 
		{
			err_display("recv()");
			break;
		}

		retval = recv(client_sock, (char*)buf, len, MSG_WAITALL);
		if (retval == SOCKET_ERROR) 
		{
			err_display("recv()");
			break;
		}

		if(hP1Thread == threadParams.hThread)
		{
			SetEvent(hPlayer1Input);
		}
		//else if(hP2Thread == threadParams.hThread)
		//	SetEvent(hPlayer2Input);


		// 업데이트 스레드가 완료되기를 기다린다.
		if (hP1Thread == threadParams.hThread)
		{
			WaitForSingleObject(hPlayer1Update, INFINITE);
		}
		//else if (hP2Thread == threadParams.hThread)
		//	WaitForSingleObject(hPlayer2Update, INFINITE);

		SendUpdateData updateData;
		updateData.player1 = p1.info;
		//updateData.player2 = p2.info;

		// 서버에서 업데이트한 내용을 보내준다
		if (SendDefaultData(client_sock, updateData) == -1)
		{
			// 오류 처리
			printf("Send Default Error\n");
		}
	}

	// 소켓 닫기
	closesocket(client_sock);
	printf("[클라이언트 종료 IP : %s, 포트 번호 : %d]", addr, ntohs(clientaddr.sin_port));
	
	return 0;
}

DWORD WINAPI UpdateThread(LPVOID arg)
{
	while (1)
	{	
		// INPUT이 완료되기를 기다린다.
		WaitForSingleObject(hPlayer1Input, INFINITE);
		//WaitForSingleObject(hPlayer2Input, INFINITE);

		//printf("업데이트를 수행합니다!!!!");
		//Sleep(5000);
		p1.info.animationNum++;
		if (p1.info.state == MainState::IDLE)
		{
			p1.info.animationNum++;
			if (p1.info.animationNum > 4)
			{
				p1.info.animationNum = 0;
			}
		}

		// 업데이트 완료
		SetEvent(hPlayer1Update);
		//SetEvent(hPlayer2Update);
	}
	return 0;
}

int main(int argc, char* argv[])
{
	int retval;

	hPlayer1Input = CreateEvent(NULL, FALSE, FALSE, NULL);
	hPlayer1Update = CreateEvent(NULL, FALSE, FALSE, NULL);

	hUpdateThread = CreateThread(NULL, 0, UpdateThread, NULL, 0, NULL);

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 소켓 생성
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// 데이터 통신에 사용할 변수
	struct sockaddr_in clientaddr;
	int addrlen;

	int nPlayer = 1;
	while (1) 
	{
		// 일단은 p1부터 받아보자
		addrlen = sizeof(clientaddr);
		if(nPlayer == 1)
		{
			// accept()
			p1ThreadParams.socket = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
			if (p1ThreadParams.socket == INVALID_SOCKET)
			{
				err_display("accept()");
				break;
			}
		}
		else if (nPlayer == 2)
		{
			// accept()
			p2ThreadParams.socket = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
			if (p2ThreadParams.socket == INVALID_SOCKET)
			{
				err_display("accept()");
				break;
			}
		}

		if (nPlayer == 1)
		{
			// 스레드 생성
			p1ThreadParams.hThread = hP1Thread = CreateThread(NULL, 0, NetworkThread, (LPVOID)&p1ThreadParams, 0, NULL);
			if (p1ThreadParams.hThread == NULL)
			{
				closesocket(p1ThreadParams.socket);
			}
		}
		else if (nPlayer == 2)
		{
			// 스레드 생성
			p2ThreadParams.hThread = hP2Thread = CreateThread(NULL, 0, NetworkThread, (LPVOID)&p2ThreadParams, 0, NULL);
			if (p1ThreadParams.hThread == NULL)
			{
				closesocket(p2ThreadParams.socket);
			}
		}

		++nPlayer;
	}

	closesocket(listen_sock);	// 소켓 닫기
	WSACleanup();				// 윈속 종료
	return 0;
}
