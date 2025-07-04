#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <mysql.h>

// ���� ����
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

// ������ ��Ŷ �Ŵ��� Ŭ����
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

    // === Ŭ���̾�Ʈ ��û ��Ŷ �Ľ� (C2S) ===
    // FlatBuffers ����ü �����͸� ���� ��ȯ

    // �α��� ��û �Ľ�
    const C2S_Login* ParseLoginRequest(const uint8_t* data, size_t size);

    // �α׾ƿ� ��û �Ľ�
    const C2S_Logout* ParseLogoutRequest(const uint8_t* data, size_t size);

    // ���� ���� ��û �Ľ�
    const C2S_CreateAccount* ParseCreateAccountRequest(const uint8_t* data, size_t size);

    // ������ ������ ��û �Ľ�
    const C2S_ItemData* ParseItemDataRequest(const uint8_t* data, size_t size);

    // �÷��̾� ������ ��û �Ľ�
    const C2S_PlayerData* ParsePlayerDataRequest(const uint8_t* data, size_t size);

    // ���� ������ ��û �Ľ�
    const C2S_MonsterData* ParseMonsterDataRequest(const uint8_t* data, size_t size);

    // ä�� ��û �Ľ�
    const C2S_PlayerChat* ParsePlayerChatRequest(const uint8_t* data, size_t size);

    // ���� ��� ��û �Ľ�
    const C2S_ShopList* ParseShopListRequest(const uint8_t* data, size_t size);

    // ���� ������ ��û �Ľ�
    const C2S_ShopItems* ParseShopItemsRequest(const uint8_t* data, size_t size);

    // ���� �ŷ� ��û �Ľ�
    const C2S_ShopTransaction* ParseShopTransactionRequest(const uint8_t* data, size_t size);

    // === ���� ���� ��Ŷ ���� (S2C) ===

    // �α��� ���� ����
    std::vector<uint8_t> CreateLoginResponse(ResultCode result, uint32_t user_id, const std::string& username, const std::string& nickname, uint32_t level, uint32_t client_socket = 0);

    // �α׾ƿ� ���� ����
    std::vector<uint8_t> CreateLogoutResponse(ResultCode result, const std::string& message, uint32_t client_socket = 0);

    // ���� ���� ���� ����
    std::vector<uint8_t> CreateAccountResponse(ResultCode result, uint32_t user_id, const std::string& message, uint32_t client_socket = 0);

    // ������ ������ ���� ���� (���� �ϵ��ڵ�)
    std::vector<uint8_t> CreateItemDataResponse(ResultCode result, uint32_t user_id, uint32_t gold, uint32_t client_socket = 0);

    // �÷��̾� ������ ���� ����
    std::vector<uint8_t> CreatePlayerDataResponse(ResultCode result, uint32_t user_id, const std::string& username, const std::string& nickname,
        uint32_t level, uint32_t exp, uint32_t hp, uint32_t mp, uint32_t attack,
        uint32_t defense, uint32_t gold, uint32_t map_id, float pos_x, float pos_y, uint32_t client_socket = 0);

    // ���� ������ ���� ����
    std::vector<uint8_t> CreateMonsterDataResponse(ResultCode result, uint32_t client_socket = 0);

    // ä�� ���� ����
    std::vector<uint8_t> CreatePlayerChatResponse(ResultCode result, uint32_t client_socket = 0);

    // ���� ��� ���� ����
    std::vector<uint8_t> CreateShopListResponse(ResultCode result, uint32_t client_socket = 0);

    // ���� ������ ���� ����
    std::vector<uint8_t> CreateShopItemsResponse(ResultCode result, uint32_t shop_id, uint32_t client_socket = 0);

    // ���� �ŷ� ���� ����
    std::vector<uint8_t> CreateShopTransactionResponse(ResultCode result, const std::string& message, uint32_t updated_gold, uint32_t client_socket = 0);

    // === MySQL ������� ���� ���� ��Ŷ ���� (���� ����) ===

    // MySQL �α��� ����� ���� ��Ŷ ����
    std::vector<uint8_t> CreateLoginResponseFromDB(MYSQL_RES* result, const std::string& username, uint32_t client_socket = 0);

    // MySQL �÷��̾� ������ ����� ���� ��Ŷ ����
    std::vector<uint8_t> CreatePlayerDataResponseFromDB(MYSQL_RES* result, uint32_t user_id, uint32_t client_socket = 0);

    // MySQL ������ ������ ����� ���� ��Ŷ ���� (������ ����Ʈ ����)
    std::vector<uint8_t> CreateItemDataResponseFromDB(MYSQL_RES* result, uint32_t user_id, uint32_t client_socket = 0);

    // MySQL ���� ������ ����� ���� ��Ŷ ����
    std::vector<uint8_t> CreateMonsterDataResponseFromDB(MYSQL_RES* result, uint32_t client_socket = 0);

    // MySQL ä�� ������ ����� ���� ��Ŷ ����
    std::vector<uint8_t> CreatePlayerChatResponseFromDB(MYSQL_RES* result, uint32_t client_socket = 0);

    // MySQL ���� ��� ����� ���� ��Ŷ ����
    std::vector<uint8_t> CreateShopListResponseFromDB(MYSQL_RES* result, uint32_t client_socket = 0);

    // MySQL ���� ������ ����� ���� ��Ŷ ����
    std::vector<uint8_t> CreateShopItemsResponseFromDB(MYSQL_RES* result, uint32_t shop_id, uint32_t client_socket = 0);

    // === ������ ���� ���� ���� ===

    // �α��� ���� ����
    std::vector<uint8_t> CreateLoginErrorResponse(ResultCode error_code, uint32_t client_socket = 0);

    // ���� ���� ���� ����
    std::vector<uint8_t> CreateAccountErrorResponse(ResultCode error_code, const std::string& message, uint32_t client_socket = 0);

    // �÷��̾� ������ ���� ����
    std::vector<uint8_t> CreatePlayerDataErrorResponse(ResultCode error_code, uint32_t client_socket = 0);

    // ������ ������ ���� ����
    std::vector<uint8_t> CreateItemDataErrorResponse(ResultCode error_code, uint32_t user_id, uint32_t client_socket = 0);

    // ���� ��� ���� ����
    std::vector<uint8_t> CreateShopListErrorResponse(ResultCode error_code, uint32_t client_socket = 0);

    // ���� ������ ���� ����
    std::vector<uint8_t> CreateShopItemsErrorResponse(ResultCode error_code, uint32_t shop_id, uint32_t client_socket = 0);

    // ���� �ŷ� ���� ����
    std::vector<uint8_t> CreateShopTransactionErrorResponse(ResultCode error_code, const std::string& message, uint32_t client_socket = 0);

    // �Ϲ����� ���� ���� ����
    std::vector<uint8_t> CreateGenericErrorResponse(EventType response_type, ResultCode error_code, uint32_t client_socket = 0);

    // === MySQL ���� �Լ��� ===

    // MySQL ������� ���� ���� ���ڿ� �� ��������
    std::string GetStringFromRow(MYSQL_ROW row, int index);

    // MySQL ������� ���� ���� ���� �� ��������
    uint32_t GetUintFromRow(MYSQL_ROW row, int index);

    // MySQL ������� ���� ���� �Ǽ� �� ��������
    float GetFloatFromRow(MYSQL_ROW row, int index);

    // MySQL ��� ���� (NULL üũ ��)
    bool IsValidMySQLResult(MYSQL_RES* result);

    // === ��û ���� ���� �Լ��� ===

    // �α��� ��û ��ȿ�� �˻�
    bool ValidateLoginRequest(const C2S_Login* request);

    // ���� ���� ��û ��ȿ�� �˻�
    bool ValidateCreateAccountRequest(const C2S_CreateAccount* request);

    // �÷��̾� ������ ��û ��ȿ�� �˻�
    bool ValidatePlayerDataRequest(const C2S_PlayerData* request);

    // ������ ������ ��û ��ȿ�� �˻�
    bool ValidateItemDataRequest(const C2S_ItemData* request);

    // ���� ��� ��û ��ȿ�� �˻�
    bool ValidateShopListRequest(const C2S_ShopList* request);

    // ���� ������ ��û ��ȿ�� �˻�
    bool ValidateShopItemsRequest(const C2S_ShopItems* request);

    // ���� �ŷ� ��û ��ȿ�� �˻�
    bool ValidateShopTransactionRequest(const C2S_ShopTransaction* request);

    // === ��ƿ��Ƽ �Լ��� ===

    // ��Ŷ Ÿ�� Ȯ�� (EventType enum ��ȯ)
    EventType GetPacketType(const uint8_t* data, size_t size);

    // Ŭ���̾�Ʈ ���� ���� Ȯ��
    uint32_t GetClientSocket(const uint8_t* data, size_t size);

    // ��Ŷ ����
    bool IsValidPacket(const uint8_t* data, size_t size);

    // ��Ŷ Ÿ���� ���ڿ��� ��ȯ (������)
    std::string GetPacketTypeName(EventType packet_type);

    // ��� �ڵ带 ���ڿ��� ��ȯ (������)
    std::string GetResultCodeName(ResultCode result);

    // ���� �޽��� ��ȯ
    std::string GetLastError() const { return _last_error; }
};