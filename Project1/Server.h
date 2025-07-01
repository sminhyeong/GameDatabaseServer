#pragma once
#include <vector>
#include <string>
#include <thread>
#include <functional>
#include <winSock2.h>
#include<mutex>

class WorkerThread;

#pragma comment(lib, "ws2_32.lib")

class Server
{
private :
	Server();
	static Server* instance;
	
	bool _is_running;
	
	std::vector<WorkerThread*> _worker_threads;
	SOCKET _server_sock;
	WSAData _wsa_data;

	std::thread _thread_cleaner;
	std::mutex _worker_threads_mutex; // ���� �߰�: _user_fds ��ȣ�� ���ؽ�

public :
	static Server* Instance();
	~Server();
	
	void Initialized(const char* ip ,int port);
	void Run();
	void Destroy();
	int FindCanUseWorkerThread();
	void ClearDeathThread();
	void Thread_ClearDeadWorkerLoop();
};
