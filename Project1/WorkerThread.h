#pragma once
#include <list>
#include <thread>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include "Packet.h"

template<typename T>
class LockFreeQueue;

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

    LockFreeQueue<Task>* _task_queue;  // Task ť ����
    std::mutex _send_mutex;  // ���� �� ����ȭ��

    void RunOnServerThread();
    void ProcessClientData(SOCKET clientSocket, char* buffer, int bufferSize);

    // Lock-free ����Ʈ���� ��� ���� (���ο����� ���)
    void RemoveSocketFromList(SOCKET target_socket);

public:
    // ���� ������ (���� ȣȯ��)
    WorkerThread(SOCKET ClientSocket);

    // ���ο� ������ (Task ť ����)
    WorkerThread(SOCKET ClientSocket, LockFreeQueue<Task>* taskQueue);

    ~WorkerThread();

    void AddClient(SOCKET clientSocket);
    void StopThread();

    // Ŭ���̾�Ʈ���� ������ ����
    bool SendToClient(SOCKET clientSocket, const std::vector<uint8_t>& data);

    // Ư�� ������ �� ��Ŀ�� ���ϴ��� Ȯ��
    bool HasClient(SOCKET clientSocket) const;

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