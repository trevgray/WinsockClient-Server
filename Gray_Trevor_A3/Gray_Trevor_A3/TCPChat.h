#pragma once
#include <memory>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h> //include after WinSock2.h

#include <iostream>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT "27015"
//#define DEFAULT_BUFFER_LENGTH 512

enum NetworkNode {
	Server, Client
};

struct ChatBuffer {
	//ChatBuffer() {}
	char username[50];
	char message[512];
};

template<typename T> using Ref = std::shared_ptr<T>;
class TCPChat {
public:
	static Ref<TCPChat> Instance() {
		if (!instance) {
			instance = Ref<TCPChat>(new TCPChat);
		}
		return instance;
	}
	~TCPChat();

	bool Initialize(int argc, char* args[]);

	void Run();

private:
	TCPChat();
	static Ref<TCPChat> instance;

	std::vector<SOCKET> clientSockets;

	//Client Variables & Functions
	//unsigned int actorID;
	bool running;

	//Server Variables & Functions
	void AddClientSession(void* data);
	void ServerInput();

	//Client Variables & Functions
	void ReceiveServerMessages();


	//server socket
	SOCKET listenSocket;

	//General Networking Variables
	ChatBuffer chatBuffer;
	NetworkNode networkMode;

	WSADATA wsaData; //https://learn.microsoft.com/en-us/windows/win32/api/winsock/ns-winsock-wsadata
	int iResult; //check if the start up failed

	struct addrinfo* result, * ptr, hints;

	//connection socket
	SOCKET connectSocket;

	char* sendbuf;
	int iSendResult;
};

