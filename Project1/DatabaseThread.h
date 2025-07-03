#pragma once

// 기본 C++ 헤더들만
#include <iostream>
#pragma once

// 기본 C++ 헤더들
#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <chrono>

// 전방 선언으로 헤더 충돌 방지
template<typename T>
class LockFreeQueue;

struct Task;
struct DBResponse;
class MySqlConnector;
class ServerPacketManager;

// 필요한 enum들만 전방 선언
enum EventType : uint8_t;
enum ResultCode : int8_t;

class DatabaseThread
{
private:
    std::atomic<bool> _is_running;
    std::thread _db_thread;

    // 큐 포인터들
    LockFreeQueue<Task>* RecvQueue;
    LockFreeQueue<DBResponse>* SendQueue;

    // 주요 컴포넌트들
    std::unique_ptr<MySqlConnector> _sql_connector;
    std::unique_ptr<ServerPacketManager> _packet_manager;

    // 연결 정보 저장
    int _port;
    std::string _host;
    std::string _user;
    std::string _password;
    std::string _database;

    // 스레드 실행 함수
    void Run();

    // 태스크 처리 함수들
    void ProcessTask(const Task& task);
    void HandleLoginRequest(const Task& task);
    void HandleLogoutRequest(const Task& task);
    void HandleCreateAccountRequest(const Task& task);
    void HandlePlayerDataRequest(const Task& task);
    void HandleItemDataRequest(const Task& task);
    void HandleMonsterDataRequest(const Task& task);
    void HandlePlayerChatRequest(const Task& task);

    // 응답 전송 헬퍼 함수들
    void SendResponse(const Task& task, const std::vector<uint8_t>& responsePacket);
    void SendErrorResponse(const Task& task, EventType responseType, ResultCode errorCode);

    // DB 연결 상태 체크
    bool CheckDBConnection();
    bool ReconnectIfNeeded();

public:
    DatabaseThread(LockFreeQueue<Task>* RecvQueue, LockFreeQueue<DBResponse>* SendQueue);
    ~DatabaseThread();

    // DB 설정 함수들
    void SetConnectionInfo(const std::string& host, const std::string& user,
        const std::string& password, const std::string& database, int port = 3306);

    bool ConnectDB();
    void Stop();

    // 상태 확인 함수들
    bool IsRunning() const { return _is_running.load(); }
    bool IsDBConnected() const;

    // 통계 정보
    std::string GetStatus() const;
};