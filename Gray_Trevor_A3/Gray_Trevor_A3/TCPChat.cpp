#include "TCPChat.h"
#include <thread>

TCPChat::TCPChat() {
	networkMode = Client;

	running = true;

	wsaData = WSADATA();
	iResult = true;
	iSendResult = true;

	result = nullptr;
	ptr = nullptr;
	hints = addrinfo();
	//server sockets
	listenSocket = INVALID_SOCKET;
	//client sockets
	connectSocket = INVALID_SOCKET;

	sendbuf = NULL;

	chatBuffer = ChatBuffer();
}

void TCPChat::AddClientSession(void* data) {
	SOCKET clientSocket = (SOCKET)data;
	ChatBuffer clientChatBuffer;
	char* clientSendbuf;
	int iClientResult;

	while (running == true) {
		ZeroMemory(&clientChatBuffer.username, 50);
		ZeroMemory(&clientChatBuffer.message, 512);

		iClientResult = recv(clientSocket, (char*)&clientChatBuffer, sizeof(ChatBuffer), 0);
		if (iClientResult > 0) {
			std::cout << clientChatBuffer.username << ": " << clientChatBuffer.message << std::endl;
			clientSendbuf = (char*)&clientChatBuffer; //binary representation 
			//SEND MESSAGE TO ALL CLIENTS---------------------------------------
			for (SOCKET socket : clientSockets) {
				if (socket != clientSocket) {
					iClientResult = send(socket, clientSendbuf, sizeof(ChatBuffer), 0);
				}
				if (iClientResult == SOCKET_ERROR) {
					std::cout << "Send failed with error: " << iClientResult << std::endl;
					break;
				}
			}
		}
		else {
			std::cout << "User Disconnected" << std::endl;
			break;
		}
	}

	//shutdown the connection on our end
	iClientResult = shutdown(clientSocket, SD_SEND);
	if (iClientResult == SOCKET_ERROR) {
		std::cout << "Shutdown failed with error: " << WSAGetLastError() << std::endl;
		WSACleanup();
	}

	for (int clientIterator = 0; clientIterator < clientSockets.size(); clientIterator++) {
		if (clientSockets[clientIterator] == clientSocket) {
			closesocket(clientSockets[clientIterator]);
			clientSockets.erase(clientSockets.begin() + clientIterator);
			break;
		}
	}

	//closesocket(clientSocket);
}

TCPChat::~TCPChat() {
	running = false;
	//shutdown the connection on our end
	if (networkMode == Server) {
		closesocket(listenSocket);
		for (SOCKET socket : clientSockets) {
			closesocket(socket);
		}
	}
	if (networkMode == Client) {
		closesocket(connectSocket);
	}

	std::cout << "Connection closed" << std::endl;

	WSACleanup();
}

bool TCPChat::Initialize(int argc, char* args[]) {
	if (argc == 1) {
		networkMode = Server;
	}
	else if (argc == 2) {
		networkMode = Client;
		ZeroMemory(chatBuffer.username, 50);
		for (int usernameTransfer = 0; usernameTransfer < 50; usernameTransfer++) {
			chatBuffer.username[usernameTransfer] = args[1][usernameTransfer];
		}
	}
	else {
		std::cout << "Invalid parameters" << std::endl;
		std::cout << "If the program is ran with no arguments, the program will run as the server" << std::endl;
		std::cout << "If the program is ran with 1 argument, the program will run as the client, where argument[1] is client name" << std::endl;
		std::cout << "THE PROGRAM WILL NOW TERMINATE" << std::endl;
		running = false;
		return running;
	}

	//initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2)/*make sure we use version 2.2*/, &wsaData);
	if (iResult != 0) {
		std::cout << "WSAStartup failed: " << iResult << std::endl;
		system("pause");
		return false;
	}

	else if (networkMode == Server) { //set up the instance as a server
		//hints define the connection type and the address that will be used
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM; //TCP works like a stream
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		//Resolve the server address and port
		iResult = getaddrinfo(nullptr, DEFAULT_PORT, &hints, &result);
		if (iResult != 0) {
			std::cout << "getaddrinfo failed with error: " << iResult << std::endl;
			this->~TCPChat();
			return false;
		}

		//Create a socket for connecting to the server
		listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (listenSocket == INVALID_SOCKET) {
			std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
			freeaddrinfo(result);
			this->~TCPChat();
			return false;
		}

		//Setup the TCP listening socket
		iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			std::cout << "Bind failed with error: " << WSAGetLastError() << std::endl;
			freeaddrinfo(result);
			this->~TCPChat();
			return false;
		}
		freeaddrinfo(result);
	}
	else if (networkMode == Client) {
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
				this->~TCPChat();
				return false;
			}
		}
	}
	return true;
}

void TCPChat::Run() {
	while (running == true) {
		if (networkMode == Server) {
			std::thread inputThread(&TCPChat::ServerInput, this);
			inputThread.detach();
			//listen
			iResult = listen(listenSocket, SOMAXCONN); //SOMAXCONN allows maximum number of connections
			if (iResult == SOCKET_ERROR) {
				std::cout << "Listen Socket failed with error: " << WSAGetLastError() << std::endl;
				//this->~TCPChat();
				return;
			}
			std::cout << "Server Open" << std::endl;

			while (connectSocket = accept(listenSocket, nullptr, nullptr)) {
				//Accept a client socket
				if (connectSocket == INVALID_SOCKET) {;
					return;
				}
				//GET USERNAME FROM CLIENT
				ChatBuffer newClient;
				ZeroMemory(newClient.username, 50);
				iResult = recv(connectSocket, (char*)&newClient, sizeof(ChatBuffer), 0);
				if (iResult > 0) {
					std::cout << newClient.username << " Connected" << std::endl;
				}
				else {
					std::cout << "Connection closing..." << std::endl;
					return;
				}

				//ADD IT TO CLIENT THREADS
				clientSockets.push_back(connectSocket);

				//create a thread and start it
				std::thread clientThread(&TCPChat::AddClientSession, this, (void*)connectSocket);
				clientThread.detach();
			}
		}
		else if (networkMode == Client) {

			if (connectSocket == INVALID_SOCKET) {
				for (ptr = result; ptr != NULL; ptr = ptr->ai_next) { //move addresses till one succeeds
					//Create a socket for connecting to the server
					connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol); //find a open socket
					if (connectSocket == INVALID_SOCKET) {
						std::cout << "Socket failed with error: " << iResult << std::endl;
						this->~TCPChat();
						return;
					}

					//Connect to the server
					iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
					if (iResult == SOCKET_ERROR) {
						closesocket(connectSocket);
						connectSocket = INVALID_SOCKET;
						continue;
					}

					//SEND USERNAME
					sendbuf = (char*)&chatBuffer;
					iResult = send(connectSocket, sendbuf, sizeof(ChatBuffer), 0);
					if (iResult == SOCKET_ERROR) {
						std::cout << "Send failed with error: " << iResult << std::endl;
						this->~TCPChat();
						return;
					}
					//-------------------------

					std::cout << "Connected to server" << std::endl;

					//SPIN A THREAD HERE TO RECIEVE MESSAGES FROM THE SERVER
					std::thread serverMessageThread(&TCPChat::ReceiveServerMessages, this);
					serverMessageThread.detach();

					break;
				}

				freeaddrinfo(result);
				if (connectSocket == INVALID_SOCKET) {
					std::cout << "Unable to connect to the server: " << iResult << std::endl;
					this->~TCPChat();
					return;
				}
			}

			//CIN THE MESSAGE HERE----------------------------------------
			std::string message;
			std::cin >> message;

			//LOGOFF
			if (message == "logoff") {
				std::cout << "Disconnecting From Server" << std::endl;
				this->~TCPChat();
				return;
			}

			//CHECK IF THE SERVER HAS BEEN DISCONNECTED
			if (running == false) {
				return;
			}

			int iterator = 0;
			ZeroMemory(&chatBuffer.message, 512);

			for (char messageChar : message) {
				chatBuffer.message[iterator] = messageChar;
				iterator++;
			}

			sendbuf = (char*)&chatBuffer; //binary representation 

			iResult = send(connectSocket, sendbuf, sizeof(ChatBuffer), 0);
			if (iResult == SOCKET_ERROR) {
				std::cout << "Send failed with error: " << iResult << std::endl;
				this->~TCPChat();
				return;
			}
		}
	}
}

void TCPChat::ReceiveServerMessages() {
	ChatBuffer printOutBuffer;
	while (running == true) {
		ZeroMemory(&printOutBuffer.username, 50);
		ZeroMemory(&printOutBuffer.message, 512);

		iResult = recv(connectSocket, (char*)&printOutBuffer, sizeof(ChatBuffer), 0);
		if (iResult > 0) {
			if (printOutBuffer.username[0] == ' ' && printOutBuffer.username[1] == '+' && printOutBuffer.message[0] == ' ' && printOutBuffer.message[1] == '+') { //server shutdown code
				std::cout << "Server Shutdown" << std::endl;
				running = false;
				break;
			}
			std::cout << printOutBuffer.username << ": " << printOutBuffer.message << std::endl;
		}
	}

}

void TCPChat::ServerInput() {
	while (running == true) {
		std::string command;
		std::cin >> command;

		if (command == "shutdown") {
			ChatBuffer clientChatBuffer;
			clientChatBuffer.username[0] = ' '; //the client is unable to make the first char in there name a space
			clientChatBuffer.username[1] = '+';
			clientChatBuffer.message[0] = ' ';
			clientChatBuffer.message[1] = '+';
			char* clientSendbuf = (char*)&clientChatBuffer; //binary representation 
			for (SOCKET socket : clientSockets) {
				int iClientResult = send(socket, clientSendbuf, sizeof(ChatBuffer), 0);
				if (iClientResult == SOCKET_ERROR) {
					std::cout << "Send failed with error: " << iClientResult << std::endl;
					break;
				}
			}

			running = false;
			this->~TCPChat();
			return;
		}
	}
}

Ref<TCPChat> TCPChat::instance = 0;