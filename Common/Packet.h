#pragma once
#include <vector>
#include <string>
#include <winSock2.h>
#pragma comment(lib, "ws2_32.lib")

// Task 타입 열거형
enum class TaskType {
    QUERY,      // SELECT 쿼리
    UPDATE,     // UPDATE, INSERT, DELETE 쿼리
    INSERT,
    ITEM_DELETE
};

// DB 응답 구조체
struct DBResponse {
    int task_id;
    SOCKET client_socket;
    int worker_thread_id;
    bool success;
    std::string error_message;
    int affected_rows;
    std::vector<uint8_t> response_data;

    DBResponse()
        : task_id(0), client_socket(INVALID_SOCKET),
        worker_thread_id(0), success(false), affected_rows(0) {
    }

    DBResponse(SOCKET sock, int thread_id, const uint8_t* data, size_t size)
        : task_id(0), client_socket(sock), worker_thread_id(thread_id),
        success(false), affected_rows(0) {
        response_data.assign(data, data + size);
    }
};

// DB 요청 구조체
struct Task {
    int id;
    SOCKET client_socket;
    int worker_thread_id;
    TaskType type;
    std::string query;  // 직접 쿼리 문자열 (테스트용)
    std::vector<uint8_t> flatbuffer_data;

    Task()
        : id(0), client_socket(INVALID_SOCKET),
        worker_thread_id(0), type(TaskType::QUERY) {
    }

    Task(SOCKET sock, int thread_id, const uint8_t* data, size_t size)
        : id(0), client_socket(sock), worker_thread_id(thread_id),
        type(TaskType::QUERY) {
        flatbuffer_data.assign(data, data + size);
    }

    // 간단한 쿼리 생성용 생성자
    Task(SOCKET sock, int thread_id, TaskType t, const std::string& q)
        : id(0), client_socket(sock), worker_thread_id(thread_id),
        type(t), query(q) {
    }
};