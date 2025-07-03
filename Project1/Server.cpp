#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "Server.h"
#include "WorkerThread.h"
#include <thread>
#include <iostream>
#include <chrono>
#include "DatabaseThread.h"

std::unique_ptr<Server> Server::instance = nullptr;
std::mutex Server::instance_mutex;

Server::Server()
    : _wsa_data(WSAData()), _server_sock(INVALID_SOCKET), _is_running(false)
{
}

Server* Server::Instance()
{
    std::lock_guard<std::mutex> lock(instance_mutex);
    if (instance == nullptr) {
        instance = std::unique_ptr<Server>(new Server());
    }
    return instance.get();
}

Server::~Server()
{
    Stop();

    // ���� ó�� ������ ���� ���
    if (_response_handler_thread && _response_handler_thread->joinable()) {
        _response_handler_thread->join();
    }

    // �����ͺ��̽� ������ ���� ���
    if (_database_thread) {
        _database_thread->Stop();
    }

    if (_thread_cleaner && _thread_cleaner->joinable()) {
        _thread_cleaner->join();
    }

    std::lock_guard<std::mutex> lock(_worker_threads_mutex);
    _worker_threads.clear();  // unique_ptr�̹Ƿ� �ڵ����� delete��

    // ���� ���� �ݱ�
    if (_server_sock != INVALID_SOCKET) {
        closesocket(_server_sock);
    }
    WSACleanup();
}

void Server::SetNonBlocking(SOCKET sock)
{
    u_long mode = 1;  // 1 for non-blocking, 0 for blocking
    ioctlsocket(sock, FIONBIO, &mode);
}

void Server::Initialize(const char* ip, int port)
{
    int result = WSAStartup(MAKEWORD(2, 2), &_wsa_data);
    if (result != 0) {
        printf("Failed WSAStartup: %d\n", result);
        throw std::runtime_error("WSAStartup failed");
    }

    _server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_server_sock == INVALID_SOCKET) {
        printf("Failed to create server socket: %d\n", WSAGetLastError());
        WSACleanup();
        throw std::runtime_error("Socket creation failed");
    }

    // SO_REUSEADDR ����
    int opt = 1;
    setsockopt(_server_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in serverSockAddr;
    memset(&serverSockAddr, 0, sizeof(serverSockAddr));
    serverSockAddr.sin_addr.s_addr = inet_addr(ip);
    serverSockAddr.sin_family = AF_INET;
    serverSockAddr.sin_port = htons(port);

    result = bind(_server_sock, (sockaddr*)&serverSockAddr, sizeof(serverSockAddr));
    if (result == SOCKET_ERROR) {
        printf("Failed to bind server socket: %d\n", WSAGetLastError());
        closesocket(_server_sock);
        WSACleanup();
        throw std::runtime_error("Socket bind failed");
    }

    result = listen(_server_sock, SOMAXCONN);
    if (result == SOCKET_ERROR) {
        printf("Failed to listen on server socket: %d\n", WSAGetLastError());
        closesocket(_server_sock);
        WSACleanup();
        throw std::runtime_error("Socket listen failed");
    }

    _database_thread = std::make_unique<DatabaseThread>(&RecvPakets, &SendPackets);
    if (!_database_thread->ConnectDB())
    {
        printf("Failed to Connect Database Server\n");
        closesocket(_server_sock);
        WSACleanup();
        throw std::runtime_error("Database connection failed");
    }

    // ���� ó�� ������ ����
    _response_handler_thread = std::make_unique<std::thread>(&Server::ResponseHandlerLoop, this);

    
    // Non-blocking ���� ����
    SetNonBlocking(_server_sock);
  
    _is_running.store(true);
    printf("Server initialized successfully on %s:%d\n", ip, port);
}

void Server::Run()
{
    if (!_is_running.load()) {
        printf("Server is not initialized\n");
        return;
    }

    _thread_cleaner = std::make_unique<std::thread>(&Server::Thread_ClearDeadWorkerLoop, this);

    printf("Server is running...\n");

    while (_is_running.load()) {
        sockaddr_in clientSockAddr;
        int clientSockAddrSize = sizeof(clientSockAddr);
        memset(&clientSockAddr, 0, sizeof(clientSockAddr));

        SOCKET clientSock = accept(_server_sock, (sockaddr*)&clientSockAddr, &clientSockAddrSize);

        if (clientSock == INVALID_SOCKET) {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                // Non-blocking ��忡�� ���� ��� ��
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            else if (!_is_running.load()) {
                // ���� ���� ��
                break;
            }
            else {
                printf("Accept error: %d\n", error);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
        }

        // Ŭ���̾�Ʈ ���ϵ� non-blocking���� ����
        SetNonBlocking(clientSock);

        std::lock_guard<std::mutex> lock(_worker_threads_mutex);
        int canUseThreadIdx = FindCanUseWorkerThread();

        if (canUseThreadIdx == -1) {
            // �� WorkerThread ����
            try {
                _worker_threads.push_back(std::make_unique<WorkerThread>(clientSock,&RecvPakets));
                printf("New WorkerThread created. Total threads: %zu\n", _worker_threads.size());
            }
            catch (const std::exception& e) {
                printf("Failed to create WorkerThread: %s\n", e.what());
                closesocket(clientSock);
            }
        }
        else {
            // ���� WorkerThread�� Ŭ���̾�Ʈ �߰�
            _worker_threads[canUseThreadIdx]->AddClient(clientSock);
            printf("Client added to existing thread %d\n", canUseThreadIdx);
        }
    }

    printf("Server main loop finished\n");
}

void Server::Stop()
{
    printf("Stopping server...\n");
    _is_running.store(false);

    // ���� ���� �ݱ� (accept ���ŷ ����)
    if (_server_sock != INVALID_SOCKET) {
        closesocket(_server_sock);
        _server_sock = INVALID_SOCKET;
    }

    // ��� WorkerThread ����
    std::lock_guard<std::mutex> lock(_worker_threads_mutex);
    for (auto& worker : _worker_threads) {
        if (worker) {
            worker->StopThread();
        }
    }
}

void Server::Destroy()
{
    if (instance) {
        instance->Stop();
        std::lock_guard<std::mutex> lock(instance_mutex);
        instance.reset();
    }
}

int Server::FindCanUseWorkerThread()
{
    // _worker_threads_mutex�� �̹� ����ִٰ� ����
    if (_worker_threads.empty()) {
        return -1;
    }

    for (size_t i = 0; i < _worker_threads.size(); ++i) {
        if (_worker_threads[i] &&
            !_worker_threads[i]->IsFullAccess() &&
            _worker_threads[i]->DoThread()) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void Server::ClearDeadThreads()
{
    std::lock_guard<std::mutex> lock(_worker_threads_mutex);

    for (auto iter = _worker_threads.begin(); iter != _worker_threads.end();) {
        if (*iter && !(*iter)->DoThread()) {
            printf("Removing dead WorkerThread. Remaining: %zu\n", _worker_threads.size() - 1);
            iter = _worker_threads.erase(iter);
        }
        else {
            ++iter;
        }
    }
}

void Server::Thread_ClearDeadWorkerLoop()
{
    while (_is_running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        ClearDeadThreads();
    }
    printf("Thread cleaner finished\n");
}

void Server::ResponseHandlerLoop()
{
    std::cout << "[Server] ���� ó�� ������ ����" << std::endl;

    while (_is_running.load()) {
        DBResponse response;
        if (SendPackets.dequeue(response)) {
            ProcessDBResponse(response);
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    std::cout << "[Server] ���� ó�� ������ ����" << std::endl;
}

void Server::ProcessDBResponse(const DBResponse& response)
{
    if (response.response_data.empty()) {
        std::cerr << "[Server] �� ���� ������" << std::endl;
        return;
    }

    // �ش� Ŭ���̾�Ʈ ������ ���� WorkerThread ã��
    WorkerThread* targetWorker = FindWorkerThreadBySocket(response.client_socket);
    if (targetWorker) {
        // WorkerThread�� ���� ���� ��û
        targetWorker->SendToClient(response.client_socket, response.response_data);
    }
    else {
        std::cerr << "[Server] Ŭ���̾�Ʈ ������ ã�� �� ����: " << response.client_socket << std::endl;
    }
}

WorkerThread* Server::FindWorkerThreadBySocket(SOCKET clientSocket)
{
    std::lock_guard<std::mutex> lock(_worker_threads_mutex);

    for (auto& worker : _worker_threads) {
        if (worker && worker->HasClient(clientSocket)) {
            return worker.get();
        }
    }
    return nullptr;
}