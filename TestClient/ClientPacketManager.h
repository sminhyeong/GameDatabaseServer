#pragma once

#include <vector>
#include <string>
#include <cstdint>

// ���� ���� (�ߺ� ����)
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

// ���� ���� ���� ����ü �߰�
struct S2C_CreateGameServer;
struct S2C_GameServerList;
struct S2C_JoinGameServer;
struct S2C_CloseGameServer;
struct S2C_SavePlayerData;
struct GameServerData;

enum EventType : uint8_t;
enum ResultCode : int8_t;

// Ŭ���̾�Ʈ�� ��Ŷ �Ŵ��� Ŭ����
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

    // === ���� Ŭ���̾�Ʈ ��û ��Ŷ ���� (C2S) ===

    // �α��� ��û
    std::vector<uint8_t> CreateLoginRequest(const std::string& username, const std::string& password);

    // �α׾ƿ� ��û
    std::vector<uint8_t> CreateLogoutRequest(uint32_t user_id);

    // ���� ���� ��û
    std::vector<uint8_t> CreateAccountRequest(const std::string& username, const std::string& password, const std::string& nickname);

    // ������ ������ ��û
    std::vector<uint8_t> CreateItemDataRequest(uint32_t user_id, uint32_t request_type, uint32_t item_id = 0, uint32_t item_count = 0);

    // �÷��̾� ������ ��û (��ȸ��)
    std::vector<uint8_t> CreatePlayerDataQueryRequest(uint32_t user_id);

    // �÷��̾� ������ ��û (������Ʈ��)
    std::vector<uint8_t> CreatePlayerDataUpdateRequest(uint32_t user_id, uint32_t level, uint32_t exp,
        uint32_t hp, uint32_t mp, float pos_x, float pos_y);

    // ���� ������ ��û
    std::vector<uint8_t> CreateMonsterDataRequest(uint32_t request_type, uint32_t monster_id = 0);

    // ä�� �޽��� ���� ��û
    std::vector<uint8_t> CreateChatSendRequest(uint32_t sender_id, uint32_t receiver_id, const std::string& message, uint32_t chat_type);

    // ä�� ��ȸ ��û
    std::vector<uint8_t> CreateChatQueryRequest(uint32_t request_type, uint32_t sender_id, uint32_t receiver_id, uint32_t chat_type);

    // ���� ��� ��û
    std::vector<uint8_t> CreateShopListRequest(uint32_t request_type = 0, uint32_t map_id = 0);

    // ���� ������ ��� ��û
    std::vector<uint8_t> CreateShopItemsRequest(uint32_t shop_id);

    // ���� �ŷ� ��û (����/�Ǹ�)
    std::vector<uint8_t> CreateShopTransactionRequest(uint32_t user_id, uint32_t shop_id, uint32_t item_id, uint32_t item_count, uint32_t transaction_type);

    // === ���� ���� ���� Ŭ���̾�Ʈ ��û ��Ŷ ���� (C2S) �߰� ===

    // ���� ���� ���� ��û
    std::vector<uint8_t> CreateGameServerRequest(uint32_t user_id, const std::string& server_name,
        const std::string& server_password, const std::string& server_ip, uint32_t server_port, uint32_t max_players = 10);

    // ���� ���� ��� ��û
    std::vector<uint8_t> CreateGameServerListRequest(uint32_t request_type = 0);

    // ���� ���� ���� ��û
    std::vector<uint8_t> CreateJoinGameServerRequest(uint32_t user_id, uint32_t server_id, const std::string& server_password = "");

    // ���� ���� ���� ��û (���� �����ڸ�)
    std::vector<uint8_t> CreateCloseGameServerRequest(uint32_t user_id, uint32_t server_id);

    // �÷��̾� ������ ���� ��û (���� �����)
    std::vector<uint8_t> CreateSavePlayerDataRequest(uint32_t user_id, uint32_t level, uint32_t exp,
        uint32_t hp, uint32_t mp, uint32_t gold, float pos_x, float pos_y);

    // === ���� ���� ���� ��Ŷ �Ľ� (S2C) ===
    // FlatBuffers ����ü �����͸� ���� ��ȯ

    // �α��� ���� �Ľ�
    const S2C_Login* ParseLoginResponse(const uint8_t* data, size_t size);

    // �α׾ƿ� ���� �Ľ�
    const S2C_Logout* ParseLogoutResponse(const uint8_t* data, size_t size);

    // ���� ���� ���� �Ľ�
    const S2C_CreateAccount* ParseCreateAccountResponse(const uint8_t* data, size_t size);

    // ������ ������ ���� �Ľ�
    const S2C_ItemData* ParseItemDataResponse(const uint8_t* data, size_t size);

    // �÷��̾� ������ ���� �Ľ�
    const S2C_PlayerData* ParsePlayerDataResponse(const uint8_t* data, size_t size);

    // ���� ������ ���� �Ľ�
    const S2C_MonsterData* ParseMonsterDataResponse(const uint8_t* data, size_t size);

    // ä�� ���� �Ľ�
    const S2C_PlayerChat* ParsePlayerChatResponse(const uint8_t* data, size_t size);

    // ���� ��� ���� �Ľ�
    const S2C_ShopList* ParseShopListResponse(const uint8_t* data, size_t size);

    // ���� ������ ���� �Ľ�
    const S2C_ShopItems* ParseShopItemsResponse(const uint8_t* data, size_t size);

    // ���� �ŷ� ���� �Ľ�
    const S2C_ShopTransaction* ParseShopTransactionResponse(const uint8_t* data, size_t size);

    // === ���� ���� ���� ���� ���� ��Ŷ �Ľ� (S2C) �߰� ===

    // ���� ���� ���� ���� �Ľ�
    const S2C_CreateGameServer* ParseCreateGameServerResponse(const uint8_t* data, size_t size);

    // ���� ���� ��� ���� �Ľ�
    const S2C_GameServerList* ParseGameServerListResponse(const uint8_t* data, size_t size);

    // ���� ���� ���� ���� �Ľ�
    const S2C_JoinGameServer* ParseJoinGameServerResponse(const uint8_t* data, size_t size);

    // ���� ���� ���� ���� �Ľ�
    const S2C_CloseGameServer* ParseCloseGameServerResponse(const uint8_t* data, size_t size);

    // �÷��̾� ������ ���� ���� �Ľ�
    const S2C_SavePlayerData* ParseSavePlayerDataResponse(const uint8_t* data, size_t size);

    // === ���� �Լ��� ===

    // �α��� ��� Ȯ��
    bool IsLoginSuccess(const S2C_Login* response);

    // ���� ���� ��� Ȯ��
    bool IsCreateAccountSuccess(const S2C_CreateAccount* response);

    // �÷��̾� ������ ��ȸ ���� Ȯ��
    bool IsPlayerDataValid(const S2C_PlayerData* response);

    // ������ ������ ��ȸ ���� Ȯ��
    bool IsItemDataValid(const S2C_ItemData* response);

    // ���� ��� ��ȸ ���� Ȯ��
    bool IsShopListValid(const S2C_ShopList* response);

    // ���� ������ ��ȸ ���� Ȯ��
    bool IsShopItemsValid(const S2C_ShopItems* response);

    // ���� �ŷ� ���� Ȯ��
    bool IsShopTransactionSuccess(const S2C_ShopTransaction* response);

    // === ���� ���� ���� ���� �Լ��� �߰� ===

    // ���� ���� ���� ���� Ȯ��
    bool IsCreateGameServerSuccess(const S2C_CreateGameServer* response);

    // ���� ���� ��� ��ȸ ���� Ȯ��
    bool IsGameServerListValid(const S2C_GameServerList* response);

    // ���� ���� ���� ���� Ȯ��
    bool IsJoinGameServerSuccess(const S2C_JoinGameServer* response);

    // ���� ���� ���� ���� Ȯ��
    bool IsCloseGameServerSuccess(const S2C_CloseGameServer* response);

    // �÷��̾� ������ ���� ���� Ȯ��
    bool IsSavePlayerDataSuccess(const S2C_SavePlayerData* response);

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