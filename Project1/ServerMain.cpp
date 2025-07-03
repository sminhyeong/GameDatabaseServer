#include <iostream>
#include <csignal>
#include "Server.h"
#include "mysql.h"

using namespace std;

std::atomic<bool> g_shutdown_requested(false);

void SignalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n[!] Shutdown signal received. Stopping server...\n";
        g_shutdown_requested.store(true);

        if (Server::Instance()) {
            Server::Instance()->Stop();
        }
    }
}

BOOL WINAPI ConsoleHandler(DWORD signal)
{
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
        std::cout << "\n[!] Console event detected. Cleaning up...\n";
        g_shutdown_requested.store(true);

        if (Server::Instance()) {
            Server::Instance()->Stop();
        }

        return TRUE;
    }
    return FALSE;
}

void Cleanup()
{
    std::cout << "[!] Cleanup called\n";
    if (Server::Instance()) {
        Server::Instance()->Destroy();
    }
}

int main()
{
    try {
        // 시그널 핸들러 설정
        std::signal(SIGINT, SignalHandler);
        std::signal(SIGTERM, SignalHandler);
        SetConsoleCtrlHandler(ConsoleHandler, TRUE);

        // 종료 시 정리 함수 등록
        std::atexit(Cleanup);

        Server* server = Server::Instance();

        std::cout << "Starting server on 127.0.0.1:7777...\n";
        server->Initialize("127.0.0.1", 7777);

        // 서버 실행
        server->Run();

        std::cout << "Server stopped gracefully\n";

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}