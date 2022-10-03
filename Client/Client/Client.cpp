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
	//initialize Winsock
	WSADATA wsaData; //https://learn.microsoft.com/en-us/windows/win32/api/winsock/ns-winsock-wsadata
	int iResult; //check if the start up failed

	SOCKET connectSocket = INVALID_SOCKET;

	struct addrinfo* result = nullptr, * ptr = nullptr, hints;

	//initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2)/*make sure we use version 2.2*/, &wsaData); //
	if (iResult != 0) {
		std::cout << "WSAStartup failed: " << iResult << std::endl;
		system("pause");
		return 1;
	}

	//hints define the connection type and the address that will be used
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC; //don't specificity the address family (IVP6 / IVP 4)
	hints.ai_socktype = SOCK_STREAM; //TCP works like a stream
	hints.ai_protocol = IPPROTO_TCP;

	//Resolve the server address and port
	iResult = getaddrinfo("localhost", DEFAULT_PORT, &hints, &result); //localhost connections be to the own computer - this is where you enter the ip
	{ //result gets full of how to connection to the computer
		if (iResult != 0) {
			std::cout << "getaddrinfo failed with error: " << iResult << std::endl;
			WSACleanup();
			system("pause");
			return 1;
		}
	}

	std::cout << "Attempting to connect to server" << std::endl;

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) { //move addresses till one succeeds
		//Create a socket for connecting to the server
		connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol); //find a open socket
		if (connectSocket == INVALID_SOCKET) {
			std::cout << "Socket failed with error: " << iResult << std::endl;
			WSACleanup();
			system("pause");
			return 1;
		}

		//Connect to the server
		iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(connectSocket);
			connectSocket = INVALID_SOCKET;
			continue;
		}
		std::cout << "Connected to server" << std::endl;
		break;
	}

	freeaddrinfo(result);
	if (connectSocket == INVALID_SOCKET) {
		std::cout << "Unable to connect to the server: " << iResult << std::endl;
		WSACleanup();
		system("pause");
		return 1;
	}

	//Receive until the peer closes connection
	std::string message;
	do {
		message.clear();
		std::cout << "Input a sting to send (!exit to disconnect): ";
		std::cin >> message;
		int messageSize = strlen(message.c_str());
		//Send an initial buffer
		iResult = send(connectSocket, message.c_str(), messageSize, 0);
		if (iResult == SOCKET_ERROR) {
			std::cout << "Send failed with error: " << iResult << std::endl;
			closesocket(connectSocket);
			WSACleanup();
			system("pause");
			return 1;
		}
		std::cout << "Bytes sent: " << iResult << std::endl;
	} while (message != "!exit");

	std::cout << "User typed '!exit' - Disconnecting from server..." << std::endl;
	std::cout << "Connection closed" << std::endl;

	//shutdown the connection
	iResult = shutdown(connectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		std::cout << "Shutdown failed with error: " << WSAGetLastError() << std::endl;
		closesocket(connectSocket);
		WSACleanup();
		system("pause");
		return 1;
	}

	closesocket(connectSocket);
	WSACleanup();

	return 0;
}