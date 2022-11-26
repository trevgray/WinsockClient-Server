#include "TCPChat.h"

int main(int argc, char* args[]) {
	TCPChat::Instance()->Initialize(Client);
	TCPChat::Instance()->Run();
	return true;
}