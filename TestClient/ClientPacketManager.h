#pragma once

#include <vector>
#include <string>
#include <cstdint>

// ���� ���� (��� �浹 ����)
struct DatabasePacket;
struct S2C_Login;
struct S2C_Logout;
struct S2C_CreateAccount;
struct S2C_ItemData;
struct S2C_PlayerData;
struct S2C_MonsterData;
struct S2C_PlayerChat;

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

    // === Ŭ���̾�Ʈ ��û ��Ŷ ���� (C2S) ===

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

    // === ���� ���� ��Ŷ �Ľ� (S2C) ===
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

    // === ���� �Լ��� ===

    // �α��� ��� Ȯ��
    bool IsLoginSuccess(const S2C_Login* response);

    // ���� ���� ��� Ȯ��
    bool IsCreateAccountSuccess(const S2C_CreateAccount* response);

    // �÷��̾� ������ ��ȸ ���� Ȯ��
    bool IsPlayerDataValid(const S2C_PlayerData* response);

    // ������ ������ ��ȸ ���� Ȯ��
    bool IsItemDataValid(const S2C_ItemData* response);

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