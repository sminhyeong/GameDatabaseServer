#include "MySqlConnector.h"

MySQLConnector::MySQLConnector() : driver(nullptr) {
    driver = sql::mysql::get_mysql_driver_instance();
}

MySQLConnector::~MySQLConnector() {
    if (connection) {
        connection->close();
    }
}


bool MySQLConnector::connect(const std::string& host, const std::string& user,
    const std::string& password, const std::string& database) {
    try {
        // �����ͺ��̽� ����
        connection.reset(driver->connect("tcp://" + host + ":3306", user, password));
        connection->setSchema(database);

        std::cout << "MySQL ���� ����!" << std::endl;
        return true;

    }
    catch (sql::SQLException& e) {
        std::cerr << "���� ����: " << e.what() << std::endl;
        std::cerr << "Error Code: " << e.getErrorCode() << std::endl;
        return false;
    }
}

std::unique_ptr<sql::ResultSet> MySQLConnector::executeQuery(const std::string& query) {
    try {
        std::unique_ptr<sql::Statement> stmt(connection->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(query));
        return res;
    }
    catch (sql::SQLException& e) {
        std::cerr << "���� ���� ����: " << e.what() << std::endl;
        return nullptr;
    }
}

// ���� ����
void MySQLConnector::disconnect() {
    try {
        if (connection && !connection->isClosed()) {
            connection->close();
            std::cout << "MySQL ������ ���������� �����Ǿ����ϴ�." << std::endl;
        }
        else {
            std::cout << "������ �̹� �����Ǿ� �ְų� �������� �ʽ��ϴ�." << std::endl;
        }
    }
    catch (sql::SQLException& e) {
        std::cerr << "���� ���� �� ���� �߻�: " << e.what() << std::endl;
    }
}

// ���� ���� Ȯ��
bool MySQLConnector::isConnected() {
    try {
        return connection && !connection->isClosed();
    }
    catch (sql::SQLException& e) {
        std::cerr << "���� ���� Ȯ�� �� ����: " << e.what() << std::endl;
        return false;
    }
}

// �翬��
void MySQLConnector::reconnect(const std::string& host, const std::string& user,
    const std::string& password, const std::string& database) {
    std::cout << "�翬���� �õ��մϴ�..." << std::endl;
    disconnect();
    connect(host, user, password, database);
}