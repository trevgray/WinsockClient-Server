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

	while (true) {
		// Write out to that socket
		std::string s;
		std::cin >> s;

		int sendOk = sendto(out, s.c_str(), s.size() + 1, 0, (sockaddr*)&server, sizeof(server));
		if (sendOk == SOCKET_ERROR) {
			std::cout << "Send Error: " << WSAGetLastError() << std::endl;
		}

		messageArray.push_back(s);
		if (s == "SEND") {
			for (std::string message : messageArray) {
				if (message == "SEND") { break; }

				ZeroMemory(&server, serverLength); // Clear the client structure
				ZeroMemory(buf, 1024); // Clear the receive buffer

				// Wait for message
				int bytesIn = recvfrom(out, buf, 1024, 0, (sockaddr*)&server, &serverLength);
				if (bytesIn == SOCKET_ERROR) {
					std::cout << "Error receiving from client " << WSAGetLastError() << std::endl;
					continue;
				}
				std::cout << buf << std::endl;
			}
		}
	}

	closesocket(out);
	WSACleanup();
}