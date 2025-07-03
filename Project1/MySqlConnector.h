#pragma once
#include <string>
#include <memory>
#include "Packet.h"
#include <mysql.h>

class MySqlConnector
{
private:
    bool _is_init;
    MYSQL* conn;
    MYSQL* conn_result;
    unsigned int timeout_sec;

public:
    MySqlConnector();
    ~MySqlConnector();

    bool Init();
    bool Connect(std::string host, std::string userName, std::string pass, int port, std::string dbName);

    // ���� ���� �Լ��� �߰�
    bool ExecuteQuery(const std::string& query);
    MYSQL_RES* GetResult();
    void FreeResult(MYSQL_RES* result);
    int GetAffectedRows();

    bool IsConnected() const { return conn && _is_init; }
};