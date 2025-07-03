#pragma once

// �⺻ C++ ����鸸
#include <iostream>
#pragma once

// �⺻ C++ �����
#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <chrono>

// ���� �������� ��� �浹 ����
template<typename T>
class LockFreeQueue;

struct Task;
struct DBResponse;
class MySqlConnector;
class ServerPacketManager;

// �ʿ��� enum�鸸 ���� ����
enum EventType : uint8_t;
enum ResultCode : int8_t;

class DatabaseThread
{
private:
    std::atomic<bool> _is_running;
    std::thread _db_thread;

    // ť �����͵�
    LockFreeQueue<Task>* RecvQueue;
    LockFreeQueue<DBResponse>* SendQueue;

    // �ֿ� ������Ʈ��
    std::unique_ptr<MySqlConnector> _sql_connector;
    std::unique_ptr<ServerPacketManager> _packet_manager;

    // ���� ���� ����
    int _port;
    std::string _host;
    std::string _user;
    std::string _password;
    std::string _database;

    // ������ ���� �Լ�
    void Run();

    // �½�ũ ó�� �Լ���
    void ProcessTask(const Task& task);
    void HandleLoginRequest(const Task& task);
    void HandleLogoutRequest(const Task& task);
    void HandleCreateAccountRequest(const Task& task);
    void HandlePlayerDataRequest(const Task& task);
    void HandleItemDataRequest(const Task& task);
    void HandleMonsterDataRequest(const Task& task);
    void HandlePlayerChatRequest(const Task& task);

    // ���� ���� ���� �Լ���
    void SendResponse(const Task& task, const std::vector<uint8_t>& responsePacket);
    void SendErrorResponse(const Task& task, EventType responseType, ResultCode errorCode);

    // DB ���� ���� üũ
    bool CheckDBConnection();
    bool ReconnectIfNeeded();

public:
    DatabaseThread(LockFreeQueue<Task>* RecvQueue, LockFreeQueue<DBResponse>* SendQueue);
    ~DatabaseThread();

    // DB ���� �Լ���
    void SetConnectionInfo(const std::string& host, const std::string& user,
        const std::string& password, const std::string& database, int port = 3306);

    bool ConnectDB();
    void Stop();

    // ���� Ȯ�� �Լ���
    bool IsRunning() const { return _is_running.load(); }
    bool IsDBConnected() const;

    // ��� ����
    std::string GetStatus() const;
};