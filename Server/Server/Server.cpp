#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN


#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h> //include after WinSock2.h

#include <iostream>
#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFFER_LENGTH 512

int main() {
	WSADATA wsaData; //https://learn.microsoft.com/en-us/windows/win32/api/winsock/ns-winsock-wsadata
	int iResult; //check if the start up failed

	struct addrinfo* result = nullptr, * ptr = nullptr, hints;

	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET clientSocket = INVALID_SOCKET;

	char recvbuf[DEFAULT_BUFFER_LENGTH];
	int iSendResult;

	//initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2)/*make sure we use version 2.2*/, &wsaData); //
	if (iResult != 0) {
		std::cout << "WSAStartup failed: " << iResult << std::endl;
		system("pause");
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM; //TCP works like a stream
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	//Resolve the server address and port
	iResult = getaddrinfo(nullptr, DEFAULT_PORT, &hints, &result);
	{
		if (iResult != 0) {
			std::cout << "getaddrinfo failed with error: " << iResult << std::endl;
			WSACleanup();
			system("pause");
			return 1;
		}
	}

	//Create a socket for connecting to the server
	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listenSocket == INVALID_SOCKET) {
		std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		WSACleanup();
		system("pause");
		return 1;
	}

	//Setup the TCP listening socket
	iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		std::cout << "Bind failed with error: " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		closesocket(listenSocket);
		WSACleanup();
		system("pause");
		return 1;
	}
	freeaddrinfo(result);

	//listen
	iResult = listen(listenSocket, SOMAXCONN); //SOMAXCONN allows maximum number of connections
	if (iResult == SOCKET_ERROR) {
		std::cout << "Listen Socket failed with error: " << WSAGetLastError() << std::endl;
		closesocket(listenSocket);
		WSACleanup();
		system("pause");
		return 1;
	}

	std::cout << "Waiting for connection request..." << std::endl;
	
	//Accept a client socket
	clientSocket = accept(listenSocket, nullptr, nullptr);
	if (clientSocket == INVALID_SOCKET) {
		std::cout << "Accept failed with error: " << WSAGetLastError() << std::endl;
		closesocket(listenSocket);
		WSACleanup();
		system("pause");
		return 1;
	}

	std::cout << "Connected to client" << std::endl;

	closesocket(listenSocket);

	//receive until the peer shutdown the connect
	do {
		for (int x = 0; x < DEFAULT_BUFFER_LENGTH; x++) {
			recvbuf[x] = 0;
		}
		iResult = recv(clientSocket, recvbuf, DEFAULT_BUFFER_LENGTH, 0);
		if (iResult > 0) {
			std::cout << "Received string: " << recvbuf << std::endl;
		}
		else if (iResult == 0) {
			std::cout << "Connection closing..." << std::endl;
			break;
		}
		else {
			std::cout << "Receive failed with error: " << WSAGetLastError() << std::endl;
			closesocket(listenSocket);
			WSACleanup();
			system("pause");
			return 1;
		}
	} while (recvbuf != "!exit");

	//shutdown the connection on our end
	iResult = shutdown(clientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		std::cout << "Shutdown failed with error: " << WSAGetLastError() << std::endl;
		closesocket(listenSocket);
		WSACleanup();
		system("pause");
		return 1;
	}

	std::cout << "Connection closed" << std::endl;

	closesocket(clientSocket);
	WSACleanup();

	return 0;
}