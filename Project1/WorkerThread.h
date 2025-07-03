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

// Lock-free 노드 구조체
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
    std::atomic<int> _client_count;  // 클라이언트 수 추적
    std::unique_ptr<std::thread> _thread;

    LockFreeQueue<Task>* _task_queue;  // Task 큐 참조
    std::mutex _send_mutex;  // 전송 시 동기화용

    void RunOnServerThread();
    void ProcessClientData(SOCKET clientSocket, char* buffer, int bufferSize);

    // Lock-free 리스트에서 노드 제거 (내부용으로 사용)
    void RemoveSocketFromList(SOCKET target_socket);

public:
    // 기존 생성자 (하위 호환성)
    WorkerThread(SOCKET ClientSocket);

    // 새로운 생성자 (Task 큐 포함)
    WorkerThread(SOCKET ClientSocket, LockFreeQueue<Task>* taskQueue);

    ~WorkerThread();

    void AddClient(SOCKET clientSocket);
    void StopThread();

    // 클라이언트에게 데이터 전송
    bool SendToClient(SOCKET clientSocket, const std::vector<uint8_t>& data);

    // 특정 소켓이 이 워커에 속하는지 확인
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