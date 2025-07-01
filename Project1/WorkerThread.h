#pragma once
#include <list>
#include <winSock2.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

#define MAX_CLIENT_COUNT 50

class WorkerThread
{
private:
	bool _do_thread;
	std::list<SOCKET> _user_fds;
	std::thread* thrad;
	void RunOnServerThread();

public:
	WorkerThread(SOCKET ClientSocket);
	~WorkerThread();
	void AddClient(SOCKET clientSocket);
	inline bool IsFullAccess()
	{
		return _user_fds.size() >= MAX_CLIENT_COUNT;
	}
	inline bool DoThread()
	{
		return _do_thread;
	}

};

