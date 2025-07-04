#define NOMINMAX
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <map>
#include <functional>
#include <iomanip>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "ClientPacketManager.h"
#include "UserEvent_generated.h"

class TestClient {
private:
    SOCKET _client_socket;
    ClientPacketManager _packet_manager;
    bool _connected;
    std::string _server_ip;
    int _server_port;
    uint32_t _current_user_id;
    std::string _current_username;

    // 연결 안정성을 위한 설정
    static const int RECV_TIMEOUT_MS = 10000;
    static const int SEND_TIMEOUT_MS = 5000;

    // 테스트 메뉴
    std::map<int, std::pair<std::string, std::function<bool()>>> _test_functions;

public:
    TestClient(const std::string& ip = "127.0.0.1", int port = 7777)
        : _client_socket(INVALID_SOCKET), _connected(false),
        _server_ip(ip), _server_port(port), _current_user_id(0) {
        InitializeTestFunctions();
    }

    ~TestClient() {
        Disconnect();
        WSACleanup();
    }

    void InitializeTestFunctions() {
        _test_functions[1] = { "Login Test", [this]() { return TestLoginInteractive(); } };
        _test_functions[2] = { "Create Account Test", [this]() { return TestCreateAccountInteractive(); } };
        _test_functions[3] = { "Player Data Query Test", [this]() { return TestPlayerDataQuery(); } };
        _test_functions[4] = { "Player Data Update Test", [this]() { return TestPlayerDataUpdate(); } };
        _test_functions[5] = { "Item Data Query Test", [this]() { return TestItemDataQuery(); } };
        _test_functions[6] = { "Monster Data Query Test", [this]() { return TestMonsterDataQuery(); } };
        _test_functions[7] = { "Chat Send Test", [this]() { return TestChatSendInteractive(); } };
        _test_functions[8] = { "Shop List Test", [this]() { return TestShopList(); } };
        _test_functions[9] = { "Shop Items Test", [this]() { return TestShopItems(); } };
        _test_functions[10] = { "Shop Transaction Test", [this]() { return TestShopTransaction(); } };
        _test_functions[11] = { "Logout Test", [this]() { return TestLogout(); } };
    }

    bool Initialize() {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "[TestClient] WSAStartup failed: " << result << std::endl;
            return false;
        }
        return true;
    }

    bool Connect() {
        if (_connected) {
            std::cout << "[TestClient] Already connected" << std::endl;
            return true;
        }

        _client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (_client_socket == INVALID_SOCKET) {
            std::cerr << "[TestClient] Failed to create socket: " << WSAGetLastError() << std::endl;
            return false;
        }

        // 소켓 타임아웃 설정
        SetSocketTimeouts();

        // TCP_NODELAY 설정
        int flag = 1;
        setsockopt(_client_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));

        sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(_server_port);

        int inetResult = inet_pton(AF_INET, _server_ip.c_str(), &serverAddr.sin_addr);
        if (inetResult <= 0) {
            std::cerr << "[TestClient] Invalid IP address: " << _server_ip << std::endl;
            closesocket(_client_socket);
            _client_socket = INVALID_SOCKET;
            return false;
        }

        int result = connect(_client_socket, (sockaddr*)&serverAddr, sizeof(serverAddr));
        if (result == SOCKET_ERROR) {
            int error = WSAGetLastError();
            std::cerr << "[TestClient] Failed to connect: " << error << std::endl;
            closesocket(_client_socket);
            _client_socket = INVALID_SOCKET;
            return false;
        }

        _connected = true;
        std::cout << "[TestClient] Successfully connected to server " << _server_ip << ":" << _server_port << std::endl;
        return true;
    }

    void SetSocketTimeouts() {
        if (_client_socket == INVALID_SOCKET) return;

        int recv_timeout = RECV_TIMEOUT_MS;
        setsockopt(_client_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&recv_timeout, sizeof(recv_timeout));

        int send_timeout = SEND_TIMEOUT_MS;
        setsockopt(_client_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&send_timeout, sizeof(send_timeout));
    }

    void Disconnect() {
        if (_connected && _client_socket != INVALID_SOCKET) {
            shutdown(_client_socket, SD_BOTH);
            closesocket(_client_socket);
            _client_socket = INVALID_SOCKET;
            _connected = false;
            std::cout << "[TestClient] Disconnected from server" << std::endl;
        }
    }

    void HandleConnectionLoss() {
        std::cout << "\n[ERROR] Connection lost! Exiting client..." << std::endl;
        _connected = false;
        exit(1);
    }

    bool SendPacket(const std::vector<uint8_t>& packet) {
        if (!_connected || packet.empty()) {
            std::cerr << "[TestClient] Not connected or empty packet" << std::endl;
            HandleConnectionLoss();
            return false;
        }

        try {
            // 패킷 크기 전송 (4바이트 헤더)
            uint32_t packetSize = static_cast<uint32_t>(packet.size());
            uint32_t networkSize = htonl(packetSize);

            // 헤더 전송
            if (!SendData(reinterpret_cast<const char*>(&networkSize), sizeof(networkSize))) {
                std::cerr << "[TestClient] Failed to send header" << std::endl;
                HandleConnectionLoss();
                return false;
            }

            // 데이터 전송
            if (!SendData(reinterpret_cast<const char*>(packet.data()), packet.size())) {
                std::cerr << "[TestClient] Failed to send packet data" << std::endl;
                HandleConnectionLoss();
                return false;
            }

            std::cout << "[TestClient] Sent packet: " << packet.size() << " bytes" << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[TestClient] Exception in SendPacket: " << e.what() << std::endl;
            HandleConnectionLoss();
            return false;
        }
    }

    bool SendData(const char* data, size_t size) {
        size_t totalSent = 0;
        while (totalSent < size) {
            int sent = send(_client_socket, data + totalSent, static_cast<int>(size - totalSent), 0);
            if (sent == SOCKET_ERROR) {
                int error = WSAGetLastError();
                std::cerr << "[TestClient] Send error: " << error << std::endl;
                return false;
            }
            totalSent += sent;
        }
        return true;
    }

    std::vector<uint8_t> ReceivePacket() {
        if (!_connected) {
            std::cerr << "[TestClient] Not connected" << std::endl;
            HandleConnectionLoss();
            return std::vector<uint8_t>();
        }

        try {
            // 패킷 크기 수신 (4바이트 헤더)
            uint32_t packetSize = 0;
            if (!ReceiveData(reinterpret_cast<char*>(&packetSize), sizeof(packetSize))) {
                std::cerr << "[TestClient] Failed to receive header" << std::endl;
                HandleConnectionLoss();
                return std::vector<uint8_t>();
            }

            // 네트워크 바이트 순서에서 호스트 바이트 순서로 변환
            packetSize = ntohl(packetSize);

            if (packetSize == 0 || packetSize > 65536) {
                std::cerr << "[TestClient] Invalid packet size: " << packetSize << std::endl;
                HandleConnectionLoss();
                return std::vector<uint8_t>();
            }

            // 실제 패킷 데이터 수신
            std::vector<uint8_t> packetData(packetSize);
            if (!ReceiveData(reinterpret_cast<char*>(packetData.data()), packetSize)) {
                std::cerr << "[TestClient] Failed to receive packet data" << std::endl;
                HandleConnectionLoss();
                return std::vector<uint8_t>();
            }

            std::cout << "[TestClient] Received packet: " << packetSize << " bytes" << std::endl;
            return packetData;
        }
        catch (const std::exception& e) {
            std::cerr << "[TestClient] Exception in ReceivePacket: " << e.what() << std::endl;
            HandleConnectionLoss();
            return std::vector<uint8_t>();
        }
    }

    bool ReceiveData(char* buffer, size_t size) {
        size_t totalReceived = 0;
        while (totalReceived < size) {
            int received = recv(_client_socket, buffer + totalReceived,
                static_cast<int>(size - totalReceived), 0);
            if (received == SOCKET_ERROR) {
                int error = WSAGetLastError();
                std::cerr << "[TestClient] Receive error: " << error << std::endl;
                return false;
            }
            else if (received == 0) {
                std::cerr << "[TestClient] Connection closed by server" << std::endl;
                return false;
            }
            totalReceived += received;
        }
        return true;
    }

    // === 사용자 입력 도우미 함수들 ===

    std::string GetUserInput(const std::string& prompt) {
        std::string input;
        std::cout << prompt;
        std::getline(std::cin, input);
        return input;
    }

    void ClearInputBuffer() {
        std::cin.ignore(10000, '\n');
    }

    // === 인터랙티브 테스트 함수들 ===

    bool TestLoginInteractive() {
        std::cout << "\n=== Login Test ===" << std::endl;

        std::string username = GetUserInput("Enter username: ");
        std::string password = GetUserInput("Enter password: ");

        if (username.empty() || password.empty()) {
            std::cout << "[ERROR] Username and password cannot be empty" << std::endl;
            return false;
        }

        return TestLogin(username, password);
    }

    bool TestCreateAccountInteractive() {
        std::cout << "\n=== Create Account Test ===" << std::endl;

        std::string username = GetUserInput("Enter new username: ");
        std::string password = GetUserInput("Enter new password: ");
        std::string nickname = GetUserInput("Enter nickname: ");

        if (username.empty() || password.empty() || nickname.empty()) {
            std::cout << "[ERROR] All fields must be filled" << std::endl;
            return false;
        }

        return TestCreateAccount(username, password, nickname);
    }

    bool TestChatSendInteractive() {
        std::cout << "\n=== Chat Send Test ===" << std::endl;

        if (_current_user_id == 0) {
            std::cerr << "[FAIL] No logged in user" << std::endl;
            return false;
        }

        std::string message = GetUserInput("Enter chat message: ");
        if (message.empty()) {
            std::cout << "[ERROR] Message cannot be empty" << std::endl;
            return false;
        }

        std::cout << "Chat types: 0=Global, 1=Whisper, 2=Guild" << std::endl;
        std::string chatTypeStr = GetUserInput("Enter chat type (0-2): ");

        uint32_t chatType = 0;
        try {
            chatType = std::stoul(chatTypeStr);
            if (chatType > 2) {
                std::cout << "[ERROR] Invalid chat type. Using Global (0)" << std::endl;
                chatType = 0;
            }
        }
        catch (...) {
            std::cout << "[ERROR] Invalid input. Using Global (0)" << std::endl;
            chatType = 0;
        }

        uint32_t receiverId = 0;
        if (chatType == 1) { // Whisper
            std::string receiverStr = GetUserInput("Enter receiver ID: ");
            try {
                receiverId = std::stoul(receiverStr);
            }
            catch (...) {
                std::cout << "[ERROR] Invalid receiver ID" << std::endl;
                return false;
            }
        }

        return TestChatSend(_current_user_id, receiverId, message, chatType);
    }

    // === 기본 테스트 함수들 ===

    bool TestLogin(const std::string& username, const std::string& password) {
        std::cout << "Testing login with username: " << username << std::endl;

        auto loginPacket = _packet_manager.CreateLoginRequest(username, password);
        if (loginPacket.empty()) {
            std::cerr << "Failed to create login packet: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        if (!SendPacket(loginPacket)) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto response = ReceivePacket();
        if (response.empty()) {
            return false;
        }

        const S2C_Login* loginResponse = _packet_manager.ParseLoginResponse(response.data(), response.size());
        if (!loginResponse) {
            std::cerr << "Failed to parse login response: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        bool success = _packet_manager.IsLoginSuccess(loginResponse);
        if (success) {
            _current_user_id = loginResponse->user_id();
            _current_username = loginResponse->username()->str();
            std::cout << "[PASS] Login SUCCESS - User ID: " << _current_user_id
                << ", Username: " << _current_username
                << ", Nickname: " << loginResponse->nickname()->str()
                << ", Level: " << loginResponse->level() << std::endl;
        }
        else {
            std::cout << "[FAIL] Login FAILED - Result: " << static_cast<int>(loginResponse->result()) << std::endl;
        }

        return success;
    }

    bool TestCreateAccount(const std::string& username, const std::string& password, const std::string& nickname) {
        std::cout << "Testing create account with username: " << username << std::endl;

        auto accountPacket = _packet_manager.CreateAccountRequest(username, password, nickname);
        if (accountPacket.empty()) {
            std::cerr << "Failed to create account packet: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        if (!SendPacket(accountPacket)) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto response = ReceivePacket();
        if (response.empty()) {
            return false;
        }

        const S2C_CreateAccount* accountResponse = _packet_manager.ParseCreateAccountResponse(response.data(), response.size());
        if (!accountResponse) {
            std::cerr << "Failed to parse create account response: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        bool success = _packet_manager.IsCreateAccountSuccess(accountResponse);
        if (success) {
            std::cout << "[PASS] Create Account SUCCESS - User ID: " << accountResponse->user_id()
                << ", Message: " << accountResponse->message()->str() << std::endl;
        }
        else {
            std::cout << "[FAIL] Create Account FAILED - Result: " << static_cast<int>(accountResponse->result())
                << ", Message: " << accountResponse->message()->str() << std::endl;
        }

        return success;
    }

    bool TestPlayerDataQuery() {
        std::cout << "\n=== Player Data Query Test ===" << std::endl;

        if (_current_user_id == 0) {
            std::cerr << "[FAIL] No logged in user" << std::endl;
            return false;
        }

        auto playerPacket = _packet_manager.CreatePlayerDataQueryRequest(_current_user_id);
        if (playerPacket.empty()) {
            std::cerr << "Failed to create player data query packet: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        if (!SendPacket(playerPacket)) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto response = ReceivePacket();
        if (response.empty()) {
            return false;
        }

        const S2C_PlayerData* playerResponse = _packet_manager.ParsePlayerDataResponse(response.data(), response.size());
        if (!playerResponse) {
            std::cerr << "Failed to parse player data response: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        bool success = _packet_manager.IsPlayerDataValid(playerResponse);
        if (success) {
            std::cout << "[PASS] Player Data Query SUCCESS:" << std::endl;
            std::cout << "  User ID: " << playerResponse->user_id() << std::endl;
            std::cout << "  Username: " << playerResponse->username()->str() << std::endl;
            std::cout << "  Nickname: " << playerResponse->nickname()->str() << std::endl;
            std::cout << "  Level: " << playerResponse->level() << std::endl;
            std::cout << "  Exp: " << playerResponse->exp() << std::endl;
            std::cout << "  HP: " << playerResponse->hp() << std::endl;
            std::cout << "  MP: " << playerResponse->mp() << std::endl;
            std::cout << "  Attack: " << playerResponse->attack() << std::endl;
            std::cout << "  Defense: " << playerResponse->defense() << std::endl;
            std::cout << "  Gold: " << playerResponse->gold() << std::endl;
            std::cout << "  Map ID: " << playerResponse->map_id() << std::endl;
            std::cout << "  Position: (" << playerResponse->pos_x() << ", " << playerResponse->pos_y() << ")" << std::endl;
        }
        else {
            std::cout << "[FAIL] Player Data Query FAILED - Result: " << static_cast<int>(playerResponse->result()) << std::endl;
        }

        return success;
    }

    bool TestPlayerDataUpdate() {
        std::cout << "\n=== Player Data Update Test ===" << std::endl;

        if (_current_user_id == 0) {
            std::cerr << "[FAIL] No logged in user" << std::endl;
            return false;
        }

        auto updatePacket = _packet_manager.CreatePlayerDataUpdateRequest(_current_user_id, 2, 150, 120, 60, 10.5f, 20.3f);
        if (updatePacket.empty()) {
            std::cerr << "Failed to create player data update packet: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        if (!SendPacket(updatePacket)) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto response = ReceivePacket();
        if (response.empty()) {
            return false;
        }

        const S2C_PlayerData* updateResponse = _packet_manager.ParsePlayerDataResponse(response.data(), response.size());
        if (!updateResponse) {
            std::cerr << "Failed to parse player data update response: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        bool success = _packet_manager.IsPlayerDataValid(updateResponse);
        if (success) {
            std::cout << "[PASS] Player Data Update SUCCESS" << std::endl;
        }
        else {
            std::cout << "[FAIL] Player Data Update FAILED - Result: " << static_cast<int>(updateResponse->result()) << std::endl;
        }

        return success;
    }

    bool TestItemDataQuery() {
        std::cout << "\n=== Item Data Query Test ===" << std::endl;

        if (_current_user_id == 0) {
            std::cerr << "[FAIL] No logged in user" << std::endl;
            return false;
        }

        auto itemPacket = _packet_manager.CreateItemDataRequest(_current_user_id, 0);
        if (itemPacket.empty()) {
            std::cerr << "Failed to create item data query packet: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        if (!SendPacket(itemPacket)) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto response = ReceivePacket();
        if (response.empty()) {
            return false;
        }

        const S2C_ItemData* itemResponse = _packet_manager.ParseItemDataResponse(response.data(), response.size());
        if (!itemResponse) {
            std::cerr << "Failed to parse item data response: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        bool success = _packet_manager.IsItemDataValid(itemResponse);
        if (success) {
            std::cout << "[PASS] Item Data Query SUCCESS:" << std::endl;
            std::cout << "  User ID: " << itemResponse->user_id() << std::endl;
            std::cout << "  Gold: " << itemResponse->gold() << std::endl;
            std::cout << "  Items:" << std::endl;

            if (itemResponse->items()) {
                for (size_t i = 0; i < itemResponse->items()->size(); ++i) {
                    const auto* item = itemResponse->items()->Get(i);
                    std::cout << "    - ID: " << item->item_id()
                        << ", Name: " << item->item_name()->str()
                        << ", Count: " << item->item_count()
                        << ", Type: " << item->item_type()
                        << ", Price: " << item->base_price() << std::endl;
                }
            }
        }
        else {
            std::cout << "[FAIL] Item Data Query FAILED - Result: " << static_cast<int>(itemResponse->result()) << std::endl;
        }

        return success;
    }

    bool TestMonsterDataQuery() {
        std::cout << "\n=== Monster Data Query Test ===" << std::endl;

        auto monsterPacket = _packet_manager.CreateMonsterDataRequest(0);
        if (monsterPacket.empty()) {
            std::cerr << "Failed to create monster data query packet: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        if (!SendPacket(monsterPacket)) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto response = ReceivePacket();
        if (response.empty()) {
            return false;
        }

        const S2C_MonsterData* monsterResponse = _packet_manager.ParseMonsterDataResponse(response.data(), response.size());
        if (!monsterResponse) {
            std::cerr << "Failed to parse monster data response: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        if (monsterResponse->result() == ResultCode_SUCCESS) {
            std::cout << "[PASS] Monster Data Query SUCCESS:" << std::endl;

            if (monsterResponse->monsters()) {
                for (size_t i = 0; i < monsterResponse->monsters()->size(); ++i) {
                    const auto* monster = monsterResponse->monsters()->Get(i);
                    std::cout << "  - ID: " << monster->monster_id()
                        << ", Name: " << monster->monster_name()->str()
                        << ", Level: " << monster->level()
                        << ", HP: " << monster->hp()
                        << ", Attack: " << monster->attack()
                        << ", Defense: " << monster->defense()
                        << ", Exp Reward: " << monster->exp_reward()
                        << ", Gold Reward: " << monster->gold_reward() << std::endl;
                }
            }
            return true;
        }
        else {
            std::cout << "[FAIL] Monster Data Query FAILED - Result: " << static_cast<int>(monsterResponse->result()) << std::endl;
            return false;
        }
    }

    bool TestChatSend(uint32_t senderId, uint32_t receiverId, const std::string& message, uint32_t chatType) {
        auto chatPacket = _packet_manager.CreateChatSendRequest(senderId, receiverId, message, chatType);
        if (chatPacket.empty()) {
            std::cerr << "Failed to create chat send packet: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        if (!SendPacket(chatPacket)) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto response = ReceivePacket();
        if (response.empty()) {
            return false;
        }

        const S2C_PlayerChat* chatResponse = _packet_manager.ParsePlayerChatResponse(response.data(), response.size());
        if (!chatResponse) {
            std::cerr << "Failed to parse chat response: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        if (chatResponse->result() == ResultCode_SUCCESS) {
            std::cout << "[PASS] Chat Send SUCCESS" << std::endl;
            return true;
        }
        else {
            std::cout << "[FAIL] Chat Send FAILED - Result: " << static_cast<int>(chatResponse->result()) << std::endl;
            return false;
        }
    }

    bool TestShopList() {
        std::cout << "\n=== Shop List Test ===" << std::endl;

        auto shopPacket = _packet_manager.CreateShopListRequest(0, 0);
        if (shopPacket.empty()) {
            std::cerr << "Failed to create shop list packet: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        if (!SendPacket(shopPacket)) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto response = ReceivePacket();
        if (response.empty()) {
            return false;
        }

        const S2C_ShopList* shopResponse = _packet_manager.ParseShopListResponse(response.data(), response.size());
        if (!shopResponse) {
            std::cerr << "Failed to parse shop list response: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        bool success = _packet_manager.IsShopListValid(shopResponse);
        if (success) {
            std::cout << "[PASS] Shop List Query SUCCESS:" << std::endl;

            if (shopResponse->shops()) {
                for (size_t i = 0; i < shopResponse->shops()->size(); ++i) {
                    const auto* shop = shopResponse->shops()->Get(i);
                    std::cout << "  - ID: " << shop->shop_id()
                        << ", Name: " << shop->shop_name()->str()
                        << ", Type: " << shop->shop_type()
                        << ", Map ID: " << shop->map_id()
                        << ", Position: (" << shop->pos_x() << ", " << shop->pos_y() << ")" << std::endl;
                }
            }
        }
        else {
            std::cout << "[FAIL] Shop List Query FAILED - Result: " << static_cast<int>(shopResponse->result()) << std::endl;
        }

        return success;
    }

    bool TestShopItems() {
        std::cout << "\n=== Shop Items Test ===" << std::endl;

        auto shopItemsPacket = _packet_manager.CreateShopItemsRequest(1);
        if (shopItemsPacket.empty()) {
            std::cerr << "Failed to create shop items packet: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        if (!SendPacket(shopItemsPacket)) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto response = ReceivePacket();
        if (response.empty()) {
            return false;
        }

        const S2C_ShopItems* shopItemsResponse = _packet_manager.ParseShopItemsResponse(response.data(), response.size());
        if (!shopItemsResponse) {
            std::cerr << "Failed to parse shop items response: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        bool success = _packet_manager.IsShopItemsValid(shopItemsResponse);
        if (success) {
            std::cout << "[PASS] Shop Items Query SUCCESS for Shop ID " << shopItemsResponse->shop_id() << ":" << std::endl;

            if (shopItemsResponse->items()) {
                for (size_t i = 0; i < shopItemsResponse->items()->size(); ++i) {
                    const auto* item = shopItemsResponse->items()->Get(i);
                    std::cout << "  - ID: " << item->item_id()
                        << ", Name: " << item->item_name()->str()
                        << ", Type: " << item->item_type()
                        << ", Price: " << item->base_price()
                        << ", Attack: +" << item->attack_bonus()
                        << ", Defense: +" << item->defense_bonus() << std::endl;
                }
            }
        }
        else {
            std::cout << "[FAIL] Shop Items Query FAILED - Result: " << static_cast<int>(shopItemsResponse->result()) << std::endl;
        }

        return success;
    }

    bool TestShopTransaction() {
        std::cout << "\n=== Shop Transaction Test ===" << std::endl;

        if (_current_user_id == 0) {
            std::cerr << "[FAIL] No logged in user" << std::endl;
            return false;
        }

        auto transactionPacket = _packet_manager.CreateShopTransactionRequest(_current_user_id, 1, 1, 1, 0);
        if (transactionPacket.empty()) {
            std::cerr << "Failed to create shop transaction packet: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        if (!SendPacket(transactionPacket)) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto response = ReceivePacket();
        if (response.empty()) {
            return false;
        }

        const S2C_ShopTransaction* transactionResponse = _packet_manager.ParseShopTransactionResponse(response.data(), response.size());
        if (!transactionResponse) {
            std::cerr << "Failed to parse shop transaction response: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        bool success = _packet_manager.IsShopTransactionSuccess(transactionResponse);
        if (success) {
            std::cout << "[PASS] Shop Transaction SUCCESS:" << std::endl;
            std::cout << "  Message: " << transactionResponse->message()->str() << std::endl;
            std::cout << "  Updated Gold: " << transactionResponse->updated_gold() << std::endl;
        }
        else {
            std::cout << "[FAIL] Shop Transaction FAILED - Result: " << static_cast<int>(transactionResponse->result()) << std::endl;
            std::cout << "  Message: " << transactionResponse->message()->str() << std::endl;
        }

        return success;
    }

    bool TestLogout() {
        std::cout << "\n=== Logout Test ===" << std::endl;

        if (_current_user_id == 0) {
            std::cerr << "[FAIL] No logged in user" << std::endl;
            return false;
        }

        auto logoutPacket = _packet_manager.CreateLogoutRequest(_current_user_id);
        if (logoutPacket.empty()) {
            std::cerr << "Failed to create logout packet: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        if (!SendPacket(logoutPacket)) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto response = ReceivePacket();
        if (response.empty()) {
            return false;
        }

        const S2C_Logout* logoutResponse = _packet_manager.ParseLogoutResponse(response.data(), response.size());
        if (!logoutResponse) {
            std::cerr << "Failed to parse logout response: " << _packet_manager.GetLastError() << std::endl;
            return false;
        }

        if (logoutResponse->result() == ResultCode_SUCCESS) {
            std::cout << "[PASS] Logout SUCCESS - Message: " << logoutResponse->message()->str() << std::endl;
            _current_user_id = 0;
            _current_username.clear();
            return true;
        }
        else {
            std::cout << "[FAIL] Logout FAILED - Result: " << static_cast<int>(logoutResponse->result()) << std::endl;
            return false;
        }
    }

    void ShowMenu() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "         Server Test Client Menu" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "0. Run All Tests (Auto Mode)" << std::endl;

        for (const auto& pair : _test_functions) {
            std::cout << pair.first << ". " << pair.second.first << std::endl;
        }

        std::cout << "90. Show Connection Status" << std::endl;
        std::cout << "99. Exit" << std::endl;
        std::cout << "========================================" << std::endl;

        if (_current_user_id > 0) {
            std::cout << "Current User: " << _current_username << " (ID: " << _current_user_id << ")" << std::endl;
        }
        else {
            std::cout << "Current User: Not logged in" << std::endl;
        }

        std::cout << "Connection Status: " << (_connected ? "[CONNECTED]" : "[DISCONNECTED]") << std::endl;
        std::cout << "Enter your choice: ";
    }

    void RunManualMode() {
        std::cout << "\n[Manual Test Mode Started]" << std::endl;

        while (true) {
            ShowMenu();

            int choice;
            std::cin >> choice;
            ClearInputBuffer(); // 입력 버퍼 정리

            if (std::cin.fail()) {
                std::cin.clear();
                ClearInputBuffer();
                std::cout << "[ERROR] Invalid input. Please enter a number." << std::endl;
                continue;
            }

            switch (choice) {
            case 0:
                RunAllTests();
                break;
            case 90:
                ShowConnectionStatus();
                break;
            case 99:
                std::cout << "[INFO] Exiting test client..." << std::endl;
                return;
            default:
                if (_test_functions.find(choice) != _test_functions.end()) {
                    std::cout << "\n[RUNNING] " << _test_functions[choice].first << std::endl;
                    bool result = _test_functions[choice].second();
                    std::cout << "\n[RESULT] " << (result ? "[PASSED]" : "[FAILED]") << std::endl;
                }
                else {
                    std::cout << "[ERROR] Invalid choice. Please try again." << std::endl;
                }
                break;
            }

            if (choice != 99) {
                std::cout << "\nPress Enter to continue...";
                std::cin.get();
            }
        }
    }

    void ShowConnectionStatus() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "         Connection Status" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Server: " << _server_ip << ":" << _server_port << std::endl;
        std::cout << "Status: " << (_connected ? "[CONNECTED]" : "[DISCONNECTED]") << std::endl;
        std::cout << "Socket: " << (_client_socket != INVALID_SOCKET ? "Valid" : "Invalid") << std::endl;

        std::cout << "Current User: ";
        if (_current_user_id > 0) {
            std::cout << _current_username << " (ID: " << _current_user_id << ")" << std::endl;
        }
        else {
            std::cout << "Not logged in" << std::endl;
        }
        std::cout << "========================================" << std::endl;
    }

    void RunAllTests() {
        std::cout << "\n[Auto Test Mode Started]" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Starting Comprehensive Server Test" << std::endl;
        std::cout << "Note: Login test will use default testuser/testpass" << std::endl;
        std::cout << "========================================" << std::endl;

        int totalTests = 0;
        int passedTests = 0;
        std::vector<std::pair<std::string, bool>> testResults;

        // 기존 계정으로 로그인 테스트 (고정 계정 사용)
        totalTests++;
        std::cout << "\n[TEST " << totalTests << "] Login with default account" << std::endl;
        bool loginResult = TestLogin("testuser", "testpass");
        testResults.push_back({ "Login Test", loginResult });
        if (loginResult) {
            passedTests++;
        }
        else {
            std::cout << "[WARNING] Login test failed - some user-dependent tests will be skipped" << std::endl;
        }

        // 사용자 로그인이 필요한 테스트들
        if (loginResult && _current_user_id > 0) {
            // 플레이어 데이터 조회
            totalTests++;
            std::cout << "\n[TEST " << totalTests << "] Player Data Query" << std::endl;
            bool playerQueryResult = TestPlayerDataQuery();
            testResults.push_back({ "Player Data Query", playerQueryResult });
            if (playerQueryResult) passedTests++;

            // 플레이어 데이터 업데이트
            totalTests++;
            std::cout << "\n[TEST " << totalTests << "] Player Data Update" << std::endl;
            bool playerUpdateResult = TestPlayerDataUpdate();
            testResults.push_back({ "Player Data Update", playerUpdateResult });
            if (playerUpdateResult) passedTests++;

            // 아이템 데이터 조회
            totalTests++;
            std::cout << "\n[TEST " << totalTests << "] Item Data Query" << std::endl;
            bool itemQueryResult = TestItemDataQuery();
            testResults.push_back({ "Item Data Query", itemQueryResult });
            if (itemQueryResult) passedTests++;

            // 채팅 전송 (고정 메시지)
            totalTests++;
            std::cout << "\n[TEST " << totalTests << "] Chat Send" << std::endl;
            bool chatResult = TestChatSend(_current_user_id, 0, "Auto test message", 0);
            testResults.push_back({ "Chat Send", chatResult });
            if (chatResult) passedTests++;

            // 상점 트랜잭션
            totalTests++;
            std::cout << "\n[TEST " << totalTests << "] Shop Transaction" << std::endl;
            bool shopTransactionResult = TestShopTransaction();
            testResults.push_back({ "Shop Transaction", shopTransactionResult });
            if (shopTransactionResult) passedTests++;
        }

        // 사용자 로그인 없이도 테스트 가능한 기능들

        // 몬스터 데이터 조회
        totalTests++;
        std::cout << "\n[TEST " << totalTests << "] Monster Data Query" << std::endl;
        bool monsterResult = TestMonsterDataQuery();
        testResults.push_back({ "Monster Data Query", monsterResult });
        if (monsterResult) passedTests++;

        // 상점 목록 조회
        totalTests++;
        std::cout << "\n[TEST " << totalTests << "] Shop List Query" << std::endl;
        bool shopListResult = TestShopList();
        testResults.push_back({ "Shop List Query", shopListResult });
        if (shopListResult) passedTests++;

        // 상점 아이템 조회
        totalTests++;
        std::cout << "\n[TEST " << totalTests << "] Shop Items Query" << std::endl;
        bool shopItemsResult = TestShopItems();
        testResults.push_back({ "Shop Items Query", shopItemsResult });
        if (shopItemsResult) passedTests++;

        // 로그아웃 (로그인된 상태에서만)
        if (loginResult && _current_user_id > 0) {
            totalTests++;
            std::cout << "\n[TEST " << totalTests << "] Logout" << std::endl;
            bool logoutResult = TestLogout();
            testResults.push_back({ "Logout Test", logoutResult });
            if (logoutResult) passedTests++;
        }

        // 상세 결과 출력
        std::cout << "\n========================================" << std::endl;
        std::cout << "           Detailed Test Results" << std::endl;
        std::cout << "========================================" << std::endl;

        for (size_t i = 0; i < testResults.size(); ++i) {
            std::cout << std::setw(3) << (i + 1) << ". " << std::setw(25) << std::left
                << testResults[i].first << " : "
                << (testResults[i].second ? "[PASSED]" : "[FAILED]") << std::endl;
        }

        // 최종 결과 요약
        std::cout << "\n========================================" << std::endl;
        std::cout << "           Final Test Summary" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Total Tests: " << totalTests << std::endl;
        std::cout << "Passed: " << passedTests << " [PASS]" << std::endl;
        std::cout << "Failed: " << (totalTests - passedTests) << " [FAIL]" << std::endl;
        std::cout << "Success Rate: " << std::fixed << std::setprecision(1)
            << (passedTests * 100.0 / totalTests) << "%" << std::endl;
        std::cout << "========================================" << std::endl;

        if (passedTests == totalTests) {
            std::cout << "[SUCCESS] ALL TESTS PASSED! Server is working perfectly!" << std::endl;
        }
        else if (passedTests > totalTests * 0.8) {
            std::cout << "[WARNING] MOSTLY SUCCESSFUL! Minor issues detected." << std::endl;
        }
        else {
            std::cout << "[ERROR] MULTIPLE FAILURES! Please check server implementation." << std::endl;
        }
    }

    void ShowStartupBanner() {
        std::cout << "========================================" << std::endl;
        std::cout << "     DB Server Test Client v2.0" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Server: " << _server_ip << ":" << _server_port << std::endl;
        std::cout << "Features:" << std::endl;
        std::cout << "  [+] Interactive user input" << std::endl;
        std::cout << "  [+] Connection loss handling" << std::endl;
        std::cout << "  [+] Auto & Manual test modes" << std::endl;
        std::cout << "  [+] Comprehensive testing" << std::endl;
        std::cout << "========================================" << std::endl;
    }
};

int main() {
    TestClient client;

    client.ShowStartupBanner();

    if (!client.Initialize()) {
        std::cerr << "[ERROR] Failed to initialize client" << std::endl;
        return -1;
    }

    if (!client.Connect()) {
        std::cerr << "[ERROR] Failed to connect to server" << std::endl;
        std::cout << "Please ensure the server is running and try again." << std::endl;
        return -1;
    }

    // 서버 준비 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "\nSelect test mode:" << std::endl;
    std::cout << "1. Auto Mode (Run all tests automatically)" << std::endl;
    std::cout << "2. Manual Mode (Interactive menu)" << std::endl;
    std::cout << "Enter your choice (1 or 2): ";

    int mode;
    std::cin >> mode;
    std::cin.ignore(); // 입력 버퍼 정리

    try {
        if (mode == 1) {
            client.RunAllTests();
        }
        else {
            client.RunManualMode();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[ERROR] Test execution failed: " << e.what() << std::endl;
        return -1;
    }

    std::cout << "\n[INFO] Test client finished. Goodbye!" << std::endl;
    return 0;
}