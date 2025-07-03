#define NOMINMAX

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#include "ServerPacketManager.h"
#include "flatbuffers/flatbuffers.h"
#include "UserEvent_generated.h"
#include <iostream>

ServerPacketManager::ServerPacketManager()
{
    ClearError();
}

ServerPacketManager::~ServerPacketManager()
{
}

void ServerPacketManager::SetError(const std::string& error)
{
    _last_error = error;
}

void ServerPacketManager::ClearError()
{
    _last_error.clear();
}

bool ServerPacketManager::VerifyPacket(const uint8_t* data, size_t size)
{
    if (!data || size == 0) {
        SetError("Invalid packet data");
        return false;
    }

    flatbuffers::Verifier verifier(data, size);
    return VerifyDatabasePacketBuffer(verifier);
}

// === 클라이언트 요청 패킷 파싱 (C2S) ===

const C2S_Login* ServerPacketManager::ParseLoginRequest(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_C2S_Login) {
        SetError("Invalid login request packet");
        return nullptr;
    }

    return packet->packet_event_as_C2S_Login();
}

const C2S_Logout* ServerPacketManager::ParseLogoutRequest(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_C2S_Logout) {
        SetError("Invalid logout request packet");
        return nullptr;
    }

    return packet->packet_event_as_C2S_Logout();
}

const C2S_CreateAccount* ServerPacketManager::ParseCreateAccountRequest(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_C2S_CreateAccount) {
        SetError("Invalid create account request packet");
        return nullptr;
    }

    return packet->packet_event_as_C2S_CreateAccount();
}

const C2S_ItemData* ServerPacketManager::ParseItemDataRequest(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_C2S_ItemData) {
        SetError("Invalid item data request packet");
        return nullptr;
    }

    return packet->packet_event_as_C2S_ItemData();
}

const C2S_PlayerData* ServerPacketManager::ParsePlayerDataRequest(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_C2S_PlayerData) {
        SetError("Invalid player data request packet");
        return nullptr;
    }

    return packet->packet_event_as_C2S_PlayerData();
}

const C2S_MonsterData* ServerPacketManager::ParseMonsterDataRequest(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_C2S_MonsterData) {
        SetError("Invalid monster data request packet");
        return nullptr;
    }

    return packet->packet_event_as_C2S_MonsterData();
}

const C2S_PlayerChat* ServerPacketManager::ParsePlayerChatRequest(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_C2S_PlayerChat) {
        SetError("Invalid player chat request packet");
        return nullptr;
    }

    return packet->packet_event_as_C2S_PlayerChat();
}

// === 서버 응답 패킷 생성 (S2C) ===

std::vector<uint8_t> ServerPacketManager::CreateLoginResponse(ResultCode result, uint32_t user_id, const std::string& username, uint32_t level, uint32_t client_socket)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto usernameOffset = builder.CreateString(username);
        auto loginResponse = CreateS2C_Login(builder, result, user_id, usernameOffset, level);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_Login, loginResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateLoginResponse failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ServerPacketManager::CreateLogoutResponse(ResultCode result, const std::string& message, uint32_t client_socket)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto messageOffset = builder.CreateString(message);
        auto logoutResponse = CreateS2C_Logout(builder, result, messageOffset);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_Logout, logoutResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateLogoutResponse failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ServerPacketManager::CreateAccountResponse(ResultCode result, uint32_t user_id, const std::string& message, uint32_t client_socket)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto messageOffset = builder.CreateString(message);
        auto accountResponse = CreateS2C_CreateAccount(builder, result, user_id, messageOffset);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_CreateAccount, accountResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateAccountResponse failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ServerPacketManager::CreateItemDataResponse(ResultCode result, uint32_t user_id, uint32_t gold, uint32_t client_socket)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        // 빈 아이템 벡터로 기본 응답 생성
        std::vector<flatbuffers::Offset<ItemData>> items;
        auto itemsVector = builder.CreateVector(items);

        auto itemResponse = CreateS2C_ItemData(builder, result, user_id, itemsVector, gold);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_ItemData, itemResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateItemDataResponse failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ServerPacketManager::CreatePlayerDataResponse(ResultCode result, uint32_t user_id, const std::string& username,
    uint32_t level, uint32_t exp, uint32_t hp, uint32_t mp, uint32_t attack,
    uint32_t defense, uint32_t gold, uint32_t map_id, float pos_x, float pos_y, uint32_t client_socket)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto usernameOffset = builder.CreateString(username);
        auto playerResponse = CreateS2C_PlayerData(builder, result, user_id, usernameOffset, level, exp, hp, mp, attack, defense, gold, map_id, pos_x, pos_y);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_PlayerData, playerResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreatePlayerDataResponse failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ServerPacketManager::CreateMonsterDataResponse(ResultCode result, uint32_t client_socket)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        // 빈 몬스터 벡터로 기본 응답 생성
        std::vector<flatbuffers::Offset<MonsterData>> monsters;
        auto monstersVector = builder.CreateVector(monsters);

        auto monsterResponse = CreateS2C_MonsterData(builder, result, monstersVector);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_MonsterData, monsterResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateMonsterDataResponse failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ServerPacketManager::CreatePlayerChatResponse(ResultCode result, uint32_t client_socket)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        // 빈 채팅 벡터로 기본 응답 생성
        std::vector<flatbuffers::Offset<ChatData>> chats;
        auto chatsVector = builder.CreateVector(chats);

        auto chatResponse = CreateS2C_PlayerChat(builder, result, chatsVector);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_PlayerChat, chatResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreatePlayerChatResponse failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

// === MySQL 결과에서 직접 응답 패킷 생성 (서버 전용) ===

std::vector<uint8_t> ServerPacketManager::CreateLoginResponseFromDB(MYSQL_RES* result, const std::string& username, uint32_t client_socket)
{
    if (!IsValidMySQLResult(result)) {
        return CreateLoginErrorResponse(ResultCode_FAIL, client_socket);
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        return CreateLoginErrorResponse(ResultCode_INVALID_USER, client_socket);
    }

    try {
        uint32_t user_id = GetUintFromRow(row, 0);
        uint32_t level = GetUintFromRow(row, 1);

        return CreateLoginResponse(ResultCode_SUCCESS, user_id, username, level, client_socket);
    }
    catch (const std::exception& e) {
        SetError("CreateLoginResponseFromDB failed: " + std::string(e.what()));
        return CreateLoginErrorResponse(ResultCode_FAIL, client_socket);
    }
}

std::vector<uint8_t> ServerPacketManager::CreatePlayerDataResponseFromDB(MYSQL_RES* result, uint32_t user_id, uint32_t client_socket)
{
    if (!IsValidMySQLResult(result)) {
        return CreatePlayerDataErrorResponse(ResultCode_FAIL, client_socket);
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        return CreatePlayerDataErrorResponse(ResultCode_USER_NOT_FOUND, client_socket);
    }

    try {
        std::string username = GetStringFromRow(row, 0);
        uint32_t level = GetUintFromRow(row, 1);
        uint32_t exp = GetUintFromRow(row, 2);
        uint32_t hp = GetUintFromRow(row, 3);
        uint32_t mp = GetUintFromRow(row, 4);
        uint32_t attack = GetUintFromRow(row, 5);
        uint32_t defense = GetUintFromRow(row, 6);
        uint32_t gold = GetUintFromRow(row, 7);
        uint32_t map_id = GetUintFromRow(row, 8);
        float pos_x = GetFloatFromRow(row, 9);
        float pos_y = GetFloatFromRow(row, 10);

        return CreatePlayerDataResponse(ResultCode_SUCCESS, user_id, username, level, exp, hp, mp,
            attack, defense, gold, map_id, pos_x, pos_y, client_socket);
    }
    catch (const std::exception& e) {
        SetError("CreatePlayerDataResponseFromDB failed: " + std::string(e.what()));
        return CreatePlayerDataErrorResponse(ResultCode_FAIL, client_socket);
    }
}

std::vector<uint8_t> ServerPacketManager::CreateItemDataResponseFromDB(MYSQL_RES* result, uint32_t user_id, uint32_t client_socket)
{
    if (!IsValidMySQLResult(result)) {
        return CreateItemDataErrorResponse(ResultCode_FAIL, user_id, client_socket);
    }

    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        std::vector<flatbuffers::Offset<ItemData>> items;
        uint32_t gold = 0;
        bool first_row = true;

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            uint32_t item_id = GetUintFromRow(row, 0);
            std::string item_name = GetStringFromRow(row, 1);
            uint32_t item_count = GetUintFromRow(row, 2);
            uint32_t item_type = GetUintFromRow(row, 3);

            auto itemNameOffset = builder.CreateString(item_name);
            auto itemData = CreateItemData(builder, item_id, itemNameOffset, item_count, item_type);
            items.push_back(itemData);

            if (first_row) {
                gold = GetUintFromRow(row, 4); // gold는 첫 번째 행에서만 가져오기
                first_row = false;
            }
        }

        auto itemsVector = builder.CreateVector(items);
        auto itemResponse = CreateS2C_ItemData(builder, ResultCode_SUCCESS, user_id, itemsVector, gold);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_ItemData, itemResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateItemDataResponseFromDB failed: " + std::string(e.what()));
        return CreateItemDataErrorResponse(ResultCode_FAIL, user_id, client_socket);
    }
}

std::vector<uint8_t> ServerPacketManager::CreateMonsterDataResponseFromDB(MYSQL_RES* result, uint32_t client_socket)
{
    if (!IsValidMySQLResult(result)) {
        return CreateMonsterDataResponse(ResultCode_FAIL, client_socket);
    }

    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        std::vector<flatbuffers::Offset<MonsterData>> monsters;

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            uint32_t monster_id = GetUintFromRow(row, 0);
            std::string monster_name = GetStringFromRow(row, 1);
            uint32_t level = GetUintFromRow(row, 2);
            uint32_t hp = GetUintFromRow(row, 3);
            uint32_t attack = GetUintFromRow(row, 4);
            uint32_t defense = GetUintFromRow(row, 5);
            uint32_t exp_reward = GetUintFromRow(row, 6);
            uint32_t gold_reward = GetUintFromRow(row, 7);

            auto monsterNameOffset = builder.CreateString(monster_name);
            auto monsterData = CreateMonsterData(builder, monster_id, monsterNameOffset, level, hp, attack, defense, exp_reward, gold_reward);
            monsters.push_back(monsterData);
        }

        auto monstersVector = builder.CreateVector(monsters);
        auto monsterResponse = CreateS2C_MonsterData(builder, ResultCode_SUCCESS, monstersVector);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_MonsterData, monsterResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateMonsterDataResponseFromDB failed: " + std::string(e.what()));
        return CreateMonsterDataResponse(ResultCode_FAIL, client_socket);
    }
}

std::vector<uint8_t> ServerPacketManager::CreatePlayerChatResponseFromDB(MYSQL_RES* result, uint32_t client_socket)
{
    if (!IsValidMySQLResult(result)) {
        return CreatePlayerChatResponse(ResultCode_FAIL, client_socket);
    }

    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        std::vector<flatbuffers::Offset<ChatData>> chats;

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            uint32_t chat_id = GetUintFromRow(row, 0);
            uint32_t sender_id = GetUintFromRow(row, 1);
            std::string sender_name = GetStringFromRow(row, 2);
            std::string message = GetStringFromRow(row, 3);
            uint32_t chat_type = GetUintFromRow(row, 4);
            uint64_t timestamp = static_cast<uint64_t>(GetUintFromRow(row, 5));

            auto senderNameOffset = builder.CreateString(sender_name);
            auto messageOffset = builder.CreateString(message);
            auto chatData = CreateChatData(builder, chat_id, sender_id, senderNameOffset, messageOffset, chat_type, timestamp);
            chats.push_back(chatData);
        }

        auto chatsVector = builder.CreateVector(chats);
        auto chatResponse = CreateS2C_PlayerChat(builder, ResultCode_SUCCESS, chatsVector);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_PlayerChat, chatResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreatePlayerChatResponseFromDB failed: " + std::string(e.what()));
        return CreatePlayerChatResponse(ResultCode_FAIL, client_socket);
    }
}

// === 간편한 에러 응답 생성 ===

std::vector<uint8_t> ServerPacketManager::CreateLoginErrorResponse(ResultCode error_code, uint32_t client_socket)
{
    return CreateLoginResponse(error_code, 0, "", 0, client_socket);
}

std::vector<uint8_t> ServerPacketManager::CreateAccountErrorResponse(ResultCode error_code, const std::string& message, uint32_t client_socket)
{
    return CreateAccountResponse(error_code, 0, message, client_socket);
}

std::vector<uint8_t> ServerPacketManager::CreatePlayerDataErrorResponse(ResultCode error_code, uint32_t client_socket)
{
    return CreatePlayerDataResponse(error_code, 0, "", 0, 0, 0, 0, 0, 0, 0, 0, 0.0f, 0.0f, client_socket);
}

std::vector<uint8_t> ServerPacketManager::CreateItemDataErrorResponse(ResultCode error_code, uint32_t user_id, uint32_t client_socket)
{
    return CreateItemDataResponse(error_code, user_id, 0, client_socket);
}

std::vector<uint8_t> ServerPacketManager::CreateGenericErrorResponse(EventType response_type, ResultCode error_code, uint32_t client_socket)
{
    switch (response_type) {
    case EventType_S2C_Login:
        return CreateLoginErrorResponse(error_code, client_socket);
    case EventType_S2C_CreateAccount:
        return CreateAccountErrorResponse(error_code, "Generic error", client_socket);
    case EventType_S2C_PlayerData:
        return CreatePlayerDataErrorResponse(error_code, client_socket);
    case EventType_S2C_ItemData:
        return CreateItemDataErrorResponse(error_code, 0, client_socket);
    default:
        SetError("Unsupported response type for generic error");
        return std::vector<uint8_t>();
    }
}

// === MySQL 헬퍼 함수들 ===

std::string ServerPacketManager::GetStringFromRow(MYSQL_ROW row, int index)
{
    return row && row[index] ? std::string(row[index]) : std::string();
}

uint32_t ServerPacketManager::GetUintFromRow(MYSQL_ROW row, int index)
{
    return row && row[index] ? static_cast<uint32_t>(std::stoul(row[index])) : 0;
}

float ServerPacketManager::GetFloatFromRow(MYSQL_ROW row, int index)
{
    return row && row[index] ? std::stof(row[index]) : 0.0f;
}

bool ServerPacketManager::IsValidMySQLResult(MYSQL_RES* result)
{
    return result != nullptr;
}

// === 요청 검증 헬퍼 함수들 ===

bool ServerPacketManager::ValidateLoginRequest(const C2S_Login* request)
{
    if (!request) {
        SetError("Login request is null");
        return false;
    }

    if (!request->username() || request->username()->size() == 0) {
        SetError("Username is empty");
        return false;
    }

    if (!request->password() || request->password()->size() == 0) {
        SetError("Password is empty");
        return false;
    }

    if (request->username()->size() > 50) {
        SetError("Username too long");
        return false;
    }

    if (request->password()->size() > 255) {
        SetError("Password too long");
        return false;
    }

    return true;
}

bool ServerPacketManager::ValidateCreateAccountRequest(const C2S_CreateAccount* request)
{
    if (!request) {
        SetError("Create account request is null");
        return false;
    }

    if (!request->username() || request->username()->size() == 0) {
        SetError("Username is empty");
        return false;
    }

    if (!request->password() || request->password()->size() == 0) {
        SetError("Password is empty");
        return false;
    }

    if (!request->nickname() || request->nickname()->size() == 0) {
        SetError("Nickname is empty");
        return false;
    }

    if (request->username()->size() > 50 || request->nickname()->size() > 50) {
        SetError("Username or nickname too long");
        return false;
    }

    return true;
}

bool ServerPacketManager::ValidatePlayerDataRequest(const C2S_PlayerData* request)
{
    if (!request) {
        SetError("Player data request is null");
        return false;
    }

    if (request->user_id() == 0) {
        SetError("Invalid user ID");
        return false;
    }

    // request_type: 0 = 조회, 1 = 업데이트
    if (request->request_type() > 1) {
        SetError("Invalid request type");
        return false;
    }

    return true;
}

bool ServerPacketManager::ValidateItemDataRequest(const C2S_ItemData* request)
{
    if (!request) {
        SetError("Item data request is null");
        return false;
    }

    if (request->user_id() == 0) {
        SetError("Invalid user ID");
        return false;
    }

    // request_type: 0 = 조회, 1 = 추가, 2 = 삭제
    if (request->request_type() > 2) {
        SetError("Invalid request type");
        return false;
    }

    return true;
}

// === 유틸리티 함수들 ===

EventType ServerPacketManager::GetPacketType(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return EventType_NONE;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    return packet ? packet->packet_event_type() : EventType_NONE;
}

uint32_t ServerPacketManager::GetClientSocket(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return 0;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    return packet ? packet->client_socket() : 0;
}

bool ServerPacketManager::IsValidPacket(const uint8_t* data, size_t size)
{
    return VerifyPacket(data, size);
}

std::string ServerPacketManager::GetPacketTypeName(EventType packet_type)
{
    switch (packet_type) {
    case EventType_C2S_Login: return "C2S_Login";
    case EventType_S2C_Login: return "S2C_Login";
    case EventType_C2S_Logout: return "C2S_Logout";
    case EventType_S2C_Logout: return "S2C_Logout";
    case EventType_C2S_CreateAccount: return "C2S_CreateAccount";
    case EventType_S2C_CreateAccount: return "S2C_CreateAccount";
    case EventType_C2S_ItemData: return "C2S_ItemData";
    case EventType_S2C_ItemData: return "S2C_ItemData";
    case EventType_C2S_PlayerData: return "C2S_PlayerData";
    case EventType_S2C_PlayerData: return "S2C_PlayerData";
    case EventType_C2S_MonsterData: return "C2S_MonsterData";
    case EventType_S2C_MonsterData: return "S2C_MonsterData";
    case EventType_C2S_PlayerChat: return "C2S_PlayerChat";
    case EventType_S2C_PlayerChat: return "S2C_PlayerChat";
    default: return "Unknown";
    }
}

std::string ServerPacketManager::GetResultCodeName(ResultCode result)
{
    switch (result) {
    case ResultCode_SUCCESS: return "SUCCESS";
    case ResultCode_FAIL: return "FAIL";
    case ResultCode_INVALID_USER: return "INVALID_USER";
    case ResultCode_USER_NOT_FOUND: return "USER_NOT_FOUND";
    default: return "Unknown";
    }
}