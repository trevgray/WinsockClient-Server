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
		iClientResult = recv(clientSocket, (char*)&clientChatBuffer, sizeof(ActorBuffer), 0);
		if (iClientResult > 0) {

			printf("%f %f %f\n", clientActorBuffer.position.x, clientActorBuffer.position.y, clientActorBuffer.position.z);
			EngineManager::Instance()->GetActorManager()->GetActor<Actor>(std::to_string(clientActorBuffer.ID))->GetComponent<TransformComponent>()->SetPosition(clientActorBuffer.position);
			EngineManager::Instance()->GetActorManager()->GetActor<Actor>(std::to_string(clientActorBuffer.ID))->GetComponent<TransformComponent>()->setOrientation(clientActorBuffer.orientation);

			clientActorBuffer.position = EngineManager::Instance()->GetActorManager()->GetActor<Actor>("Player")->GetComponent<TransformComponent>()->GetPosition();
			clientActorBuffer.orientation = EngineManager::Instance()->GetActorManager()->GetActor<Actor>("Player")->GetComponent<TransformComponent>()->GetQuaternion();

			//SEND MESSAGE TO ALL CLIENTS---------------------------------------

			clientSendbuf = (char*)&clientActorBuffer; //binary representation 

			iClientResult = send(clientSocket, clientSendbuf, sizeof(ActorBuffer), 0);
			if (iClientResult == SOCKET_ERROR) {
				std::cout << "Send failed with error: " << iClientResult << std::endl;
				break;
			}
		}
		else {
			std::cout << "Connection closing..." << std::endl;
			break;
		}
	}

	//shutdown the connection on our end
	iClientResult = shutdown(clientSocket, SD_SEND);
	if (iClientResult == SOCKET_ERROR) {
		std::cout << "Shutdown failed with error: " << WSAGetLastError() << std::endl;
		WSACleanup();
	}

	closesocket(clientSocket);
}

TCPChat::~TCPChat() {
	std::cout << ("Shutting Down the Chat") << std::endl;
	//shutdown the connection on our end
	iResult = shutdown(listenSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		std::cout << "Shutdown failed with error: " << WSAGetLastError() << std::endl;
	}

	std::cout << "Connection closed" << std::endl;

	closesocket(listenSocket);
	closesocket(connectSocket);
	WSACleanup();
}

bool TCPChat::Initialize(NetworkNode networkMode_) {
	networkMode = networkMode_;

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
			//listen
			iResult = listen(listenSocket, SOMAXCONN); //SOMAXCONN allows maximum number of connections
			if (iResult == SOCKET_ERROR) {
				std::cout << "Listen Socket failed with error: " << WSAGetLastError() << std::endl;
				this->~TCPChat();
				return;
			}
			std::cout << "Waiting for connection request..." << std::endl;

			while (connectSocket = accept(listenSocket, nullptr, nullptr)) {
				//Accept a client socket
				if (connectSocket == INVALID_SOCKET) {
					std::cout << "Accept failed with error: " << WSAGetLastError() << std::endl;
					this->~TCPChat();
					return;
				}

				std::cout << "Connected to client" << std::endl;

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
					std::cout << "Connected to server" << std::endl;

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

			//Receive until the peer closes connection
			actorBuffer.position = EngineManager::Instance()->GetActorManager()->GetActor<Actor>("Player")->GetComponent<TransformComponent>()->GetPosition();
			actorBuffer.orientation = EngineManager::Instance()->GetActorManager()->GetActor<Actor>("Player")->GetComponent<TransformComponent>()->GetQuaternion();
			actorBuffer.ID = actorID;

			sendbuf = (char*)&actorBuffer; //binary representation 

			iResult = send(connectSocket, sendbuf, sizeof(ActorBuffer), 0);
			if (iResult == SOCKET_ERROR) {
				std::cout << "Send failed with error: " << iResult << std::endl;
				this->~TCPChat();
				return;
			}

			iResult = recv(connectSocket, (char*)&actorBuffer, sizeof(ActorBuffer), 0);
			if (iResult > 0) {

				printf("%f %f %f\n", actorBuffer.position.x, actorBuffer.position.y, actorBuffer.position.z);
				EngineManager::Instance()->GetActorManager()->GetActor<Actor>("ServerActor")->GetComponent<TransformComponent>()->SetPosition(actorBuffer.position);
				EngineManager::Instance()->GetActorManager()->GetActor<Actor>("ServerActor")->GetComponent<TransformComponent>()->setOrientation(actorBuffer.orientation);
			}
			else {
				std::cout << "Receive failed with error: " << WSAGetLastError() << std::endl;
				this->~TCPChat();
				return;
			}
			//std::cout << "Bytes sent: " << iResult << std::endl;
		}
	}
}

Ref<TCPChat> TCPChat::instance = 0;