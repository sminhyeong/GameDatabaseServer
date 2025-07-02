#pragma once
#include <list>
#include <thread>
#include <atomic>
#include <memory>
#include "Packet.h"




#define MAX_CLIENT_COUNT 50

// Lock-free ��� ����ü
struct SocketNode {
    SOCKET socket;
    std::atomic<SocketNode*> next;

    SocketNode(SOCKET s) : socket(s), next(nullptr) {}
};

class WorkerThread
{
private:
    std::atomic<bool> _do_thread;
    std::atomic<SocketNode*> _head;  // Lock-free linked list head
    std::atomic<SocketNode*> _tail;  // Lock-free linked list tail
    std::atomic<int> _client_count;  // Ŭ���̾�Ʈ �� ����
    std::unique_ptr<std::thread> _thread;

    void RunOnServerThread();

    // Lock-free ����Ʈ���� ��� ���� (���������� ���)
    void RemoveSocketFromList(SOCKET target_socket);

public:
    WorkerThread(SOCKET ClientSocket);
    ~WorkerThread();

    void AddClient(SOCKET clientSocket);
    void StopThread();

    inline bool IsFullAccess() const {
        return _client_count.load() >= MAX_CLIENT_COUNT;
    }

    inline bool DoThread() const {
        return _do_thread.load();
    }

    inline int GetClientCount() const {
        return _client_count.load();
    }
};