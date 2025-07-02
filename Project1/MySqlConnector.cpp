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
        // 데이터베이스 연결
        connection.reset(driver->connect("tcp://" + host + ":3306", user, password));
        connection->setSchema(database);

        std::cout << "MySQL 연결 성공!" << std::endl;
        return true;

    }
    catch (sql::SQLException& e) {
        std::cerr << "연결 실패: " << e.what() << std::endl;
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
        std::cerr << "쿼리 실행 실패: " << e.what() << std::endl;
        return nullptr;
    }
}

// 연결 해제
void MySQLConnector::disconnect() {
    try {
        if (connection && !connection->isClosed()) {
            connection->close();
            std::cout << "MySQL 연결이 성공적으로 해제되었습니다." << std::endl;
        }
        else {
            std::cout << "연결이 이미 해제되어 있거나 존재하지 않습니다." << std::endl;
        }
    }
    catch (sql::SQLException& e) {
        std::cerr << "연결 해제 중 오류 발생: " << e.what() << std::endl;
    }
}

// 연결 상태 확인
bool MySQLConnector::isConnected() {
    try {
        return connection && !connection->isClosed();
    }
    catch (sql::SQLException& e) {
        std::cerr << "연결 상태 확인 중 오류: " << e.what() << std::endl;
        return false;
    }
}

// 재연결
void MySQLConnector::reconnect(const std::string& host, const std::string& user,
    const std::string& password, const std::string& database) {
    std::cout << "재연결을 시도합니다..." << std::endl;
    disconnect();
    connect(host, user, password, database);
}