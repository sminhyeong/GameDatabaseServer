#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define NOMINMAX

#include <iostream>
#include <winsock2.h>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include "ClientPacketManager.h"
#include "UserEvent_generated.h"

#pragma comment(lib, "ws2_32.lib")

class GameClient {
private:
    SOCKET sock;
    ClientPacketManager packetManager;
    uint32_t currentUserId;
    bool isConnected;
    std::string currentUsername;

public:
    GameClient() : sock(INVALID_SOCKET), currentUserId(0), isConnected(false) {
        WSAData wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }

    ~GameClient() {
        Disconnect();
        WSACleanup();
    }

    bool Connect(const char* ip, int port) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            std::cerr << "���� ���� ����" << std::endl;
            return false;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr(ip);
        serverAddr.sin_port = htons(port);

        if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "���� ���� ����: " << WSAGetLastError() << std::endl;
            closesocket(sock);
            sock = INVALID_SOCKET;
            return false;
        }

        isConnected = true;
        std::cout << "���� ���� ����! (" << ip << ":" << port << ")" << std::endl;
        return true;
    }

    void Disconnect() {
        if (sock != INVALID_SOCKET) {
            closesocket(sock);
            sock = INVALID_SOCKET;
        }
        isConnected = false;
        currentUserId = 0;
        currentUsername.clear();
    }

    bool SendPacket(const std::vector<uint8_t>& data) {
        if (!isConnected || data.empty()) {
            return false;
        }

        // ��Ŷ ũ�� ���� (4����Ʈ ���)
        uint32_t size = htonl(static_cast<uint32_t>(data.size()));
        if (send(sock, (char*)&size, sizeof(size), 0) <= 0) {
            std::cerr << "��� ���� ����" << std::endl;
            return false;
        }

        // ���� ������ ����
        int totalSent = 0;
        while (totalSent < data.size()) {
            int sent = send(sock, (char*)data.data() + totalSent,
                data.size() - totalSent, 0);
            if (sent <= 0) {
                std::cerr << "������ ���� ����" << std::endl;
                return false;
            }
            totalSent += sent;
        }

        std::cout << "[SEND] ��Ŷ ���� �Ϸ�: " << data.size() << " bytes" << std::endl;
        return true;
    }

    std::vector<uint8_t> ReceivePacket() {
        if (!isConnected) {
            return std::vector<uint8_t>();
        }

        // ��Ŷ ũ�� ���� (4����Ʈ ���)
        uint32_t packetSize = 0;
        int headerReceived = recv(sock, (char*)&packetSize, sizeof(packetSize), 0);

        if (headerReceived <= 0) {
            std::cerr << "��� ���� ����" << std::endl;
            return std::vector<uint8_t>();
        }

        packetSize = ntohl(packetSize);
        if (packetSize == 0 || packetSize > 65536) {
            std::cerr << "�߸��� ��Ŷ ũ��: " << packetSize << std::endl;
            return std::vector<uint8_t>();
        }

        // ���� ��Ŷ ������ ����
        std::vector<uint8_t> packetData(packetSize);
        int totalReceived = 0;

        while (totalReceived < packetSize) {
            int received = recv(sock, (char*)packetData.data() + totalReceived,
                packetSize - totalReceived, 0);
            if (received <= 0) {
                std::cerr << "��Ŷ ������ ���� ����" << std::endl;
                return std::vector<uint8_t>();
            }
            totalReceived += received;
        }

        std::cout << "[RECV] ��Ŷ ���� �Ϸ�: " << packetSize << " bytes" << std::endl;
        return packetData;
    }

    bool Login(const std::string& username, const std::string& password) {
        std::cout << "\n=== �α��� �õ� ===" << std::endl;
        std::cout << "����ڸ�: " << username << std::endl;

        // 1. �α��� ��û ��Ŷ ����
        auto loginPacket = packetManager.CreateLoginRequest(username, password);
        if (loginPacket.empty()) {
            std::cerr << "�α��� ��Ŷ ���� ����: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 2. ��Ŷ ����
        if (!SendPacket(loginPacket)) {
            return false;
        }

        // 3. ���� ����
        auto responseData = ReceivePacket();
        if (responseData.empty()) {
            return false;
        }

        // 4. ���� �Ľ�
        const S2C_Login* loginResponse = packetManager.ParseLoginResponse(responseData.data(), responseData.size());
        if (!loginResponse) {
            std::cerr << "�α��� ���� �Ľ� ����: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 5. ��� Ȯ��
        if (packetManager.IsLoginSuccess(loginResponse)) {
            currentUserId = loginResponse->user_id();
            currentUsername = loginResponse->username()->str();
            std::cout << "? �α��� ����!" << std::endl;
            std::cout << "   ����� ID: " << currentUserId << std::endl;
            std::cout << "   ����ڸ�: " << currentUsername << std::endl;
            std::cout << "   ����: " << loginResponse->level() << std::endl;
            return true;
        }
        else {
            std::cout << "? �α��� ����: " << packetManager.GetResultCodeName(loginResponse->result()) << std::endl;
            return false;
        }
    }

    bool Logout() {
        if (currentUserId == 0) {
            std::cerr << "? �α��� ���°� �ƴմϴ�!" << std::endl;
            return false;
        }

        std::cout << "\n=== �α׾ƿ� �õ� ===" << std::endl;
        std::cout << "�����: " << currentUsername << " (ID: " << currentUserId << ")" << std::endl;

        // 1. �α׾ƿ� ��û ��Ŷ ����
        auto logoutPacket = packetManager.CreateLogoutRequest(currentUserId);
        if (logoutPacket.empty()) {
            std::cerr << "? �α׾ƿ� ��Ŷ ���� ����: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 2. ��Ŷ ����
        if (!SendPacket(logoutPacket)) {
            return false;
        }

        // 3. ���� ����
        auto responseData = ReceivePacket();
        if (responseData.empty()) {
            return false;
        }

        // 4. ���� �Ľ�
        const S2C_Logout* logoutResponse = packetManager.ParseLogoutResponse(responseData.data(), responseData.size());
        if (!logoutResponse) {
            std::cerr << "? �α׾ƿ� ���� �Ľ� ����: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 5. ��� Ȯ��
        if (logoutResponse->result() == ResultCode_SUCCESS) {
            std::cout << "? �α׾ƿ� ����!" << std::endl;
            if (logoutResponse->message()) {
                std::cout << "   �޽���: " << logoutResponse->message()->c_str() << std::endl;
            }

            // ���� ���� �ʱ�ȭ
            currentUserId = 0;
            currentUsername.clear();
            return true;
        }
        else {
            std::cout << "? �α׾ƿ� ����: " << packetManager.GetResultCodeName(logoutResponse->result()) << std::endl;
            return false;
        }
    }

    bool CreateAccount(const std::string& username, const std::string& password, const std::string& nickname) {
        std::cout << "\n=== ���� ���� �õ� ===" << std::endl;
        std::cout << "����ڸ�: " << username << ", �г���: " << nickname << std::endl;

        // 1. ���� ���� ��û ��Ŷ ����
        auto accountPacket = packetManager.CreateAccountRequest(username, password, nickname);
        if (accountPacket.empty()) {
            std::cerr << "���� ���� ��Ŷ ���� ����: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 2. ��Ŷ ����
        if (!SendPacket(accountPacket)) {
            return false;
        }

        // 3. ���� ����
        auto responseData = ReceivePacket();
        if (responseData.empty()) {
            return false;
        }

        // 4. ���� �Ľ�
        const S2C_CreateAccount* accountResponse = packetManager.ParseCreateAccountResponse(responseData.data(), responseData.size());
        if (!accountResponse) {
            std::cerr << "���� ���� ���� �Ľ� ����: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 5. ��� Ȯ��
        if (packetManager.IsCreateAccountSuccess(accountResponse)) {
            std::cout << "? ���� ���� ����!" << std::endl;
            std::cout << "   �� ����� ID: " << accountResponse->user_id() << std::endl;
            std::cout << "   �޽���: " << accountResponse->message()->c_str() << std::endl;
            return true;
        }
        else {
            std::cout << "? ���� ���� ����: " << accountResponse->message()->c_str() << std::endl;
            return false;
        }
    }

    bool GetItemData() {
        if (currentUserId == 0) {
            std::cerr << "? �α����� �ʿ��մϴ�!" << std::endl;
            return false;
        }

        std::cout << "\n=== ��ü ������ ������ ��ȸ ===" << std::endl;
        std::cout << "����� ID: " << currentUserId << std::endl;

        // 1. ������ ������ ��û ��Ŷ ���� (request_type = 0: ��ȸ)
        auto itemPacket = packetManager.CreateItemDataRequest(currentUserId, 0, 0, 0);
        if (itemPacket.empty()) {
            std::cerr << "������ ������ ��Ŷ ���� ����: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 2. ��Ŷ ����
        if (!SendPacket(itemPacket)) {
            return false;
        }

        // 3. ���� ����
        auto responseData = ReceivePacket();
        if (responseData.empty()) {
            return false;
        }

        // 4. ���� �Ľ�
        const S2C_ItemData* itemResponse = packetManager.ParseItemDataResponse(responseData.data(), responseData.size());
        if (!itemResponse) {
            std::cerr << "������ ������ ���� �Ľ� ����: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 5. ��� Ȯ�� �� ���
        if (packetManager.IsItemDataValid(itemResponse)) {
            std::cout << "? ������ ������ ��ȸ ����!" << std::endl;
            std::cout << "?? ���� ���: " << itemResponse->gold() << std::endl;

            if (itemResponse->items() && itemResponse->items()->size() > 0) {
                std::cout << "?? ���� ������ ���:" << std::endl;
                std::cout << "����������������������������������������������������������������������������������������������" << std::endl;
                std::cout << "�� ID   �� �����۸�       �� ���� �� Ÿ��         ��" << std::endl;
                std::cout << "����������������������������������������������������������������������������������������������" << std::endl;

                for (size_t i = 0; i < itemResponse->items()->size(); ++i) {
                    const ItemData* item = itemResponse->items()->Get(i);
                    if (item) {
                        std::string typeStr;
                        switch (item->item_type()) {
                        case 0: typeStr = "����"; break;
                        case 1: typeStr = "��"; break;
                        case 2: typeStr = "�Һ�ǰ"; break;
                        default: typeStr = "��Ÿ"; break;
                        }

                        printf("�� %-4d �� %-14s �� %-4d �� %-12s ��\n",
                            item->item_id(),
                            item->item_name()->c_str(),
                            item->item_count(),
                            typeStr.c_str());
                    }
                }
                std::cout << "����������������������������������������������������������������������������������������������" << std::endl;

                // ù ��° ������ �� ���� ���
                const ItemData* firstItem = itemResponse->items()->Get(0);
                if (firstItem) {
                    std::cout << "\n?? [ù ��° ������ �� ����]" << std::endl;
                    std::cout << "   ?? ������ ID: " << firstItem->item_id() << std::endl;
                    std::cout << "   ?? �����۸�: " << firstItem->item_name()->c_str() << std::endl;
                    std::cout << "   ?? ����: " << firstItem->item_count() << std::endl;
                    std::cout << "   ???  Ÿ��: " << firstItem->item_type() << " (";
                    switch (firstItem->item_type()) {
                    case 0: std::cout << "����"; break;
                    case 1: std::cout << "��"; break;
                    case 2: std::cout << "�Һ�ǰ"; break;
                    default: std::cout << "��Ÿ"; break;
                    }
                    std::cout << ")" << std::endl;
                }
            }
            else {
                std::cout << "?? ������ �������� �����ϴ�." << std::endl;
            }
            return true;
        }
        else {
            std::cout << "? ������ ������ ��ȸ ����: " << packetManager.GetResultCodeName(itemResponse->result()) << std::endl;
            return false;
        }
    }

    bool GetSpecificItemInfo() {
        if (currentUserId == 0) {
            std::cerr << "? �α����� �ʿ��մϴ�!" << std::endl;
            return false;
        }

        uint32_t itemId;
        std::cout << "\n=== Ư�� ������ ���� ��ȸ ===" << std::endl;
        std::cout << "��ȸ�� ������ ID�� �Է��ϼ���: ";
        std::cin >> itemId;

        if (itemId == 0) {
            std::cerr << "? �߸��� ������ ID�Դϴ�." << std::endl;
            return false;
        }

        std::cout << "������ ID " << itemId << " ������ ��ȸ�մϴ�..." << std::endl;

        // 1. ��ü ������ ����� ��ȸ�� �� Ư�� ������ ã��
        auto itemPacket = packetManager.CreateItemDataRequest(currentUserId, 0, 0, 0);
        if (itemPacket.empty()) {
            std::cerr << "? ������ ������ ��Ŷ ���� ����: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 2. ��Ŷ ����
        if (!SendPacket(itemPacket)) {
            return false;
        }

        // 3. ���� ����
        auto responseData = ReceivePacket();
        if (responseData.empty()) {
            return false;
        }

        // 4. ���� �Ľ�
        const S2C_ItemData* itemResponse = packetManager.ParseItemDataResponse(responseData.data(), responseData.size());
        if (!itemResponse) {
            std::cerr << "? ������ ������ ���� �Ľ� ����: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 5. Ư�� ������ ã��
        if (packetManager.IsItemDataValid(itemResponse)) {
            if (itemResponse->items() && itemResponse->items()->size() > 0) {
                const ItemData* targetItem = nullptr;

                // �Է��� ID�� ��ġ�ϴ� ������ ã��
                for (size_t i = 0; i < itemResponse->items()->size(); ++i) {
                    const ItemData* item = itemResponse->items()->Get(i);
                    if (item && item->item_id() == itemId) {
                        targetItem = item;
                        break;
                    }
                }

                if (targetItem) {
                    std::cout << "? �������� ã�ҽ��ϴ�!" << std::endl;
                    std::cout << "������������������������������������������������������������������������������" << std::endl;
                    std::cout << "��           ������ �� ����          ��" << std::endl;
                    std::cout << "������������������������������������������������������������������������������" << std::endl;
                    printf("�� ?? ������ ID: %-21d ��\n", targetItem->item_id());
                    printf("�� ?? �����۸�: %-23s ��\n", targetItem->item_name()->c_str());
                    printf("�� ?? ���� ����: %-22d ��\n", targetItem->item_count());
                    printf("�� ???  ������ Ÿ��: %-19d ��\n", targetItem->item_type());
                    std::cout << "�� ???  Ÿ�� ����: ";
                    switch (targetItem->item_type()) {
                    case 0: std::cout << "����                     ��" << std::endl; break;
                    case 1: std::cout << "��                   ��" << std::endl; break;
                    case 2: std::cout << "�Һ�ǰ                   ��" << std::endl; break;
                    default: std::cout << "��Ÿ                     ��" << std::endl; break;
                    }
                    std::cout << "������������������������������������������������������������������������������" << std::endl;

                    // ������ ��� ���� ���� �� �߰� ����
                    std::cout << "\n?? [�߰� ����]" << std::endl;
                    std::cout << "   ?? ���� ���� ���: " << itemResponse->gold() << std::endl;

                    if (targetItem->item_count() > 0) {
                        std::cout << "   ? ��� ������ �������Դϴ�." << std::endl;
                    }
                    else {
                        std::cout << "   ? �������� ���� �������Դϴ�." << std::endl;
                    }

                    return true;
                }
                else {
                    std::cout << "? �ش� ID(" << itemId << ")�� �������� �����ϰ� ���� �ʽ��ϴ�." << std::endl;

                    // ���� ���� ������ ID ��� ǥ��
                    std::cout << "\n?? ���� ���� ���� ������ ID ���:" << std::endl;
                    for (size_t i = 0; i < itemResponse->items()->size(); ++i) {
                        const ItemData* item = itemResponse->items()->Get(i);
                        if (item) {
                            std::cout << "   ? ID " << item->item_id()
                                << ": " << item->item_name()->c_str() << std::endl;
                        }
                    }
                    return false;
                }
            }
            else {
                std::cout << "? ������ �������� �����ϴ�." << std::endl;
                return false;
            }
        }
        else {
            std::cout << "? ������ ������ ��ȸ ����: " << packetManager.GetResultCodeName(itemResponse->result()) << std::endl;
            return false;
        }
    }

    void ShowCurrentStatus() {
        std::cout << "\n������������������������������������������������������������������������������" << std::endl;
        std::cout << "��             ���� ����               ��" << std::endl;
        std::cout << "������������������������������������������������������������������������������" << std::endl;
        if (currentUserId > 0) {
            std::cout << "�� ?? �α��� ����: ���� ��            ��" << std::endl;
            printf("�� ?? ����ڸ�: %-22s ��\n", currentUsername.c_str());
            printf("�� ?? ����� ID: %-21d ��\n", currentUserId);
        }
        else {
            std::cout << "�� ?? �α��� ����: �α׾ƿ�           ��" << std::endl;
            std::cout << "�� ?? ����ڸ�: ����                  ��" << std::endl;
            std::cout << "�� ?? ����� ID: ����                 ��" << std::endl;
        }
        std::cout << "�� ?? ���� ����: " << (isConnected ? "�����             ��" : "���� �ȵ�         ��") << std::endl;
        std::cout << "������������������������������������������������������������������������������" << std::endl;
    }

    void ShowMenu() {
        std::cout << "\n������������������������������������������������������������������������������" << std::endl;
        std::cout << "��           ���� Ŭ���̾�Ʈ           ��" << std::endl;
        std::cout << "������������������������������������������������������������������������������" << std::endl;
        std::cout << "�� 1. �α���                           ��" << std::endl;
        std::cout << "�� 2. �α׾ƿ�                         ��" << std::endl;
        std::cout << "�� 3. ���� ����                        ��" << std::endl;
        std::cout << "�� 4. ��ü ������ ������ ��ȸ          ��" << std::endl;
        std::cout << "�� 5. Ư�� ������ ���� ��ȸ            ��" << std::endl;
        std::cout << "�� 6. ���� ���� Ȯ��                   ��" << std::endl;
        std::cout << "�� 0. ���� ����                        ��" << std::endl;
        std::cout << "������������������������������������������������������������������������������" << std::endl;
        std::cout << "���� >> ";
    }

    void Run() {
        int choice;
        std::string username, password, nickname;

        std::cout << "\n?? ���� Ŭ���̾�Ʈ�� ���� ���� ȯ���մϴ�!" << std::endl;

        while (isConnected) {
            ShowMenu();

            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "? �߸��� �Է��Դϴ�. ���ڸ� �Է��ϼ���." << std::endl;
                continue;
            }

            switch (choice) {
            case 1: // �α���
                if (currentUserId > 0) {
                    std::cout << "??  �̹� �α��ε� �����Դϴ�. (�����: " << currentUsername << ")" << std::endl;
                }
                else {
                    std::cout << "\n?? �α��� ������ �Է��ϼ���." << std::endl;
                    std::cout << "����ڸ�: ";
                    std::cin >> username;
                    std::cout << "��й�ȣ: ";
                    std::cin >> password;
                    Login(username, password);
                }
                break;

            case 2: // �α׾ƿ�
                if (currentUserId == 0) {
                    std::cout << "??  �α��ε� ���°� �ƴմϴ�." << std::endl;
                }
                else {
                    Logout();
                }
                break;

            case 3: // ���� ����
                std::cout << "\n?? ���� ���� ������ �Է��ϼ���." << std::endl;
                std::cout << "����ڸ�: ";
                std::cin >> username;
                std::cout << "��й�ȣ: ";
                std::cin >> password;
                std::cout << "�г���: ";
                std::cin >> nickname;
                CreateAccount(username, password, nickname);
                break;

            case 4: // ��ü ������ ������ ��ȸ
                GetItemData();
                break;

            case 5: // Ư�� ������ ���� ��ȸ
                GetSpecificItemInfo();
                break;

            case 6: // ���� ���� Ȯ��
                ShowCurrentStatus();
                break;

            case 0: // ���� ����
                if (currentUserId > 0) {
                    std::cout << "\n?? �α׾ƿ� �� �����մϴ�..." << std::endl;
                    Logout();
                }
                std::cout << "?? Ŭ���̾�Ʈ�� �����մϴ�." << std::endl;
                return;

            default:
                std::cout << "? �߸��� �����Դϴ�. 0-6 ������ ���ڸ� �Է��ϼ���." << std::endl;
                break;
            }

            // ��� Ȯ���� ���� ��� ���
            std::cout << "\n����Ϸ��� Enter Ű�� ��������...";
            std::cin.ignore();
            std::cin.get();
        }
    }
};

int main() {
    // �ܼ� â ���� ����
    SetConsoleTitle(L"���� Ŭ���̾�Ʈ - Enhanced Version");

    std::cout << "??????????????????????????????????????????????????????????" << std::endl;
    std::cout << "?                                                        ?" << std::endl;
    std::cout << "?            ?? ���� ���� �׽�Ʈ Ŭ���̾�Ʈ ??            ?" << std::endl;
    std::cout << "?                                                        ?" << std::endl;
    std::cout << "?              Enhanced with Logout & Item Search       ?" << std::endl;
    std::cout << "?                                                        ?" << std::endl;
    std::cout << "??????????????????????????????????????????????????????????" << std::endl;

    GameClient client;

    // ������ ����
    std::cout << "\n?? ������ ���� ��..." << std::endl;
    if (!client.Connect("127.0.0.1", 7777)) {
        std::cerr << "? ���� ���� ����!" << std::endl;
        std::cout << "\n���� ������ Ȯ�����ּ���:" << std::endl;
        std::cout << "? ������ ���� ������ Ȯ��" << std::endl;
        std::cout << "? IP �ּҿ� ��Ʈ�� ��Ȯ���� Ȯ�� (127.0.0.1:7777)" << std::endl;
        std::cout << "? ��ȭ�� ���� Ȯ��" << std::endl;

        system("pause");
        return -1;
    }

    std::cout << "���� ���� ����! �޴��� �����ϼ���." << std::endl;

    try {
        // Ŭ���̾�Ʈ ����
        client.Run();
    }
    catch (const std::exception& e) {
        std::cerr << "? ���� �߻�: " << e.what() << std::endl;
    }

    std::cout << "\n?? ���� Ŭ���̾�Ʈ�� �̿��� �ּż� �����մϴ�!" << std::endl;
    std::cout << "���α׷��� �����մϴ�..." << std::endl;
    system("pause");
    return 0;
}