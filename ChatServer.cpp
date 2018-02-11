// ChatServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <queue>
#include <shared_mutex>
#include <condition_variable>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

#define MAX_CLIENTS 3

// Struct that holds clientsockets and clientID's
typedef struct ThreadData {
private:
	int client_ID;
public:
	SOCKET ClientSockets[MAX_CLIENTS];
	int getClientID() {
		return client_ID++;
	}
	SOCKET * getClientSocket(int index) {
		return &ClientSockets[index];
	}
} ThreadData, *CLIENTDATA;

std::queue<std::string> buffer;
std::shared_mutex mutex_;

DWORD WINAPI broadcast_msg(LPVOID params) {
	while (true) {
	}
}

// Thread that receives textual data from client and prints it to console
DWORD WINAPI rcv_msg(LPVOID params) {
	SOCKET *ClientSocket = new SOCKET;
	ClientSocket = (SOCKET*)params;
	int iResult;
	int clientID = 0;
	char recvbuf[DEFAULT_BUFLEN];
	std::string recBuf = "";
	int recvbuflen = DEFAULT_BUFLEN;
	do {
		iResult = recv(*ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("Client %d> %.*s\n", clientID, iResult, recvbuf);
		}
		else if (iResult == 0)
			printf("Connection closed\n");
		else
			printf("recv failed with error: %d\n", WSAGetLastError());
		//memset(&recvbuf, 0, sizeof(recvbuf));
	} while (iResult > 0);
	closesocket(*ClientSocket);
	delete ClientSocket;
	return 0;
}

int main()
{
	// -------------------------------------THREAD VARIABLES----------------------------------------
	CLIENTDATA 	myClientData = new ThreadData();
	HANDLE		hThreadArray[MAX_CLIENTS];
	DWORD		dwThreadIdArray[MAX_CLIENTS];
	

	// ----------------------------------------------------------------------------------------------

	WSADATA wsaData;
	int iResult;
	SOCKET ListenSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL;
	struct addrinfo hints;


	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	SecureZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	for (int i = 0; i < MAX_CLIENTS; ++i) {
		SOCKET * sckt = new SOCKET;
		*sckt = accept(ListenSocket, NULL, NULL);
		if (*sckt == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		hThreadArray[i] = CreateThread(		
			NULL,
			0,
			rcv_msg,
			sckt,
			0,
			&dwThreadIdArray[i]);
		
	}

	WaitForMultipleObjects(MAX_CLIENTS, hThreadArray, TRUE, INFINITE);
	closesocket(ListenSocket);
	
	WSACleanup();
	for (int i = 0; i<MAX_CLIENTS; i++)
	{
		CloseHandle(hThreadArray[i]);
		if (myClientData != NULL)
		{
			delete myClientData;
			myClientData = NULL;    // Ensure address is not reused.
		}
	}

    return 0;
}

