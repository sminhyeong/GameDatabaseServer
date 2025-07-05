#pragma once
#include <vector>
#include <string>
#include <winSock2.h>
#pragma comment(lib, "ws2_32.lib")

// Task Ÿ�� ������ (CLIENT_DISCONNECTED �߰�)
enum class TaskType {
    QUERY,      // SELECT ó��
    UPDATE,     // UPDATE, INSERT, DELETE ó��
    INSERT,
    ITEM_DELETE,
    CLIENT_DISCONNECTED  // Ŭ���̾�Ʈ ���� ���� �˸� �߰�
};

// DB ���� ����ü (������ ����)
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

// DB ��û ����ü (������ ����)
struct Task {
    int id;
    SOCKET client_socket;
    int worker_thread_id;
    TaskType type;
    std::string query;  // ���� ó�� ���ڿ� (�׽�Ʈ��)
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

    // ������ ó�� ������ ������
    Task(SOCKET sock, int thread_id, TaskType t, const std::string& q)
        : id(0), client_socket(sock), worker_thread_id(thread_id),
        type(t), query(q) {
    }

    // Ŭ���̾�Ʈ ���� ������ ������ �߰�
    Task(SOCKET sock, TaskType disconnect_type)
        : id(0), client_socket(sock), worker_thread_id(0),
        type(disconnect_type) {
    }
};