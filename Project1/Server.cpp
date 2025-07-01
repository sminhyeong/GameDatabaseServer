#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "Server.h"
#include "WorkerThread.h"
#include<thread>


Server::Server()
{
	_wsa_data = WSAData();
	instance = nullptr;
	_server_sock = INVALID_SOCKET;
	_is_running = false;
}

Server* Server::instance = nullptr;

Server* Server::Instance()
{
	if (instance == nullptr)
	{
		instance = new Server();
	}
	return instance;
}

Server::~Server()
{
	_is_running = false;

	if (_thread_cleaner.joinable())
		_thread_cleaner.join();

	std::lock_guard<std::mutex> lock(_worker_threads_mutex);
	for (WorkerThread* worker : _worker_threads) {
		delete worker;
	}
	_worker_threads.clear();

	// 서버 소켓 닫기
	if (_server_sock != INVALID_SOCKET) {
		closesocket(_server_sock);
	}
	WSACleanup(); // WSAStartup에 대한 WSACleanup 호출
}

void Server::Initialized(const char* ip, int port)
{
	int Result = WSAStartup(MAKEWORD(2, 2), &_wsa_data);
	if (Result != 0)
	{
		_is_running = false;
		printf("Failed WSASTartUp\n");
		exit(-1);
	}
	_server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_server_sock == INVALID_SOCKET)
	{
		_is_running = false;
		printf("Failed ServerSocket Create\n");
		exit(-1);
	}

	sockaddr_in serverSockAddr;
	memset(&serverSockAddr, 0, sizeof(serverSockAddr));
	serverSockAddr.sin_addr.s_addr = inet_addr(ip);
	serverSockAddr.sin_family = PF_INET;
	serverSockAddr.sin_port = htons(port);

	Result = bind(_server_sock, (sockaddr*)&serverSockAddr, sizeof(serverSockAddr));
	if (Result == SOCKET_ERROR)
	{
		_is_running = false;
		printf("Failed ServerSocket Bind\n");
		exit(-1);
	}

	Result = listen(_server_sock, 10);
	if (Result == SOCKET_ERROR)
	{
		_is_running = false;
		printf("Failed ServerSocket Listen\n");
		exit(-1);
	}
	_is_running = true;
}

void Server::Run()
{
	if (!_is_running) return;

	_thread_cleaner = std::thread(&Server::Thread_ClearDeadWorkerLoop, this);

	while (true)
	{
		sockaddr_in clientSockAddr;
		int clientSockAddrSize = sizeof(clientSockAddr);
		memset(&clientSockAddr, 0, sizeof(clientSockAddr));
		SOCKET clientSock = accept(_server_sock, (sockaddr*)&clientSockAddr, &clientSockAddrSize);
		
		std::lock_guard<std::mutex> lock(_worker_threads_mutex); // <<<< 여기에 뮤텍스
		
		if (clientSock == INVALID_SOCKET)
		{
			printf("Failed ClientSocket Accept\n");
			continue;
		}

		int canUseThredIdx = FindCanUseWorkerThread();
		if (canUseThredIdx == -1)
		{

			_worker_threads.push_back(new WorkerThread(clientSock));
			printf("Connect Client %d\n", (int)_worker_threads.size());
			continue;
		}

		_worker_threads[canUseThredIdx]->AddClient(clientSock);
		printf("Connect Client %d\n", (int)_worker_threads.size());
	}
}

void Server::Destroy()
{
	if (!_is_running) return;
	if (instance != nullptr)
	{
		Server* temp = instance;
		instance = nullptr;
		delete temp;
	}
}

int Server::FindCanUseWorkerThread()
{
	if (_worker_threads.size() <= 0)
		return -1;
	int idx = 0;

	for (auto iter = _worker_threads.begin(); iter != _worker_threads.end(); iter++)
	{
		if (!(*iter)->IsFullAccess() && (*iter)->DoThread())
			return idx;
		idx++;
	}
	return -1;
}

void Server::ClearDeathThread()
{
	std::lock_guard<std::mutex> lock(_worker_threads_mutex);
	
	for (auto iter = _worker_threads.begin(); iter != _worker_threads.end();)
	{
		if (!(*iter)->DoThread())
		{
			iter = _worker_threads.erase(iter);
			printf("Destroy WorkerThread\n");
			continue;
		}
		iter++;
	}
}

void Server::Thread_ClearDeadWorkerLoop()
{
	while (_is_running) // 또는 true
	{
		std::this_thread::sleep_for(std::chrono::seconds(2)); // 5초마다

		ClearDeathThread(); // 죽은 워커 스레드 정리
	}
}