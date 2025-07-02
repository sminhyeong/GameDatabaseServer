#pragma once
#include <vector>
#include <string>
#include <thread>
#include <functional>
#include <mutex>
#include <atomic>
#include <memory>
#include "LockFreeQueue.h"
#include "Packet.h"


class WorkerThread;
class DatabaseThread;

class Server
{
private:
    Server();
    static std::unique_ptr<Server> instance;
    static std::mutex instance_mutex;

    std::atomic<bool> _is_running;

    std::vector<std::unique_ptr<WorkerThread>> _worker_threads;
    SOCKET _server_sock;
    WSAData _wsa_data;

    std::unique_ptr<std::thread> _thread_cleaner;
    std::unique_ptr<DatabaseThread> _database_thread;
    std::mutex _worker_threads_mutex;  // WorkerThread 벡터 보호용

    LockFreeQueue<Task> RecvPakets;
    LockFreeQueue<DBResponse> SendPackets;

    // Non-blocking accept를 위한 설정
    void SetNonBlocking(SOCKET sock);

public:
    static Server* Instance();
    ~Server();

    void Initialize(const char* ip, int port);
    void Run();
    void Stop();
    void Destroy();

    int FindCanUseWorkerThread();
    void ClearDeadThreads();
    void Thread_ClearDeadWorkerLoop();

    bool IsRunning() const { return _is_running.load(); }
};