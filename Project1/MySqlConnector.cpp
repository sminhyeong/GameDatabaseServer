#include "MySqlConnector.h"
#include <iostream>

MySqlConnector::MySqlConnector() : _is_init(false), timeout_sec(10)
{
    conn = nullptr;
    conn_result = nullptr;
}

MySqlConnector::~MySqlConnector()
{
    if (conn) {
        mysql_close(conn);
        conn = nullptr;
    }
}

bool MySqlConnector::Init()
{
    try
    {
        conn = mysql_init(NULL);
        if (!conn) {
            std::cerr << "mysql_init() failed" << std::endl;
            return false;
        }

        mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout_sec);
        mysql_options(conn, MYSQL_OPT_RECONNECT, &timeout_sec); // 자동 재연결
        _is_init = true;
        return true;
    }
    catch (std::exception& e)
    {
        std::cerr << "MySQL Init Exception: " << e.what() << std::endl;
        return false;
    }
}

bool MySqlConnector::Connect(std::string host, std::string userName, std::string pass, int port, std::string dbName)
{
    try
    {
        conn_result = mysql_real_connect(conn, host.c_str(), userName.c_str(), pass.c_str(), dbName.c_str(), port, NULL, CLIENT_MULTI_STATEMENTS);
        
        if (conn_result) {
            std::cout << "Success To Connect MySQL Database!" << std::endl;
            return true;
        }
        else {
            std::cerr << "MySQL Connection failed: " << mysql_error(conn) << std::endl;
            return false;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "MySQL Connect Exception: " << e.what() << std::endl;
        return false;
    }
}

bool MySqlConnector::ExecuteQuery(const std::string& query)
{
    mysql_set_character_set(conn, "euckr");

    if (!conn || !_is_init) {
        std::cerr << "MySQL not initialized or connected" << std::endl;
        return false;
    }

    int result = mysql_query(conn, query.c_str());
    if (result != 0) {
        std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
        return false;
    }
    return true;
}

MYSQL_RES* MySqlConnector::GetResult()
{
    if (!conn) return nullptr;
    return mysql_store_result(conn);
}

void MySqlConnector::FreeResult(MYSQL_RES* result)
{
    if (result) {
        mysql_free_result(result);
    }
}

int MySqlConnector::GetAffectedRows()
{
    if (!conn) return -1;
    return static_cast<int>(mysql_affected_rows(conn));
}