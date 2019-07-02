#include <cstdio>
#include <cstdlib>
#include <WinSock2.h>
#include <Windows.h>
#include "ServerManager.h"

int main(int argc, char* argv[])
{
	//if (argc != 2) {
	//	printf("Usage: %s <Port>\n", argv[0]);
	//	exit(1);
	//}

	ServerManager& servManager = ServerManager::getInstance();
	servManager.Start();

	return 0;
}