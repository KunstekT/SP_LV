// client using winsock2 

#define _WIN32_WINNT 0x501

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <limits>

#define DEFAULT_PORT 2000
#define DEFAULT_BUFLEN 512

HANDLE receiveThread;

DWORD WINAPI receiveMessagesProc(LPVOID lpParam) {

	SOCKET clientSock = *(SOCKET *)lpParam;
	char recvbuf[DEFAULT_BUFLEN] = { 0 };
	int recvbuflen = DEFAULT_BUFLEN;
	int rtnVal;

	while (1) {
		ZeroMemory(recvbuf, recvbuflen);
		rtnVal = recv(clientSock, recvbuf, recvbuflen, 0);
		if (rtnVal > 0) {
			printf("%s\n", recvbuf);
		}
		else if (rtnVal == "P") {
			send(clientSock, "OK", (int)strlen("OK"), 0);
			shutdown(clientSock, SD_SEND);
		}
		else if (rtnVal == 0) {
			printf("connection closed.\n");
			exit(1);
		}
		else {
			printf("recv failed: %d\n", WSAGetLastError());
			exit(1);
		}
	}

	return 0;
}

int main(int argc, char** argv) {

	if (argc != 3) {
		return 1;
	}

	// argv[1] = ip address to connect with 
	// argv[2] = user name 

	WSADATA wsaData;

	int iResult;
	const char* username = argv[2];

	// Winsock inicijalizacija 
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	// Startanje socketa
	SOCKET connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connectSocket == INVALID_SOCKET) {
		printf("socket() failed\n");
		WSACleanup();
		return 1;
	}

	struct sockaddr_in servAddr;
	ZeroMemory(&servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;

	int size = sizeof(servAddr);
	iResult = WSAStringToAddressA(argv[1], AF_INET, NULL, (struct sockaddr *)&servAddr, &size);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	servAddr.sin_port = htons(DEFAULT_PORT);

	// Spajanje socketa sa serverom
	iResult = connect(connectSocket, (struct sockaddr *)&servAddr, sizeof(servAddr));
	if (iResult == SOCKET_ERROR) {
		closesocket(connectSocket);
		connectSocket = INVALID_SOCKET;
		return 1;
	}
	else {
		printf("uspjesno spojeno na server\n");
		printf("------------------------------\n");
	}

	receiveThread = CreateThread(NULL, 0, receiveMessagesProc, &connectSocket, 0, 0);

	std::string helloMsg = "N" + std::string(username);
	const char *helloMsgBuf = (const char *)helloMsg.c_str();
	send(connectSocket, helloMsgBuf, (int)strlen(helloMsgBuf), 0);

	do {

		std::string newInputHead = username + std::string(": ");
		std::string newInput;
		std::getline(std::cin, newInput);
		std::cin.clear();

		if (newInput == "quit") {

			DWORD exitCode;
			if (GetExitCodeThread(receiveThread, &exitCode) != 0) {
				TerminateThread(receiveThread, exitCode);
			}

			shutdown(connectSocket, SD_SEND);
			closesocket(connectSocket);
			WSACleanup();
			return 0;
		}

		newInput = newInputHead + newInput;
		const char *msg = (const char *)newInput.c_str();
		iResult = send(connectSocket, msg, (int)strlen(msg), 0);

	} while (iResult > 0);

	iResult = shutdown(connectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}

	closesocket(connectSocket);
	WSACleanup();

	return 0;
}