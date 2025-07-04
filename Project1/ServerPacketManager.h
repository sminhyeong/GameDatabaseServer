#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <mysql.h>

// 전방 선언
struct DatabasePacket;
struct C2S_Login;
struct C2S_Logout;
struct C2S_CreateAccount;
struct C2S_ItemData;
struct C2S_PlayerData;
struct C2S_MonsterData;
struct C2S_PlayerChat;
struct C2S_ShopList;
struct C2S_ShopItems;
struct C2S_ShopTransaction;

enum EventType : uint8_t;
enum ResultCode : int8_t;

// 서버용 패킷 매니저 클래스
class ServerPacketManager
{
private:
    std::string _last_error;

    void SetError(const std::string& error);
    void ClearError();
    bool VerifyPacket(const uint8_t* data, size_t size);

public:
    ServerPacketManager();
    ~ServerPacketManager();

    // === 클라이언트 요청 패킷 파싱 (C2S) ===
    // FlatBuffers 구조체 포인터를 직접 반환

    // 로그인 요청 파싱
    const C2S_Login* ParseLoginRequest(const uint8_t* data, size_t size);

    // 로그아웃 요청 파싱
    const C2S_Logout* ParseLogoutRequest(const uint8_t* data, size_t size);

    // 계정 생성 요청 파싱
    const C2S_CreateAccount* ParseCreateAccountRequest(const uint8_t* data, size_t size);

    // 아이템 데이터 요청 파싱
    const C2S_ItemData* ParseItemDataRequest(const uint8_t* data, size_t size);

    // 플레이어 데이터 요청 파싱
    const C2S_PlayerData* ParsePlayerDataRequest(const uint8_t* data, size_t size);

    // 몬스터 데이터 요청 파싱
    const C2S_MonsterData* ParseMonsterDataRequest(const uint8_t* data, size_t size);

    // 채팅 요청 파싱
    const C2S_PlayerChat* ParsePlayerChatRequest(const uint8_t* data, size_t size);

    // 상점 목록 요청 파싱
    const C2S_ShopList* ParseShopListRequest(const uint8_t* data, size_t size);

    // 상점 아이템 요청 파싱
    const C2S_ShopItems* ParseShopItemsRequest(const uint8_t* data, size_t size);

    // 상점 거래 요청 파싱
    const C2S_ShopTransaction* ParseShopTransactionRequest(const uint8_t* data, size_t size);

    // === 서버 응답 패킷 생성 (S2C) ===

    // 로그인 응답 생성
    std::vector<uint8_t> CreateLoginResponse(ResultCode result, uint32_t user_id, const std::string& username, const std::string& nickname, uint32_t level, uint32_t client_socket = 0);

    // 로그아웃 응답 생성
    std::vector<uint8_t> CreateLogoutResponse(ResultCode result, const std::string& message, uint32_t client_socket = 0);

    // 계정 생성 응답 생성
    std::vector<uint8_t> CreateAccountResponse(ResultCode result, uint32_t user_id, const std::string& message, uint32_t client_socket = 0);

    // 아이템 데이터 응답 생성 (개별 하드코딩)
    std::vector<uint8_t> CreateItemDataResponse(ResultCode result, uint32_t user_id, uint32_t gold, uint32_t client_socket = 0);

    // 플레이어 데이터 응답 생성
    std::vector<uint8_t> CreatePlayerDataResponse(ResultCode result, uint32_t user_id, const std::string& username, const std::string& nickname,
        uint32_t level, uint32_t exp, uint32_t hp, uint32_t mp, uint32_t attack,
        uint32_t defense, uint32_t gold, uint32_t map_id, float pos_x, float pos_y, uint32_t client_socket = 0);

    // 몬스터 데이터 응답 생성
    std::vector<uint8_t> CreateMonsterDataResponse(ResultCode result, uint32_t client_socket = 0);

    // 채팅 응답 생성
    std::vector<uint8_t> CreatePlayerChatResponse(ResultCode result, uint32_t client_socket = 0);

    // 상점 목록 응답 생성
    std::vector<uint8_t> CreateShopListResponse(ResultCode result, uint32_t client_socket = 0);

    // 상점 아이템 응답 생성
    std::vector<uint8_t> CreateShopItemsResponse(ResultCode result, uint32_t shop_id, uint32_t client_socket = 0);

    // 상점 거래 응답 생성
    std::vector<uint8_t> CreateShopTransactionResponse(ResultCode result, const std::string& message, uint32_t updated_gold, uint32_t client_socket = 0);

    // === MySQL 결과에서 직접 응답 패킷 생성 (서버 전용) ===

    // MySQL 로그인 결과로 응답 패킷 생성
    std::vector<uint8_t> CreateLoginResponseFromDB(MYSQL_RES* result, const std::string& username, uint32_t client_socket = 0);

    // MySQL 플레이어 데이터 결과로 응답 패킷 생성
    std::vector<uint8_t> CreatePlayerDataResponseFromDB(MYSQL_RES* result, uint32_t user_id, uint32_t client_socket = 0);

    // MySQL 아이템 데이터 결과로 응답 패킷 생성 (아이템 리스트 포함)
    std::vector<uint8_t> CreateItemDataResponseFromDB(MYSQL_RES* result, uint32_t user_id, uint32_t client_socket = 0);

    // MySQL 몬스터 데이터 결과로 응답 패킷 생성
    std::vector<uint8_t> CreateMonsterDataResponseFromDB(MYSQL_RES* result, uint32_t client_socket = 0);

    // MySQL 채팅 데이터 결과로 응답 패킷 생성
    std::vector<uint8_t> CreatePlayerChatResponseFromDB(MYSQL_RES* result, uint32_t client_socket = 0);

    // MySQL 상점 목록 결과로 응답 패킷 생성
    std::vector<uint8_t> CreateShopListResponseFromDB(MYSQL_RES* result, uint32_t client_socket = 0);

    // MySQL 상점 아이템 결과로 응답 패킷 생성
    std::vector<uint8_t> CreateShopItemsResponseFromDB(MYSQL_RES* result, uint32_t shop_id, uint32_t client_socket = 0);

    // === 간편한 에러 응답 생성 ===

    // 로그인 실패 응답
    std::vector<uint8_t> CreateLoginErrorResponse(ResultCode error_code, uint32_t client_socket = 0);

    // 계정 생성 실패 응답
    std::vector<uint8_t> CreateAccountErrorResponse(ResultCode error_code, const std::string& message, uint32_t client_socket = 0);

    // 플레이어 데이터 실패 응답
    std::vector<uint8_t> CreatePlayerDataErrorResponse(ResultCode error_code, uint32_t client_socket = 0);

    // 아이템 데이터 실패 응답
    std::vector<uint8_t> CreateItemDataErrorResponse(ResultCode error_code, uint32_t user_id, uint32_t client_socket = 0);

    // 상점 목록 실패 응답
    std::vector<uint8_t> CreateShopListErrorResponse(ResultCode error_code, uint32_t client_socket = 0);

    // 상점 아이템 실패 응답
    std::vector<uint8_t> CreateShopItemsErrorResponse(ResultCode error_code, uint32_t shop_id, uint32_t client_socket = 0);

    // 상점 거래 실패 응답
    std::vector<uint8_t> CreateShopTransactionErrorResponse(ResultCode error_code, const std::string& message, uint32_t client_socket = 0);

    // 일반적인 에러 응답 생성
    std::vector<uint8_t> CreateGenericErrorResponse(EventType response_type, ResultCode error_code, uint32_t client_socket = 0);

    // === MySQL 헬퍼 함수들 ===

    // MySQL 결과에서 단일 행의 문자열 값 가져오기
    std::string GetStringFromRow(MYSQL_ROW row, int index);

    // MySQL 결과에서 단일 행의 정수 값 가져오기
    uint32_t GetUintFromRow(MYSQL_ROW row, int index);

    // MySQL 결과에서 단일 행의 실수 값 가져오기
    float GetFloatFromRow(MYSQL_ROW row, int index);

    // MySQL 결과 검증 (NULL 체크 등)
    bool IsValidMySQLResult(MYSQL_RES* result);

    // === 요청 검증 헬퍼 함수들 ===

    // 로그인 요청 유효성 검사
    bool ValidateLoginRequest(const C2S_Login* request);

    // 계정 생성 요청 유효성 검사
    bool ValidateCreateAccountRequest(const C2S_CreateAccount* request);

    // 플레이어 데이터 요청 유효성 검사
    bool ValidatePlayerDataRequest(const C2S_PlayerData* request);

    // 아이템 데이터 요청 유효성 검사
    bool ValidateItemDataRequest(const C2S_ItemData* request);

    // 상점 목록 요청 유효성 검사
    bool ValidateShopListRequest(const C2S_ShopList* request);

    // 상점 아이템 요청 유효성 검사
    bool ValidateShopItemsRequest(const C2S_ShopItems* request);

    // 상점 거래 요청 유효성 검사
    bool ValidateShopTransactionRequest(const C2S_ShopTransaction* request);

    // === 유틸리티 함수들 ===

    // 패킷 타입 확인 (EventType enum 반환)
    EventType GetPacketType(const uint8_t* data, size_t size);

    // 클라이언트 소켓 정보 확인
    uint32_t GetClientSocket(const uint8_t* data, size_t size);

    // 패킷 검증
    bool IsValidPacket(const uint8_t* data, size_t size);

    // 패킷 타입을 문자열로 반환 (디버깅용)
    std::string GetPacketTypeName(EventType packet_type);

    // 결과 코드를 문자열로 반환 (디버깅용)
    std::string GetResultCodeName(ResultCode result);

    // 에러 메시지 반환
    std::string GetLastError() const { return _last_error; }
};