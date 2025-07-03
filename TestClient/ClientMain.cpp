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
            std::cerr << "소켓 생성 실패" << std::endl;
            return false;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr(ip);
        serverAddr.sin_port = htons(port);

        if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "서버 연결 실패: " << WSAGetLastError() << std::endl;
            closesocket(sock);
            sock = INVALID_SOCKET;
            return false;
        }

        isConnected = true;
        std::cout << "서버 연결 성공! (" << ip << ":" << port << ")" << std::endl;
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

        // 패킷 크기 전송 (4바이트 헤더)
        uint32_t size = htonl(static_cast<uint32_t>(data.size()));
        if (send(sock, (char*)&size, sizeof(size), 0) <= 0) {
            std::cerr << "헤더 전송 실패" << std::endl;
            return false;
        }

        // 실제 데이터 전송
        int totalSent = 0;
        while (totalSent < data.size()) {
            int sent = send(sock, (char*)data.data() + totalSent,
                data.size() - totalSent, 0);
            if (sent <= 0) {
                std::cerr << "데이터 전송 실패" << std::endl;
                return false;
            }
            totalSent += sent;
        }

        std::cout << "[SEND] 패킷 전송 완료: " << data.size() << " bytes" << std::endl;
        return true;
    }

    std::vector<uint8_t> ReceivePacket() {
        if (!isConnected) {
            return std::vector<uint8_t>();
        }

        // 패킷 크기 수신 (4바이트 헤더)
        uint32_t packetSize = 0;
        int headerReceived = recv(sock, (char*)&packetSize, sizeof(packetSize), 0);

        if (headerReceived <= 0) {
            std::cerr << "헤더 수신 실패" << std::endl;
            return std::vector<uint8_t>();
        }

        packetSize = ntohl(packetSize);
        if (packetSize == 0 || packetSize > 65536) {
            std::cerr << "잘못된 패킷 크기: " << packetSize << std::endl;
            return std::vector<uint8_t>();
        }

        // 실제 패킷 데이터 수신
        std::vector<uint8_t> packetData(packetSize);
        int totalReceived = 0;

        while (totalReceived < packetSize) {
            int received = recv(sock, (char*)packetData.data() + totalReceived,
                packetSize - totalReceived, 0);
            if (received <= 0) {
                std::cerr << "패킷 데이터 수신 실패" << std::endl;
                return std::vector<uint8_t>();
            }
            totalReceived += received;
        }

        std::cout << "[RECV] 패킷 수신 완료: " << packetSize << " bytes" << std::endl;
        return packetData;
    }

    bool Login(const std::string& username, const std::string& password) {
        std::cout << "\n=== 로그인 시도 ===" << std::endl;
        std::cout << "사용자명: " << username << std::endl;

        // 1. 로그인 요청 패킷 생성
        auto loginPacket = packetManager.CreateLoginRequest(username, password);
        if (loginPacket.empty()) {
            std::cerr << "로그인 패킷 생성 실패: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 2. 패킷 전송
        if (!SendPacket(loginPacket)) {
            return false;
        }

        // 3. 응답 수신
        auto responseData = ReceivePacket();
        if (responseData.empty()) {
            return false;
        }

        // 4. 응답 파싱
        const S2C_Login* loginResponse = packetManager.ParseLoginResponse(responseData.data(), responseData.size());
        if (!loginResponse) {
            std::cerr << "로그인 응답 파싱 실패: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 5. 결과 확인
        if (packetManager.IsLoginSuccess(loginResponse)) {
            currentUserId = loginResponse->user_id();
            currentUsername = loginResponse->username()->str();
            std::cout << "? 로그인 성공!" << std::endl;
            std::cout << "   사용자 ID: " << currentUserId << std::endl;
            std::cout << "   사용자명: " << currentUsername << std::endl;
            std::cout << "   레벨: " << loginResponse->level() << std::endl;
            return true;
        }
        else {
            std::cout << "? 로그인 실패: " << packetManager.GetResultCodeName(loginResponse->result()) << std::endl;
            return false;
        }
    }

    bool Logout() {
        if (currentUserId == 0) {
            std::cerr << "? 로그인 상태가 아닙니다!" << std::endl;
            return false;
        }

        std::cout << "\n=== 로그아웃 시도 ===" << std::endl;
        std::cout << "사용자: " << currentUsername << " (ID: " << currentUserId << ")" << std::endl;

        // 1. 로그아웃 요청 패킷 생성
        auto logoutPacket = packetManager.CreateLogoutRequest(currentUserId);
        if (logoutPacket.empty()) {
            std::cerr << "? 로그아웃 패킷 생성 실패: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 2. 패킷 전송
        if (!SendPacket(logoutPacket)) {
            return false;
        }

        // 3. 응답 수신
        auto responseData = ReceivePacket();
        if (responseData.empty()) {
            return false;
        }

        // 4. 응답 파싱
        const S2C_Logout* logoutResponse = packetManager.ParseLogoutResponse(responseData.data(), responseData.size());
        if (!logoutResponse) {
            std::cerr << "? 로그아웃 응답 파싱 실패: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 5. 결과 확인
        if (logoutResponse->result() == ResultCode_SUCCESS) {
            std::cout << "? 로그아웃 성공!" << std::endl;
            if (logoutResponse->message()) {
                std::cout << "   메시지: " << logoutResponse->message()->c_str() << std::endl;
            }

            // 로컬 상태 초기화
            currentUserId = 0;
            currentUsername.clear();
            return true;
        }
        else {
            std::cout << "? 로그아웃 실패: " << packetManager.GetResultCodeName(logoutResponse->result()) << std::endl;
            return false;
        }
    }

    bool CreateAccount(const std::string& username, const std::string& password, const std::string& nickname) {
        std::cout << "\n=== 계정 생성 시도 ===" << std::endl;
        std::cout << "사용자명: " << username << ", 닉네임: " << nickname << std::endl;

        // 1. 계정 생성 요청 패킷 생성
        auto accountPacket = packetManager.CreateAccountRequest(username, password, nickname);
        if (accountPacket.empty()) {
            std::cerr << "계정 생성 패킷 생성 실패: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 2. 패킷 전송
        if (!SendPacket(accountPacket)) {
            return false;
        }

        // 3. 응답 수신
        auto responseData = ReceivePacket();
        if (responseData.empty()) {
            return false;
        }

        // 4. 응답 파싱
        const S2C_CreateAccount* accountResponse = packetManager.ParseCreateAccountResponse(responseData.data(), responseData.size());
        if (!accountResponse) {
            std::cerr << "계정 생성 응답 파싱 실패: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 5. 결과 확인
        if (packetManager.IsCreateAccountSuccess(accountResponse)) {
            std::cout << "? 계정 생성 성공!" << std::endl;
            std::cout << "   새 사용자 ID: " << accountResponse->user_id() << std::endl;
            std::cout << "   메시지: " << accountResponse->message()->c_str() << std::endl;
            return true;
        }
        else {
            std::cout << "? 계정 생성 실패: " << accountResponse->message()->c_str() << std::endl;
            return false;
        }
    }

    bool GetItemData() {
        if (currentUserId == 0) {
            std::cerr << "? 로그인이 필요합니다!" << std::endl;
            return false;
        }

        std::cout << "\n=== 전체 아이템 데이터 조회 ===" << std::endl;
        std::cout << "사용자 ID: " << currentUserId << std::endl;

        // 1. 아이템 데이터 요청 패킷 생성 (request_type = 0: 조회)
        auto itemPacket = packetManager.CreateItemDataRequest(currentUserId, 0, 0, 0);
        if (itemPacket.empty()) {
            std::cerr << "아이템 데이터 패킷 생성 실패: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 2. 패킷 전송
        if (!SendPacket(itemPacket)) {
            return false;
        }

        // 3. 응답 수신
        auto responseData = ReceivePacket();
        if (responseData.empty()) {
            return false;
        }

        // 4. 응답 파싱
        const S2C_ItemData* itemResponse = packetManager.ParseItemDataResponse(responseData.data(), responseData.size());
        if (!itemResponse) {
            std::cerr << "아이템 데이터 응답 파싱 실패: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 5. 결과 확인 및 출력
        if (packetManager.IsItemDataValid(itemResponse)) {
            std::cout << "? 아이템 데이터 조회 성공!" << std::endl;
            std::cout << "?? 보유 골드: " << itemResponse->gold() << std::endl;

            if (itemResponse->items() && itemResponse->items()->size() > 0) {
                std::cout << "?? 보유 아이템 목록:" << std::endl;
                std::cout << "┌──────┬────────────────┬──────┬──────────────┐" << std::endl;
                std::cout << "│ ID   │ 아이템명       │ 수량 │ 타입         │" << std::endl;
                std::cout << "├──────┼────────────────┼──────┼──────────────┤" << std::endl;

                for (size_t i = 0; i < itemResponse->items()->size(); ++i) {
                    const ItemData* item = itemResponse->items()->Get(i);
                    if (item) {
                        std::string typeStr;
                        switch (item->item_type()) {
                        case 0: typeStr = "무기"; break;
                        case 1: typeStr = "방어구"; break;
                        case 2: typeStr = "소비품"; break;
                        default: typeStr = "기타"; break;
                        }

                        printf("│ %-4d │ %-14s │ %-4d │ %-12s │\n",
                            item->item_id(),
                            item->item_name()->c_str(),
                            item->item_count(),
                            typeStr.c_str());
                    }
                }
                std::cout << "└──────┴────────────────┴──────┴──────────────┘" << std::endl;

                // 첫 번째 아이템 상세 정보 출력
                const ItemData* firstItem = itemResponse->items()->Get(0);
                if (firstItem) {
                    std::cout << "\n?? [첫 번째 아이템 상세 정보]" << std::endl;
                    std::cout << "   ?? 아이템 ID: " << firstItem->item_id() << std::endl;
                    std::cout << "   ?? 아이템명: " << firstItem->item_name()->c_str() << std::endl;
                    std::cout << "   ?? 수량: " << firstItem->item_count() << std::endl;
                    std::cout << "   ???  타입: " << firstItem->item_type() << " (";
                    switch (firstItem->item_type()) {
                    case 0: std::cout << "무기"; break;
                    case 1: std::cout << "방어구"; break;
                    case 2: std::cout << "소비품"; break;
                    default: std::cout << "기타"; break;
                    }
                    std::cout << ")" << std::endl;
                }
            }
            else {
                std::cout << "?? 보유한 아이템이 없습니다." << std::endl;
            }
            return true;
        }
        else {
            std::cout << "? 아이템 데이터 조회 실패: " << packetManager.GetResultCodeName(itemResponse->result()) << std::endl;
            return false;
        }
    }

    bool GetSpecificItemInfo() {
        if (currentUserId == 0) {
            std::cerr << "? 로그인이 필요합니다!" << std::endl;
            return false;
        }

        uint32_t itemId;
        std::cout << "\n=== 특정 아이템 정보 조회 ===" << std::endl;
        std::cout << "조회할 아이템 ID를 입력하세요: ";
        std::cin >> itemId;

        if (itemId == 0) {
            std::cerr << "? 잘못된 아이템 ID입니다." << std::endl;
            return false;
        }

        std::cout << "아이템 ID " << itemId << " 정보를 조회합니다..." << std::endl;

        // 1. 전체 아이템 목록을 조회한 후 특정 아이템 찾기
        auto itemPacket = packetManager.CreateItemDataRequest(currentUserId, 0, 0, 0);
        if (itemPacket.empty()) {
            std::cerr << "? 아이템 데이터 패킷 생성 실패: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 2. 패킷 전송
        if (!SendPacket(itemPacket)) {
            return false;
        }

        // 3. 응답 수신
        auto responseData = ReceivePacket();
        if (responseData.empty()) {
            return false;
        }

        // 4. 응답 파싱
        const S2C_ItemData* itemResponse = packetManager.ParseItemDataResponse(responseData.data(), responseData.size());
        if (!itemResponse) {
            std::cerr << "? 아이템 데이터 응답 파싱 실패: " << packetManager.GetLastError() << std::endl;
            return false;
        }

        // 5. 특정 아이템 찾기
        if (packetManager.IsItemDataValid(itemResponse)) {
            if (itemResponse->items() && itemResponse->items()->size() > 0) {
                const ItemData* targetItem = nullptr;

                // 입력한 ID와 일치하는 아이템 찾기
                for (size_t i = 0; i < itemResponse->items()->size(); ++i) {
                    const ItemData* item = itemResponse->items()->Get(i);
                    if (item && item->item_id() == itemId) {
                        targetItem = item;
                        break;
                    }
                }

                if (targetItem) {
                    std::cout << "? 아이템을 찾았습니다!" << std::endl;
                    std::cout << "┌─────────────────────────────────────┐" << std::endl;
                    std::cout << "│           아이템 상세 정보          │" << std::endl;
                    std::cout << "├─────────────────────────────────────┤" << std::endl;
                    printf("│ ?? 아이템 ID: %-21d │\n", targetItem->item_id());
                    printf("│ ?? 아이템명: %-23s │\n", targetItem->item_name()->c_str());
                    printf("│ ?? 보유 수량: %-22d │\n", targetItem->item_count());
                    printf("│ ???  아이템 타입: %-19d │\n", targetItem->item_type());
                    std::cout << "│ ???  타입 설명: ";
                    switch (targetItem->item_type()) {
                    case 0: std::cout << "무기                     │" << std::endl; break;
                    case 1: std::cout << "방어구                   │" << std::endl; break;
                    case 2: std::cout << "소비품                   │" << std::endl; break;
                    default: std::cout << "기타                     │" << std::endl; break;
                    }
                    std::cout << "└─────────────────────────────────────┘" << std::endl;

                    // 아이템 사용 가능 여부 등 추가 정보
                    std::cout << "\n?? [추가 정보]" << std::endl;
                    std::cout << "   ?? 현재 보유 골드: " << itemResponse->gold() << std::endl;

                    if (targetItem->item_count() > 0) {
                        std::cout << "   ? 사용 가능한 아이템입니다." << std::endl;
                    }
                    else {
                        std::cout << "   ? 보유하지 않은 아이템입니다." << std::endl;
                    }

                    return true;
                }
                else {
                    std::cout << "? 해당 ID(" << itemId << ")의 아이템을 보유하고 있지 않습니다." << std::endl;

                    // 보유 중인 아이템 ID 목록 표시
                    std::cout << "\n?? 현재 보유 중인 아이템 ID 목록:" << std::endl;
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
                std::cout << "? 보유한 아이템이 없습니다." << std::endl;
                return false;
            }
        }
        else {
            std::cout << "? 아이템 데이터 조회 실패: " << packetManager.GetResultCodeName(itemResponse->result()) << std::endl;
            return false;
        }
    }

    void ShowCurrentStatus() {
        std::cout << "\n┌─────────────────────────────────────┐" << std::endl;
        std::cout << "│             현재 상태               │" << std::endl;
        std::cout << "├─────────────────────────────────────┤" << std::endl;
        if (currentUserId > 0) {
            std::cout << "│ ?? 로그인 상태: 접속 중            │" << std::endl;
            printf("│ ?? 사용자명: %-22s │\n", currentUsername.c_str());
            printf("│ ?? 사용자 ID: %-21d │\n", currentUserId);
        }
        else {
            std::cout << "│ ?? 로그인 상태: 로그아웃           │" << std::endl;
            std::cout << "│ ?? 사용자명: 없음                  │" << std::endl;
            std::cout << "│ ?? 사용자 ID: 없음                 │" << std::endl;
        }
        std::cout << "│ ?? 서버 연결: " << (isConnected ? "연결됨             │" : "연결 안됨         │") << std::endl;
        std::cout << "└─────────────────────────────────────┘" << std::endl;
    }

    void ShowMenu() {
        std::cout << "\n┌─────────────────────────────────────┐" << std::endl;
        std::cout << "│           게임 클라이언트           │" << std::endl;
        std::cout << "├─────────────────────────────────────┤" << std::endl;
        std::cout << "│ 1. 로그인                           │" << std::endl;
        std::cout << "│ 2. 로그아웃                         │" << std::endl;
        std::cout << "│ 3. 계정 생성                        │" << std::endl;
        std::cout << "│ 4. 전체 아이템 데이터 조회          │" << std::endl;
        std::cout << "│ 5. 특정 아이템 정보 조회            │" << std::endl;
        std::cout << "│ 6. 현재 상태 확인                   │" << std::endl;
        std::cout << "│ 0. 연결 종료                        │" << std::endl;
        std::cout << "└─────────────────────────────────────┘" << std::endl;
        std::cout << "선택 >> ";
    }

    void Run() {
        int choice;
        std::string username, password, nickname;

        std::cout << "\n?? 게임 클라이언트에 오신 것을 환영합니다!" << std::endl;

        while (isConnected) {
            ShowMenu();

            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "? 잘못된 입력입니다. 숫자를 입력하세요." << std::endl;
                continue;
            }

            switch (choice) {
            case 1: // 로그인
                if (currentUserId > 0) {
                    std::cout << "??  이미 로그인된 상태입니다. (사용자: " << currentUsername << ")" << std::endl;
                }
                else {
                    std::cout << "\n?? 로그인 정보를 입력하세요." << std::endl;
                    std::cout << "사용자명: ";
                    std::cin >> username;
                    std::cout << "비밀번호: ";
                    std::cin >> password;
                    Login(username, password);
                }
                break;

            case 2: // 로그아웃
                if (currentUserId == 0) {
                    std::cout << "??  로그인된 상태가 아닙니다." << std::endl;
                }
                else {
                    Logout();
                }
                break;

            case 3: // 계정 생성
                std::cout << "\n?? 계정 생성 정보를 입력하세요." << std::endl;
                std::cout << "사용자명: ";
                std::cin >> username;
                std::cout << "비밀번호: ";
                std::cin >> password;
                std::cout << "닉네임: ";
                std::cin >> nickname;
                CreateAccount(username, password, nickname);
                break;

            case 4: // 전체 아이템 데이터 조회
                GetItemData();
                break;

            case 5: // 특정 아이템 정보 조회
                GetSpecificItemInfo();
                break;

            case 6: // 현재 상태 확인
                ShowCurrentStatus();
                break;

            case 0: // 연결 종료
                if (currentUserId > 0) {
                    std::cout << "\n?? 로그아웃 후 종료합니다..." << std::endl;
                    Logout();
                }
                std::cout << "?? 클라이언트를 종료합니다." << std::endl;
                return;

            default:
                std::cout << "? 잘못된 선택입니다. 0-6 사이의 숫자를 입력하세요." << std::endl;
                break;
            }

            // 결과 확인을 위한 잠시 대기
            std::cout << "\n계속하려면 Enter 키를 누르세요...";
            std::cin.ignore();
            std::cin.get();
        }
    }
};

int main() {
    // 콘솔 창 제목 설정
    SetConsoleTitle(L"게임 클라이언트 - Enhanced Version");

    std::cout << "??????????????????????????????????????????????????????????" << std::endl;
    std::cout << "?                                                        ?" << std::endl;
    std::cout << "?            ?? 게임 서버 테스트 클라이언트 ??            ?" << std::endl;
    std::cout << "?                                                        ?" << std::endl;
    std::cout << "?              Enhanced with Logout & Item Search       ?" << std::endl;
    std::cout << "?                                                        ?" << std::endl;
    std::cout << "??????????????????????????????????????????????????????????" << std::endl;

    GameClient client;

    // 서버에 연결
    std::cout << "\n?? 서버에 연결 중..." << std::endl;
    if (!client.Connect("127.0.0.1", 7777)) {
        std::cerr << "? 서버 연결 실패!" << std::endl;
        std::cout << "\n다음 사항을 확인해주세요:" << std::endl;
        std::cout << "? 서버가 실행 중인지 확인" << std::endl;
        std::cout << "? IP 주소와 포트가 정확한지 확인 (127.0.0.1:7777)" << std::endl;
        std::cout << "? 방화벽 설정 확인" << std::endl;

        system("pause");
        return -1;
    }

    std::cout << "서버 연결 성공! 메뉴를 선택하세요." << std::endl;

    try {
        // 클라이언트 실행
        client.Run();
    }
    catch (const std::exception& e) {
        std::cerr << "? 예외 발생: " << e.what() << std::endl;
    }

    std::cout << "\n?? 게임 클라이언트를 이용해 주셔서 감사합니다!" << std::endl;
    std::cout << "프로그램을 종료합니다..." << std::endl;
    system("pause");
    return 0;
}