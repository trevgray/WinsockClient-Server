#include "TCPChat.h"

int main(int argc, char* args[]) {
	TCPChat::Instance()->Initialize(argc, args);
	TCPChat::Instance()->Run();
	return true;
}