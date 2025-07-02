#pragma once
#include "mysql/jdbc.h"

class MySQLConnector {
private:
	sql::mysql::MySQL_Driver* driver;
	std::unique_ptr<sql::Connection> connection;

public:
	MySQLConnector();
	~MySQLConnector();
	
	bool connect(const std::string& host, const std::string& user, const std::string& password, const std::string& database);
	std::unique_ptr<sql::ResultSet> executeQuery(const std::string& query);
	
	// 연결 해제
	void disconnect();

	// 연결 상태 확인
	bool isConnected();

	// 재연결
	void reconnect(const std::string& host, const std::string& user,
		const std::string& password, const std::string& database);
};