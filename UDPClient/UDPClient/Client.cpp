#include <iostream>
#include <WS2tcpip.h>
#include <vector>

// Include the Winsock library (lib) file
#pragma comment (lib, "ws2_32.lib")

void main(int argc, char* argv[]) {
	////////////////////////////////////////////////////////////
	// INITIALIZE WINSOCK
	////////////////////////////////////////////////////////////

	// Structure to store the WinSock version. This is filled in
	// on the call to WSAStartup()
	WSAData data;

	// To start WinSock, the required version must be passed to
	// WSAStartup(). This server is going to use WinSock version
	// 2 so I create a word that will store 2 and 2 in hex i.e.
	// 0x0202
	WORD version = MAKEWORD(2, 2); //version of winsock

	// Start WinSock
	int ws0k = WSAStartup(version, &data);
	if (ws0k != 0) { //winsock failed to start
		std::cout << "Can't start Winsock" << std::endl;
		return;
	}

	////////////////////////////////////////////////////////////
	// SOCKET CREATION AND BINDING
	////////////////////////////////////////////////////////////

	// Socket creation, note that the socket type is datagram
	SOCKET out = socket(AF_INET, SOCK_DGRAM, 0);

	char buf[1024];

	// Create a hint structure for the server
	sockaddr_in server;
	int serverLength = sizeof(server);
	server.sin_family = AF_INET; // AF_INET = IPv4 addresses
	server.sin_port = htons(5400); // Little to big endian conversion
	inet_pton(AF_INET, "127.0.0.1", &server.sin_addr); //"127.0.0.1" is your own ip - Convert from string to byte array

	std::vector<std::string> messageArray;
	bool messageCheck;

	std::cout << "Please enter Messages\n-SEND- submits all messages\n-EXIT- closes the client\n-------------------------------" << std::endl;

	////////////////////////////////////////////////////////////
	// MAIN LOOP SETUP AND ENTRY
	////////////////////////////////////////////////////////////

	while (true) {
		// Write out to that socket
		std::string s;
		std::cin >> s;

		messageArray.push_back(s);
		if (s == "SEND") {
			messageCheck = true;

			std::cout << "Sending all messages :" << std::endl;
			for (std::string message : messageArray) {
				int sendOk = sendto(out, message.c_str(), message.size() + 1, 0, (sockaddr*)&server, sizeof(server));
				if (sendOk == SOCKET_ERROR) {
					std::cout << "Send Error: " << WSAGetLastError() << std::endl;
				}
				if (message == "SEND") { continue; }
				std::cout << "Sending: " << message << std::endl;
			}

			std::cout << "Waiting for acknowledgment of reception for all messages (" << messageArray.size() - 1 << ")" << std::endl;

			int messageIterator = 1;
			for (std::string message : messageArray) {
				if (message == "SEND") { continue; }

				ZeroMemory(&server, serverLength); // Clear the client structure
				ZeroMemory(buf, 1024); // Clear the receive buffer

				// Wait for message
				int bytesIn = recvfrom(out, buf, 1024, 0, (sockaddr*)&server, &serverLength);
				if (bytesIn == SOCKET_ERROR) {
					std::cout << "Error receiving from server " << WSAGetLastError() << std::endl;
					continue;
				}

				// Get server info
				char serverIP[256]; // Create enough space to convert the address byte array
				ZeroMemory(serverIP, 256); // to string of characters

				// Convert from byte array to chars

				inet_ntop(AF_INET, &server.sin_addr, serverIP, 256);

				std::cout << "Message from [" << serverIP << "]: " << messageIterator << std::endl;
				messageIterator++;

				int receiveCheck = strcmp(buf, message.c_str());
				if (receiveCheck == 1) {
					messageCheck = false;
				}
			}

			messageArray.clear();

			if (messageCheck == true) {
				std::cout << "All messages received, confirmation sent!" << std::endl;
				std::string confirmation = "All good!";
				int sendOk = sendto(out, confirmation.c_str(), confirmation.size() + 1, 0, (sockaddr*)&server, sizeof(server));
				if (sendOk == SOCKET_ERROR) {
					std::cout << "Send Error: " << WSAGetLastError() << std::endl;
				}
			}
			else {
				std::cout << "Error receiving string from server" << std::endl;
			}
			std::cout << "Enter new messages or SEND / EXIT\n-------------------------------" << std::endl;
		}

		else if (s == "EXIT") {
			int sendOk = sendto(out, s.c_str(), s.size() + 1, 0, (sockaddr*)&server, sizeof(server));
			if (sendOk == SOCKET_ERROR) {
				std::cout << "Send Error: " << WSAGetLastError() << std::endl;
			}
			std::cout << "Disconnected from server" << std::endl;
			break;
		}
	}

	// Close socket
	closesocket(out);
	// Shutdown winsock
	WSACleanup();
}