#include <iostream>
#include "Server.h"

void Cleanup()
{
	if (Server::Instance())
	{
		Server::Instance()->Destroy();
		delete Server::Instance(); // or Server::Instance()->Shutdown();
	}
}

BOOL WINAPI ConsoleHandler(DWORD signal)
{
	if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT)
	{
		std::cout << "\n[!] Ctrl+C or console closed detected. Cleaning up...\n";
		if (Server::Instance())
		{
			Server::Instance()->Destroy();
			delete Server::Instance(); // �Ҹ��� ȣ��
		}

		return TRUE; // ó�� �Ϸ�
	}
	return FALSE;
}

int main()
{
	Server* server = Server::Instance();
	server->Initialized((char*)"127.0.0.1", 7777);
	std::atexit(Cleanup); // exit�� ����� ���� ���
	SetConsoleCtrlHandler(ConsoleHandler, TRUE);//���� ���� ���� �ڵ� ���
	server->Run();
	server->Destroy();

	return 0;
}