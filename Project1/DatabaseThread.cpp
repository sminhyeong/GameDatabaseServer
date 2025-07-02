#include "DatabaseThread.h"

DatabaseThread::DatabaseThread(LockFreeQueue<Task>* InRecvQueue, LockFreeQueue<DBResponse>* InSendQueue) : RecvQueue(InRecvQueue), SendQueue(InSendQueue)
{

	_is_running = false;
}

DatabaseThread::~DatabaseThread()
{
}

bool DatabaseThread::ConnectDB()
{
	return _sql_connector->connect("127.0.0.1", "root", "projectc", "gamedb");
}
