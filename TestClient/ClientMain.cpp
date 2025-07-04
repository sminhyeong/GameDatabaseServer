#define NOMINMAX
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "ClientPacketManager.h"
#include "UserEvent_generated.h"

class SimpleTestClient {
private:
    SOCKET _client_socket;
    ClientPacketManager _packet_manager;
    bool _connected;
    std::string _server_ip;
    int _server_port;
    uint32_t _current_user_id;
    std::string _current_username;

    // 연결 설정
    static const int RECV_TIMEOUT_MS = 30000;
    static const int SEND_TIMEOUT_MS = 10000;

public:
    SimpleTestClient(const std::string& ip = "127.0.0.1", int port = 7777)
        : _client_socket(INVALID_SOCKET), _connected(false),
        _server_ip(ip), _server_port(port), _current_user_id(0) {
    }

    ~SimpleTestClient() {
        Disconnect();
        WSACleanup();
    }

    bool Initialize() {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cout << "[ERROR] WSAStartup failed: " << result << std::endl;
            return false;
        }
        return true;
    }

    bool Connect() {
        if (_connected) {
            std::cout << "[INFO] Already connected" << std::endl;
            return true;
        }

        _client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (_client_socket == INVALID_SOCKET) {
            std::cout << "[ERROR] Failed to create socket: " << WSAGetLastError() << std::endl;
            return false;
        }

        // 타임아웃 설정
        int recv_timeout = RECV_TIMEOUT_MS;
        setsockopt(_client_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&recv_timeout, sizeof(recv_timeout));
        int send_timeout = SEND_TIMEOUT_MS;
        setsockopt(_client_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&send_timeout, sizeof(send_timeout));

        sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(_server_port);
        inet_pton(AF_INET, _server_ip.c_str(), &serverAddr.sin_addr);

        if (connect(_client_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cout << "[ERROR] Failed to connect: " << WSAGetLastError() << std::endl;
            closesocket(_client_socket);
            _client_socket = INVALID_SOCKET;
            return false;
        }

        _connected = true;
        std::cout << "[SUCCESS] Connected to server " << _server_ip << ":" << _server_port << std::endl;
        return true;
    }

    void Disconnect() {
        if (_connected && _client_socket != INVALID_SOCKET) {
            closesocket(_client_socket);
            _client_socket = INVALID_SOCKET;
            _connected = false;
            std::cout << "[INFO] Disconnected from server" << std::endl;
        }
    }

    bool TryReconnect() {
        std::cout << "[INFO] Attempting to reconnect..." << std::endl;
        Disconnect();
        std::this_thread::sleep_for(std::chrono::seconds(1));

        for (int attempt = 1; attempt <= 3; attempt++) {
            std::cout << "[INFO] Reconnection attempt " << attempt << "/3" << std::endl;
            if (Connect()) {
                std::cout << "[SUCCESS] Reconnected successfully!" << std::endl;
                return true;
            }
            if (attempt < 3) {
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }

        std::cout << "[ERROR] Failed to reconnect after 3 attempts" << std::endl;
        return false;
    }

    bool IsConnected() {
        return _connected && _client_socket != INVALID_SOCKET;
    }

    bool SendPacket(const std::vector<uint8_t>& packet) {
        if (!IsConnected() || packet.empty()) {
            std::cout << "[ERROR] Not connected or empty packet" << std::endl;
            return false;
        }

        // 패킷 크기 전송 (4바이트 헤더)
        uint32_t packetSize = static_cast<uint32_t>(packet.size());
        uint32_t networkSize = htonl(packetSize);

        if (send(_client_socket, reinterpret_cast<const char*>(&networkSize), sizeof(networkSize), 0) <= 0) {
            std::cout << "[ERROR] Failed to send header - Error: " << WSAGetLastError() << std::endl;
            _connected = false;
            return false;
        }

        // 패킷 데이터 전송
        size_t totalSent = 0;
        while (totalSent < packet.size()) {
            int sent = send(_client_socket, reinterpret_cast<const char*>(packet.data()) + totalSent,
                static_cast<int>(packet.size() - totalSent), 0);
            if (sent <= 0) {
                std::cout << "[ERROR] Failed to send packet data - Error: " << WSAGetLastError() << std::endl;
                _connected = false;
                return false;
            }
            totalSent += sent;
        }

        std::cout << "[DEBUG] Sent packet: " << packet.size() << " bytes" << std::endl;
        return true;
    }

    std::vector<uint8_t> ReceivePacket() {
        if (!IsConnected()) {
            std::cout << "[ERROR] Not connected" << std::endl;
            return std::vector<uint8_t>();
        }

        // 패킷 크기 수신
        uint32_t packetSize = 0;
        int headerReceived = recv(_client_socket, reinterpret_cast<char*>(&packetSize), sizeof(packetSize), 0);
        if (headerReceived <= 0) {
            std::cout << "[ERROR] Failed to receive header - Error: " << WSAGetLastError() << std::endl;
            _connected = false;
            return std::vector<uint8_t>();
        }

        packetSize = ntohl(packetSize);
        if (packetSize == 0 || packetSize > 65536) {
            std::cout << "[ERROR] Invalid packet size: " << packetSize << std::endl;
            _connected = false;
            return std::vector<uint8_t>();
        }

        // 패킷 데이터 수신
        std::vector<uint8_t> packetData(packetSize);
        size_t totalReceived = 0;
        while (totalReceived < packetSize) {
            int received = recv(_client_socket, reinterpret_cast<char*>(packetData.data()) + totalReceived,
                static_cast<int>(packetSize - totalReceived), 0);
            if (received <= 0) {
                std::cout << "[ERROR] Failed to receive packet data - Error: " << WSAGetLastError() << std::endl;
                _connected = false;
                return std::vector<uint8_t>();
            }
            totalReceived += received;
        }

        std::cout << "[DEBUG] Received packet: " << packetSize << " bytes" << std::endl;
        return packetData;
    }

    // =========================
    // 핵심 테스트 기능들
    // =========================

    bool TestLogin() {
        std::cout << "\n=== LOGIN TEST ===" << std::endl;

        std::string username, password;
        std::cout << "Enter username: ";
        std::getline(std::cin, username);
        std::cout << "Enter password: ";
        std::getline(std::cin, password);

        if (username.empty() || password.empty()) {
            std::cout << "[ERROR] Username and password cannot be empty" << std::endl;
            return false;
        }

        auto loginPacket = _packet_manager.CreateLoginRequest(username, password);
        if (loginPacket.empty()) {
            std::cout << "[ERROR] Failed to create login packet" << std::endl;
            return false;
        }

        if (!SendPacket(loginPacket)) {
            std::cout << "[WARNING] Failed to send packet, trying to reconnect..." << std::endl;
            if (TryReconnect()) {
                if (!SendPacket(loginPacket)) {
                    std::cout << "[ERROR] Failed to send packet even after reconnection" << std::endl;
                    return false;
                }
            }
            else {
                return false;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 조금 더 길게 대기

        auto response = ReceivePacket();
        if (response.empty()) {
            std::cout << "[WARNING] Failed to receive response, trying to reconnect..." << std::endl;
            if (TryReconnect()) {
                std::cout << "[INFO] Reconnected successfully. Please try login again." << std::endl;
            }
            return false;
        }

        const S2C_Login* loginResponse = _packet_manager.ParseLoginResponse(response.data(), response.size());
        if (!loginResponse) {
            std::cout << "[ERROR] Failed to parse login response" << std::endl;
            return false;
        }

        if (_packet_manager.IsLoginSuccess(loginResponse)) {
            _current_user_id = loginResponse->user_id();
            _current_username = loginResponse->username()->str();
            std::cout << "[SUCCESS] Login successful!" << std::endl;
            std::cout << "  User ID: " << _current_user_id << std::endl;
            std::cout << "  Username: " << _current_username << std::endl;
            std::cout << "  Nickname: " << loginResponse->nickname()->str() << std::endl;
            std::cout << "  Level: " << loginResponse->level() << std::endl;
            return true;
        }
        else {
            std::cout << "[FAILED] Login failed - Result code: " << static_cast<int>(loginResponse->result()) << std::endl;
            std::cout << "[INFO] Connection is still alive, you can try again." << std::endl;
            return false;
        }
    }

    bool TestLogout() {
        std::cout << "\n=== LOGOUT TEST ===" << std::endl;

        if (_current_user_id == 0) {
            std::cout << "[ERROR] No user logged in" << std::endl;
            return false;
        }

        auto logoutPacket = _packet_manager.CreateLogoutRequest(_current_user_id);
        if (logoutPacket.empty()) {
            std::cout << "[ERROR] Failed to create logout packet" << std::endl;
            return false;
        }

        if (!SendPacket(logoutPacket)) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        auto response = ReceivePacket();
        if (response.empty()) {
            return false;
        }

        const S2C_Logout* logoutResponse = _packet_manager.ParseLogoutResponse(response.data(), response.size());
        if (!logoutResponse) {
            std::cout << "[ERROR] Failed to parse logout response" << std::endl;
            return false;
        }

        if (logoutResponse->result() == ResultCode_SUCCESS) {
            std::cout << "[SUCCESS] Logout successful!" << std::endl;
            std::cout << "  Message: " << logoutResponse->message()->str() << std::endl;
            _current_user_id = 0;
            _current_username.clear();
            return true;
        }
        else {
            std::cout << "[FAILED] Logout failed - Result code: " << static_cast<int>(logoutResponse->result()) << std::endl;
            return false;
        }
    }

    bool TestCreateAccount() {
        std::cout << "\n=== CREATE ACCOUNT TEST ===" << std::endl;

        std::string username, password, nickname;
        std::cout << "Enter new username: ";
        std::getline(std::cin, username);
        std::cout << "Enter new password: ";
        std::getline(std::cin, password);
        std::cout << "Enter nickname: ";
        std::getline(std::cin, nickname);

        if (username.empty() || password.empty() || nickname.empty()) {
            std::cout << "[ERROR] All fields must be filled" << std::endl;
            return false;
        }

        auto accountPacket = _packet_manager.CreateAccountRequest(username, password, nickname);
        if (accountPacket.empty()) {
            std::cout << "[ERROR] Failed to create account packet" << std::endl;
            return false;
        }

        if (!SendPacket(accountPacket)) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        auto response = ReceivePacket();
        if (response.empty()) {
            return false;
        }

        const S2C_CreateAccount* accountResponse = _packet_manager.ParseCreateAccountResponse(response.data(), response.size());
        if (!accountResponse) {
            std::cout << "[ERROR] Failed to parse create account response" << std::endl;
            return false;
        }

        if (_packet_manager.IsCreateAccountSuccess(accountResponse)) {
            std::cout << "[SUCCESS] Account created successfully!" << std::endl;
            std::cout << "  New User ID: " << accountResponse->user_id() << std::endl;
            std::cout << "  Message: " << accountResponse->message()->str() << std::endl;
            return true;
        }
        else {
            std::cout << "[FAILED] Account creation failed - Result code: " << static_cast<int>(accountResponse->result()) << std::endl;
            std::cout << "  Message: " << accountResponse->message()->str() << std::endl;
            return false;
        }
    }

    bool TestGetStoreItems() {
        std::cout << "\n=== GET STORE ITEMS TEST ===" << std::endl;

        std::string storeIdStr;
        std::cout << "Enter Store ID: ";
        std::getline(std::cin, storeIdStr);

        if (storeIdStr.empty()) {
            std::cout << "[ERROR] Store ID cannot be empty" << std::endl;
            return false;
        }

        uint32_t storeId = 0;
        try {
            storeId = std::stoul(storeIdStr);
        }
        catch (...) {
            std::cout << "[ERROR] Invalid Store ID" << std::endl;
            return false;
        }

        auto shopItemsPacket = _packet_manager.CreateShopItemsRequest(storeId);
        if (shopItemsPacket.empty()) {
            std::cout << "[ERROR] Failed to create shop items packet" << std::endl;
            return false;
        }

        if (!SendPacket(shopItemsPacket)) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        auto response = ReceivePacket();
        if (response.empty()) {
            return false;
        }

        const S2C_ShopItems* shopItemsResponse = _packet_manager.ParseShopItemsResponse(response.data(), response.size());
        if (!shopItemsResponse) {
            std::cout << "[ERROR] Failed to parse shop items response" << std::endl;
            return false;
        }

        if (_packet_manager.IsShopItemsValid(shopItemsResponse)) {
            std::cout << "[SUCCESS] Store items retrieved successfully!" << std::endl;
            std::cout << "  Store ID: " << shopItemsResponse->shop_id() << std::endl;

            if (shopItemsResponse->items() && shopItemsResponse->items()->size() > 0) {
                std::cout << "  Items (" << shopItemsResponse->items()->size() << " found):" << std::endl;
                for (size_t i = 0; i < shopItemsResponse->items()->size(); ++i) {
                    const auto* item = shopItemsResponse->items()->Get(i);
                    std::cout << "    [" << (i + 1) << "] ID: " << item->item_id()
                        << ", Name: " << item->item_name()->str()
                        << ", Price: " << item->base_price()
                        << ", Type: " << item->item_type() << std::endl;
                }
            }
            else {
                std::cout << "  No items found in this store." << std::endl;
            }
            return true;
        }
        else {
            std::cout << "[FAILED] Failed to get store items - Result code: " << static_cast<int>(shopItemsResponse->result()) << std::endl;
            return false;
        }
    }

    void ShowStatus() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "           CURRENT STATUS" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Server: " << _server_ip << ":" << _server_port << std::endl;
        std::cout << "Connection: " << (_connected ? "CONNECTED" : "DISCONNECTED") << std::endl;

        if (_current_user_id > 0) {
            std::cout << "Logged in as: " << _current_username << " (ID: " << _current_user_id << ")" << std::endl;
        }
        else {
            std::cout << "Status: Not logged in" << std::endl;
        }
        std::cout << "========================================" << std::endl;
    }

    void ShowMenu() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "         SIMPLE TEST CLIENT MENU" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "1. Login" << std::endl;
        std::cout << "2. Logout" << std::endl;
        std::cout << "3. Create Account" << std::endl;
        std::cout << "4. Get Store Items" << std::endl;
        std::cout << "5. Show Status" << std::endl;
        std::cout << "6. Reconnect" << std::endl;
        std::cout << "0. Exit" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Enter your choice: ";
    }

    void RunTestLoop() {
        std::cout << "Simple Test Client Started!" << std::endl;
        std::cout << "Connected to: " << _server_ip << ":" << _server_port << std::endl;

        while (true) {
            ShowMenu();

            std::string input;
            std::getline(std::cin, input);

            if (input.empty()) {
                std::cout << "[ERROR] Please enter a valid choice." << std::endl;
                continue;
            }

            int choice = 0;
            try {
                choice = std::stoi(input);
            }
            catch (...) {
                std::cout << "[ERROR] Invalid input. Please enter a number." << std::endl;
                continue;
            }

            switch (choice) {
            case 1:
                TestLogin();
                break;
            case 2:
                TestLogout();
                break;
            case 3:
                TestCreateAccount();
                break;
            case 4:
                TestGetStoreItems();
                break;
            case 5:
                ShowStatus();
                break;
            case 6:
                std::cout << "[INFO] Reconnecting..." << std::endl;
                if (TryReconnect()) {
                    std::cout << "[SUCCESS] Reconnected!" << std::endl;
                }
                else {
                    std::cout << "[ERROR] Reconnection failed!" << std::endl;
                }
                break;
            case 0:
                std::cout << "[INFO] Exiting test client. Goodbye!" << std::endl;
                return;
            default:
                std::cout << "[ERROR] Invalid choice. Please try again." << std::endl;
                break;
            }

            std::cout << "\nPress Enter to continue...";
            std::cin.get();
        }
    }
};

int main() {
    SimpleTestClient client;

    std::cout << "========================================" << std::endl;
    std::cout << "    Simple DB Server Test Client" << std::endl;
    std::cout << "========================================" << std::endl;

    if (!client.Initialize()) {
        std::cerr << "[ERROR] Failed to initialize client" << std::endl;
        return -1;
    }

    if (!client.Connect()) {
        std::cerr << "[ERROR] Failed to connect to server" << std::endl;
        std::cout << "Please make sure the server is running and try again." << std::endl;
        return -1;
    }

    client.RunTestLoop();

    return 0;
}