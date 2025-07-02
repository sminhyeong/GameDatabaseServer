#include "WorkerThread.h"
#include <iostream>
#include <chrono>
#include <vector>

WorkerThread::WorkerThread(SOCKET ClientSocket)
    : _do_thread(true), _head(nullptr), _tail(nullptr), _client_count(0)
{
    // 첫 번째 클라이언트 추가
    SocketNode* newNode = new SocketNode(ClientSocket);
    _head.store(newNode);
    _tail.store(newNode);
    _client_count.store(1);

    // 스레드 시작
    _thread = std::make_unique<std::thread>(&WorkerThread::RunOnServerThread, this);
}

WorkerThread::~WorkerThread()
{
    StopThread();

    if (_thread && _thread->joinable()) {
        _thread->join();
    }

    // 남은 소켓들 정리
    SocketNode* current = _head.load();
    while (current != nullptr) {
        SocketNode* next = current->next.load();
        closesocket(current->socket);
        delete current;
        current = next;
    }
}

void WorkerThread::AddClient(SOCKET clientSocket)
{
    if (_client_count.load() >= MAX_CLIENT_COUNT) {
        closesocket(clientSocket);
        return;
    }

    SocketNode* newNode = new SocketNode(clientSocket);

    // Lock-free tail insertion
    while (true) {
        SocketNode* tail = _tail.load();
        SocketNode* next = tail->next.load();

        if (tail == _tail.load()) {  // tail이 변경되지 않았는지 확인
            if (next == nullptr) {
                // tail의 next를 새 노드로 설정 시도
                if (tail->next.compare_exchange_weak(next, newNode))
                {
                    // 성공하면 tail을 새 노드로 업데이트
                    _tail.compare_exchange_weak(tail, newNode);
                    _client_count.fetch_add(1);
                    printf("Add Client %d\n", _client_count.load());
                    break;
                }
            }
            else {
                // tail이 실제 마지막이 아니면 tail을 앞으로 이동 시도
                _tail.compare_exchange_weak(tail, next);
            }
        }
    }
}

void WorkerThread::RemoveSocketFromList(SOCKET target_socket)
{
    SocketNode* prev = nullptr;
    SocketNode* current = _head.load();

    while (current != nullptr) {
        if (current->socket == target_socket) {
            SocketNode* next = current->next.load();

            if (prev == nullptr) {
                // 첫 번째 노드 제거
                if (_head.compare_exchange_weak(current, next)) {
                    if (next == nullptr) {
                        _tail.store(nullptr);
                    }
                    closesocket(current->socket);
                    delete current;
                    _client_count.fetch_sub(1);
                    return;
                }
            }
            else {
                // 중간 또는 마지막 노드 제거
                if (prev->next.compare_exchange_weak(current, next)) {
                    if (current == _tail.load()) {
                        _tail.store(prev);
                    }
                    closesocket(current->socket);
                    delete current;
                    _client_count.fetch_sub(1);
                    return;
                }
            }
        }
        prev = current;
        current = current->next.load();
    }
}

void WorkerThread::RunOnServerThread()
{
    char buffer[1024];
    printf("Create WorkerThread\n");

    while (_do_thread.load()) {
        if (_client_count.load() == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        fd_set readSet;
        FD_ZERO(&readSet);

        SOCKET maxSock = 0;
        std::vector<SOCKET> socketList;  // 현재 처리할 소켓 목록

        // 현재 연결된 소켓들을 복사 (lock-free 순회)
        SocketNode* current = _head.load();
        while (current != nullptr) {
            SOCKET s = current->socket;
            socketList.push_back(s);
            FD_SET(s, &readSet);
            if (s > maxSock) maxSock = s;
            current = current->next.load();
        }

        if (socketList.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000; // 10ms

        int result = select(0, &readSet, NULL, NULL, &timeout);

        if (result == SOCKET_ERROR) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // 이벤트 발생한 소켓들 처리
        for (SOCKET s : socketList) {
            if (FD_ISSET(s, &readSet)) {
                int len = recv(s, buffer, sizeof(buffer) - 1, 0);
                if (len <= 0) {
                    // 클라이언트 연결 종료
                    RemoveSocketFromList(s);
                    printf("Client disconnected. Remaining: %d\n", _client_count.load());
                }
                else {
                    // 메시지 수신 및 echo back
                    buffer[len] = '\0';
                    send(s, buffer, len, 0);
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    printf("WorkerThread finished\n");
}

void WorkerThread::StopThread()
{
    _do_thread.store(false);
}