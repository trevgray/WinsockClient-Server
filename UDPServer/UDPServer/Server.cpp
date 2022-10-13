//using namespace std;

#include <iostream>
#include <WS2tcpip.h>

// Include the Winsock library (lib) file
#pragma comment (lib, "ws2_32.lib")

// Main entry point into the server
void main() {
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
		std::cout << "Can't start Winsock" << ws0k << std::endl;
		return;
	}

	////////////////////////////////////////////////////////////
	// SOCKET CREATION AND BINDING
	////////////////////////////////////////////////////////////

	// Create a socket, notice that it is a user datagram socket (UDP)
	SOCKET in = socket(AF_INET, SOCK_DGRAM, 0); //SOCK_DGRAM is what makes it a udp socket

	// Create a server hint structure for the server
	sockaddr_in serverhint;
	serverHint.sin_addr.S_un.S_addr = ADDR_ANY; // Us any IP address available on the machine
	serverHint.sin_family = AF_INET; // Address format is IPv4
	serverhint.sin_port = htons(5400); //big endian system - most machines are big endian

	//Try and bind the socket to the IP and port
	if (bind(in, (sockaddr*)&serverhint, sizeof(serverhint)) == SOCKET_ERROR) { //bind the socket
		std::cout << "Can't bind to socket " << WSAGetLastError() << std::endl;
	}

	////////////////////////////////////////////////////////////
	// MAIN LOOP SETUP AND ENTRY
	////////////////////////////////////////////////////////////

	sockaddr_in client; //used to hold the client info
	int clientLength = sizeof(client); // The size of the client information
	
	char buf[1024];

	while (true) {
		ZeroMemory(&client, clientLength); // Clear the client structure
		ZeroMemory(buf, 1024); // Clear the receive buffer

		// Wait for message
		int bytesIn = recvfrom(in, buf, 1024, 0, (sockaddr*)&client, &clientLength);
		if (bytesIn == SOCKET_ERROR) {
			std::cout << "Error receiving from client " << WSAGetLastError() << std::endl;
			continue;
		}

		// Display message and client info
		char clientIP[256]; // Create enough space to convert the address byte array
		ZeroMemory(clientIP, 256); // to string of characters

		// Convert from byte array to chars
		inet_ntop(AF_INET, &client.sin_addr, clientIP, 256);

		// Display the message / who sent it
		std::cout << "Message received from [" << clientIP << "] > " << buf << std::endl;

	}

	// Close socket
	closesocket(in);

	// Shutdown winsock
	WSACleanup();
}