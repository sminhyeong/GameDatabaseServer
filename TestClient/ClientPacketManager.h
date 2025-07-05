#pragma once

#include <vector>
#include <string>
#include <cstdint>

// 전방 선언 (중복 방지)
struct DatabasePacket;
struct S2C_Login;
struct S2C_Logout;
struct S2C_CreateAccount;
struct S2C_ItemData;
struct S2C_PlayerData;
struct S2C_MonsterData;
struct S2C_PlayerChat;
struct S2C_ShopList;
struct S2C_ShopItems;
struct S2C_ShopTransaction;

// 게임 서버 관련 구조체 추가
struct S2C_CreateGameServer;
struct S2C_GameServerList;
struct S2C_JoinGameServer;
struct S2C_CloseGameServer;
struct S2C_SavePlayerData;
struct GameServerData;

enum EventType : uint8_t;
enum ResultCode : int8_t;

// 클라이언트용 패킷 매니저 클래스
class ClientPacketManager
{
private:
    std::string _last_error;

    void SetError(const std::string& error);
    void ClearError();
    bool VerifyPacket(const uint8_t* data, size_t size);

public:
    ClientPacketManager();
    ~ClientPacketManager();

    // === 기존 클라이언트 요청 패킷 생성 (C2S) ===

    // 로그인 요청
    std::vector<uint8_t> CreateLoginRequest(const std::string& username, const std::string& password);

    // 로그아웃 요청
    std::vector<uint8_t> CreateLogoutRequest(uint32_t user_id);

    // 계정 생성 요청
    std::vector<uint8_t> CreateAccountRequest(const std::string& username, const std::string& password, const std::string& nickname);

    // 아이템 데이터 요청
    std::vector<uint8_t> CreateItemDataRequest(uint32_t user_id, uint32_t request_type, uint32_t item_id = 0, uint32_t item_count = 0);

    // 플레이어 데이터 요청 (조회용)
    std::vector<uint8_t> CreatePlayerDataQueryRequest(uint32_t user_id);

    // 플레이어 데이터 요청 (업데이트용)
    std::vector<uint8_t> CreatePlayerDataUpdateRequest(uint32_t user_id, uint32_t level, uint32_t exp,
        uint32_t hp, uint32_t mp, float pos_x, float pos_y);

    // 몬스터 데이터 요청
    std::vector<uint8_t> CreateMonsterDataRequest(uint32_t request_type, uint32_t monster_id = 0);

    // 채팅 메시지 전송 요청
    std::vector<uint8_t> CreateChatSendRequest(uint32_t sender_id, uint32_t receiver_id, const std::string& message, uint32_t chat_type);

    // 채팅 조회 요청
    std::vector<uint8_t> CreateChatQueryRequest(uint32_t request_type, uint32_t sender_id, uint32_t receiver_id, uint32_t chat_type);

    // 상점 목록 요청
    std::vector<uint8_t> CreateShopListRequest(uint32_t request_type = 0, uint32_t map_id = 0);

    // 상점 아이템 목록 요청
    std::vector<uint8_t> CreateShopItemsRequest(uint32_t shop_id);

    // 상점 거래 요청 (구매/판매)
    std::vector<uint8_t> CreateShopTransactionRequest(uint32_t user_id, uint32_t shop_id, uint32_t item_id, uint32_t item_count, uint32_t transaction_type);

    // === 게임 서버 관련 클라이언트 요청 패킷 생성 (C2S) 추가 ===

    // 게임 서버 생성 요청
    std::vector<uint8_t> CreateGameServerRequest(uint32_t user_id, const std::string& server_name,
        const std::string& server_password, const std::string& server_ip, uint32_t server_port, uint32_t max_players = 10);

    // 게임 서버 목록 요청
    std::vector<uint8_t> CreateGameServerListRequest(uint32_t request_type = 0);

    // 게임 서버 접속 요청
    std::vector<uint8_t> CreateJoinGameServerRequest(uint32_t user_id, uint32_t server_id, const std::string& server_password = "");

    // 게임 서버 종료 요청 (서버 관리자만)
    std::vector<uint8_t> CreateCloseGameServerRequest(uint32_t user_id, uint32_t server_id);

    // 플레이어 데이터 저장 요청 (월드 퇴장시)
    std::vector<uint8_t> CreateSavePlayerDataRequest(uint32_t user_id, uint32_t level, uint32_t exp,
        uint32_t hp, uint32_t mp, uint32_t gold, float pos_x, float pos_y);

    // === 기존 서버 응답 패킷 파싱 (S2C) ===
    // FlatBuffers 구조체 포인터를 직접 반환

    // 로그인 응답 파싱
    const S2C_Login* ParseLoginResponse(const uint8_t* data, size_t size);

    // 로그아웃 응답 파싱
    const S2C_Logout* ParseLogoutResponse(const uint8_t* data, size_t size);

    // 계정 생성 응답 파싱
    const S2C_CreateAccount* ParseCreateAccountResponse(const uint8_t* data, size_t size);

    // 아이템 데이터 응답 파싱
    const S2C_ItemData* ParseItemDataResponse(const uint8_t* data, size_t size);

    // 플레이어 데이터 응답 파싱
    const S2C_PlayerData* ParsePlayerDataResponse(const uint8_t* data, size_t size);

    // 몬스터 데이터 응답 파싱
    const S2C_MonsterData* ParseMonsterDataResponse(const uint8_t* data, size_t size);

    // 채팅 응답 파싱
    const S2C_PlayerChat* ParsePlayerChatResponse(const uint8_t* data, size_t size);

    // 상점 목록 응답 파싱
    const S2C_ShopList* ParseShopListResponse(const uint8_t* data, size_t size);

    // 상점 아이템 응답 파싱
    const S2C_ShopItems* ParseShopItemsResponse(const uint8_t* data, size_t size);

    // 상점 거래 응답 파싱
    const S2C_ShopTransaction* ParseShopTransactionResponse(const uint8_t* data, size_t size);

    // === 게임 서버 관련 서버 응답 패킷 파싱 (S2C) 추가 ===

    // 게임 서버 생성 응답 파싱
    const S2C_CreateGameServer* ParseCreateGameServerResponse(const uint8_t* data, size_t size);

    // 게임 서버 목록 응답 파싱
    const S2C_GameServerList* ParseGameServerListResponse(const uint8_t* data, size_t size);

    // 게임 서버 접속 응답 파싱
    const S2C_JoinGameServer* ParseJoinGameServerResponse(const uint8_t* data, size_t size);

    // 게임 서버 종료 응답 파싱
    const S2C_CloseGameServer* ParseCloseGameServerResponse(const uint8_t* data, size_t size);

    // 플레이어 데이터 저장 응답 파싱
    const S2C_SavePlayerData* ParseSavePlayerDataResponse(const uint8_t* data, size_t size);

    // === 편의 함수들 ===

    // 로그인 결과 확인
    bool IsLoginSuccess(const S2C_Login* response);

    // 계정 생성 결과 확인
    bool IsCreateAccountSuccess(const S2C_CreateAccount* response);

    // 플레이어 데이터 조회 성공 확인
    bool IsPlayerDataValid(const S2C_PlayerData* response);

    // 아이템 데이터 조회 성공 확인
    bool IsItemDataValid(const S2C_ItemData* response);

    // 상점 목록 조회 성공 확인
    bool IsShopListValid(const S2C_ShopList* response);

    // 상점 아이템 조회 성공 확인
    bool IsShopItemsValid(const S2C_ShopItems* response);

    // 상점 거래 성공 확인
    bool IsShopTransactionSuccess(const S2C_ShopTransaction* response);

    // === 게임 서버 관련 편의 함수들 추가 ===

    // 게임 서버 생성 성공 확인
    bool IsCreateGameServerSuccess(const S2C_CreateGameServer* response);

    // 게임 서버 목록 조회 성공 확인
    bool IsGameServerListValid(const S2C_GameServerList* response);

    // 게임 서버 접속 성공 확인
    bool IsJoinGameServerSuccess(const S2C_JoinGameServer* response);

    // 게임 서버 종료 성공 확인
    bool IsCloseGameServerSuccess(const S2C_CloseGameServer* response);

    // 플레이어 데이터 저장 성공 확인
    bool IsSavePlayerDataSuccess(const S2C_SavePlayerData* response);

    // === 유틸리티 함수들 ===

    // 패킷 타입 확인 (EventType enum 반환)
    EventType GetPacketType(const uint8_t* data, size_t size);

    // 클라이언트 소켓 정보 확인
    uint32_t GetClientSocket(const uint8_t* data, size_t size);

    // 패킷 검증
    bool IsValidPacket(const uint8_t* data, size_t size);

    // 패킷 타입을 문자열로 변환 (디버깅용)
    std::string GetPacketTypeName(EventType packet_type);

    // 결과 코드를 문자열로 변환 (디버깅용)
    std::string GetResultCodeName(ResultCode result);

    // 에러 메시지 반환
    std::string GetLastError() const { return _last_error; }
};