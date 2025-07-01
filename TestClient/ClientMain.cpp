#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

int main()
{
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    sockaddr_in serverAddr;

    // 1. WSA �ʱ�ȭ
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "WSAStartup failed\n";
        return -1;
    }

    // 2. ���� ����
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cout << "Socket creation failed\n";
        WSACleanup();
        return -1;
    }

    // 3. ���� �ּ� ���� (localhost:7777 ����)
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(7777);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 4. ���� ���� �õ�
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cout << "Connect failed\n";
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    std::cout << "Connected to server\n";

    // 5. �޽��� �ۼ��� ����
    while (true) {
        std::string input;
        std::cout << "Send message (q to quit): ";
        std::getline(std::cin, input);

        if (input == "q") break;

        // �޽��� ������
        int sendResult = send(sock, input.c_str(), (int)input.size(), 0);
        if (sendResult == SOCKET_ERROR) {
            std::cout << "Send failed\n";
            break;
        }

        // ���� ���� �ޱ� (����)
        char buffer[512];
        int recvLen = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (recvLen > 0) {
            buffer[recvLen] = '\0';
            std::cout << "Received from server: " << buffer << "\n";
        }
        else {
            std::cout << "Server closed connection or error\n";
            break;
        }
    }

    // 6. ���� ó��
    closesocket(sock);
    WSACleanup();

    return 0;
}