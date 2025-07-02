#pragma once
#include <vector>
#include <winSock2.h>
#pragma comment(lib, "ws2_32.lib")


// DB 응답 구조체
struct DBResponse {
    SOCKET client_socket;
    int worker_thread_id;
    std::vector<uint8_t> response_data;
       
    DBResponse(SOCKET sock, int thread_id, const uint8_t* data, size_t size)
        : client_socket(sock), worker_thread_id(thread_id) {
        response_data.assign(data, data + size);
    }
};

// DB 요청 구조체 (기존 Task 구조체 활용)
struct Task {
    SOCKET client_socket;
    int worker_thread_id;
    std::vector<uint8_t> flatbuffer_data;

    Task(SOCKET sock, int thread_id, const uint8_t* data, size_t size)
        : client_socket(sock), worker_thread_id(thread_id) {
        flatbuffer_data.assign(data, data + size);
    }
};
