#pragma once
#include <iostream>
#include <memory>
#include "Packet.h"
#include "LockFreeQueue.h"
#include "MySqlConnector.h"

class DatabaseThread
{
private:
	bool _is_running;
	void Run();
	LockFreeQueue<Task>* RecvQueue;
	LockFreeQueue<DBResponse>* SendQueue;
	std::unique_ptr<MySQLConnector> _sql_connector;
public:
	DatabaseThread(LockFreeQueue<Task>* RecvQueue, LockFreeQueue<DBResponse>* SendQueue);
	~DatabaseThread();

	bool ConnectDB();


};