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
			delete Server::Instance(); // 소멸자 호출
		}

		return TRUE; // 처리 완료
	}
	return FALSE;
}

int main()
{
	Server* server = Server::Instance();
	server->Initialized((char*)"127.0.0.1", 7777);
	std::atexit(Cleanup); // exit로 종료시 실행 등록
	SetConsoleCtrlHandler(ConsoleHandler, TRUE);//강제 종료 조건 코드 등록
	server->Run();
	server->Destroy();

	return 0;
}