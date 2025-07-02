#include "WorkerThread.h"
#include <iostream>
#include <chrono>
#include <vector>

WorkerThread::WorkerThread(SOCKET ClientSocket)
    : _do_thread(true), _head(nullptr), _tail(nullptr), _client_count(0)
{
    // ù ��° Ŭ���̾�Ʈ �߰�
    SocketNode* newNode = new SocketNode(ClientSocket);
    _head.store(newNode);
    _tail.store(newNode);
    _client_count.store(1);

    // ������ ����
    _thread = std::make_unique<std::thread>(&WorkerThread::RunOnServerThread, this);
}

WorkerThread::~WorkerThread()
{
    StopThread();

    if (_thread && _thread->joinable()) {
        _thread->join();
    }

    // ���� ���ϵ� ����
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

        if (tail == _tail.load()) {  // tail�� ������� �ʾҴ��� Ȯ��
            if (next == nullptr) {
                // tail�� next�� �� ���� ���� �õ�
                if (tail->next.compare_exchange_weak(next, newNode))
                {
                    // �����ϸ� tail�� �� ���� ������Ʈ
                    _tail.compare_exchange_weak(tail, newNode);
                    _client_count.fetch_add(1);
                    printf("Add Client %d\n", _client_count.load());
                    break;
                }
            }
            else {
                // tail�� ���� �������� �ƴϸ� tail�� ������ �̵� �õ�
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
                // ù ��° ��� ����
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
                // �߰� �Ǵ� ������ ��� ����
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
        std::vector<SOCKET> socketList;  // ���� ó���� ���� ���

        // ���� ����� ���ϵ��� ���� (lock-free ��ȸ)
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

        // �̺�Ʈ �߻��� ���ϵ� ó��
        for (SOCKET s : socketList) {
            if (FD_ISSET(s, &readSet)) {
                int len = recv(s, buffer, sizeof(buffer) - 1, 0);
                if (len <= 0) {
                    // Ŭ���̾�Ʈ ���� ����
                    RemoveSocketFromList(s);
                    printf("Client disconnected. Remaining: %d\n", _client_count.load());
                }
                else {
                    // �޽��� ���� �� echo back
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