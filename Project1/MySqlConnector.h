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
	
	// ���� ����
	void disconnect();

	// ���� ���� Ȯ��
	bool isConnected();

	// �翬��
	void reconnect(const std::string& host, const std::string& user,
		const std::string& password, const std::string& database);
};