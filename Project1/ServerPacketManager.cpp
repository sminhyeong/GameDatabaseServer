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

const C2S_ShopList* ServerPacketManager::ParseShopListRequest(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_C2S_ShopList) {
        SetError("Invalid shop list request packet");
        return nullptr;
    }

    return packet->packet_event_as_C2S_ShopList();
}

const C2S_ShopItems* ServerPacketManager::ParseShopItemsRequest(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_C2S_ShopItems) {
        SetError("Invalid shop items request packet");
        return nullptr;
    }

    return packet->packet_event_as_C2S_ShopItems();
}

const C2S_ShopTransaction* ServerPacketManager::ParseShopTransactionRequest(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_C2S_ShopTransaction) {
        SetError("Invalid shop transaction request packet");
        return nullptr;
    }

    return packet->packet_event_as_C2S_ShopTransaction();
}

const C2S_CreateGameServer* ServerPacketManager::ParseCreateGameServerRequest(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_C2S_CreateGameServer) {
        SetError("Invalid create game server request packet");
        return nullptr;
    }

    return packet->packet_event_as_C2S_CreateGameServer();
}

const C2S_GameServerList* ServerPacketManager::ParseGameServerListRequest(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_C2S_GameServerList) {
        SetError("Invalid game server list request packet");
        return nullptr;
    }

    return packet->packet_event_as_C2S_GameServerList();
}

const C2S_JoinGameServer* ServerPacketManager::ParseJoinGameServerRequest(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_C2S_JoinGameServer) {
        SetError("Invalid join game server request packet");
        return nullptr;
    }

    return packet->packet_event_as_C2S_JoinGameServer();
}

const C2S_CloseGameServer* ServerPacketManager::ParseCloseGameServerRequest(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_C2S_CloseGameServer) {
        SetError("Invalid close game server request packet");
        return nullptr;
    }

    return packet->packet_event_as_C2S_CloseGameServer();
}

const C2S_SavePlayerData* ServerPacketManager::ParseSavePlayerDataRequest(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_C2S_SavePlayerData) {
        SetError("Invalid save player data request packet");
        return nullptr;
    }

    return packet->packet_event_as_C2S_SavePlayerData();
}

// === 서버 응답 패킷 생성 (S2C) ===

std::vector<uint8_t> ServerPacketManager::CreateLoginResponse(ResultCode result, uint32_t user_id, const std::string& username, const std::string& nickname, uint32_t level, uint32_t client_socket)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto usernameOffset = builder.CreateString(username);
        auto nicknameOffset = builder.CreateString(nickname);
        auto loginResponse = CreateS2C_Login(builder, result, user_id, usernameOffset, nicknameOffset, level);
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

std::vector<uint8_t> ServerPacketManager::CreatePlayerDataResponse(ResultCode result, uint32_t user_id, const std::string& username, const std::string& nickname,
    uint32_t level, uint32_t exp, uint32_t hp, uint32_t mp, uint32_t attack,
    uint32_t defense, uint32_t gold, uint32_t map_id, float pos_x, float pos_y, uint32_t client_socket)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto usernameOffset = builder.CreateString(username);
        auto nicknameOffset = builder.CreateString(nickname);
        auto playerResponse = CreateS2C_PlayerData(builder, result, user_id, usernameOffset, nicknameOffset, level, exp, hp, mp, attack, defense, gold, map_id, pos_x, pos_y);
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

std::vector<uint8_t> ServerPacketManager::CreateShopListResponse(ResultCode result, uint32_t client_socket)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        // 빈 상점 벡터로 기본 응답 생성
        std::vector<flatbuffers::Offset<ShopData>> shops;
        auto shopsVector = builder.CreateVector(shops);

        auto shopResponse = CreateS2C_ShopList(builder, result, shopsVector);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_ShopList, shopResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateShopListResponse failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ServerPacketManager::CreateShopItemsResponse(ResultCode result, uint32_t shop_id, uint32_t client_socket)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        // 빈 아이템 벡터로 기본 응답 생성
        std::vector<flatbuffers::Offset<ItemData>> items;
        auto itemsVector = builder.CreateVector(items);

        auto shopItemsResponse = CreateS2C_ShopItems(builder, result, shop_id, itemsVector);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_ShopItems, shopItemsResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateShopItemsResponse failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ServerPacketManager::CreateShopTransactionResponse(ResultCode result, const std::string& message, uint32_t updated_gold, uint32_t client_socket)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto messageOffset = builder.CreateString(message);
        auto transactionResponse = CreateS2C_ShopTransaction(builder, result, messageOffset, updated_gold);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_ShopTransaction, transactionResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateShopTransactionResponse failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ServerPacketManager::CreateGameServerResponse(ResultCode result, uint32_t server_id, const std::string& message, uint32_t client_socket)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto messageOffset = builder.CreateString(message);
        auto gameServerResponse = CreateS2C_CreateGameServer(builder, result, server_id, messageOffset);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_CreateGameServer, gameServerResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateGameServerResponse failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ServerPacketManager::CreateGameServerListResponse(ResultCode result, uint32_t client_socket)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        // 빈 서버 벡터로 기본 응답 생성
        std::vector<flatbuffers::Offset<GameServerData>> servers;
        auto serversVector = builder.CreateVector(servers);

        auto gameServerListResponse = CreateS2C_GameServerList(builder, result, serversVector);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_GameServerList, gameServerListResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateGameServerListResponse failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ServerPacketManager::CreateJoinGameServerResponse(ResultCode result, const std::string& server_ip, uint32_t server_port, const std::string& message, uint32_t client_socket)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto serverIpOffset = builder.CreateString(server_ip);
        auto messageOffset = builder.CreateString(message);
        auto joinGameServerResponse = CreateS2C_JoinGameServer(builder, result, serverIpOffset, server_port, messageOffset);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_JoinGameServer, joinGameServerResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateJoinGameServerResponse failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ServerPacketManager::CreateCloseGameServerResponse(ResultCode result, const std::string& message, uint32_t client_socket)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto messageOffset = builder.CreateString(message);
        auto closeGameServerResponse = CreateS2C_CloseGameServer(builder, result, messageOffset);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_CloseGameServer, closeGameServerResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateCloseGameServerResponse failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ServerPacketManager::CreateSavePlayerDataResponse(ResultCode result, const std::string& message, uint32_t client_socket)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto messageOffset = builder.CreateString(message);
        auto savePlayerDataResponse = CreateS2C_SavePlayerData(builder, result, messageOffset);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_SavePlayerData, savePlayerDataResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateSavePlayerDataResponse failed: " + std::string(e.what()));
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
        std::string nickname = GetStringFromRow(row, 1);  // nickname 추가
        uint32_t level = GetUintFromRow(row, 2);

        return CreateLoginResponse(ResultCode_SUCCESS, user_id, username, nickname, level, client_socket);
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
        std::string nickname = GetStringFromRow(row, 1);  // nickname 추가
        uint32_t level = GetUintFromRow(row, 2);
        uint32_t exp = GetUintFromRow(row, 3);
        uint32_t hp = GetUintFromRow(row, 4);
        uint32_t mp = GetUintFromRow(row, 5);
        uint32_t attack = GetUintFromRow(row, 6);
        uint32_t defense = GetUintFromRow(row, 7);
        uint32_t gold = GetUintFromRow(row, 8);
        uint32_t map_id = GetUintFromRow(row, 9);
        float pos_x = GetFloatFromRow(row, 10);
        float pos_y = GetFloatFromRow(row, 11);

        return CreatePlayerDataResponse(ResultCode_SUCCESS, user_id, username, nickname, level, exp, hp, mp,
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
            uint32_t base_price = GetUintFromRow(row, 4);
            uint32_t attack_bonus = GetUintFromRow(row, 5);
            uint32_t defense_bonus = GetUintFromRow(row, 6);
            uint32_t hp_bonus = GetUintFromRow(row, 7);
            uint32_t mp_bonus = GetUintFromRow(row, 8);
            std::string description = GetStringFromRow(row, 9);

            auto itemNameOffset = builder.CreateString(item_name);
            auto descriptionOffset = builder.CreateString(description);
            auto itemData = CreateItemData(builder, item_id, itemNameOffset, item_count, item_type,
                base_price, attack_bonus, defense_bonus, hp_bonus, mp_bonus, descriptionOffset);
            items.push_back(itemData);

            if (first_row) {
                gold = GetUintFromRow(row, 10); // gold는 첫 번째 행에서만 가져오기
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

std::vector<uint8_t> ServerPacketManager::CreateShopListResponseFromDB(MYSQL_RES* result, uint32_t client_socket)
{
    if (!IsValidMySQLResult(result)) {
        return CreateShopListResponse(ResultCode_FAIL, client_socket);
    }

    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        std::vector<flatbuffers::Offset<ShopData>> shops;

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            uint32_t shop_id = GetUintFromRow(row, 0);
            std::string shop_name = GetStringFromRow(row, 1);
            uint32_t shop_type = GetUintFromRow(row, 2);
            uint32_t map_id = GetUintFromRow(row, 3);
            float pos_x = GetFloatFromRow(row, 4);
            float pos_y = GetFloatFromRow(row, 5);

            auto shopNameOffset = builder.CreateString(shop_name);
            auto shopData = CreateShopData(builder, shop_id, shopNameOffset, shop_type, map_id, pos_x, pos_y);
            shops.push_back(shopData);
        }

        auto shopsVector = builder.CreateVector(shops);
        auto shopResponse = CreateS2C_ShopList(builder, ResultCode_SUCCESS, shopsVector);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_ShopList, shopResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateShopListResponseFromDB failed: " + std::string(e.what()));
        return CreateShopListResponse(ResultCode_FAIL, client_socket);
    }
}

std::vector<uint8_t> ServerPacketManager::CreateShopItemsResponseFromDB(MYSQL_RES* result, uint32_t shop_id, uint32_t client_socket)
{
    if (!IsValidMySQLResult(result)) {
        return CreateShopItemsResponse(ResultCode_FAIL, shop_id, client_socket);
    }

    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        std::vector<flatbuffers::Offset<ItemData>> items;

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            uint32_t item_id = GetUintFromRow(row, 0);
            std::string item_name = GetStringFromRow(row, 1);
            uint32_t item_type = GetUintFromRow(row, 2);
            uint32_t base_price = GetUintFromRow(row, 3);
            uint32_t attack_bonus = GetUintFromRow(row, 4);
            uint32_t defense_bonus = GetUintFromRow(row, 5);
            uint32_t hp_bonus = GetUintFromRow(row, 6);
            uint32_t mp_bonus = GetUintFromRow(row, 7);
            std::string description = GetStringFromRow(row, 8);

            auto itemNameOffset = builder.CreateString(item_name);
            auto descriptionOffset = builder.CreateString(description);
            auto itemData = CreateItemData(builder, item_id, itemNameOffset, 1, item_type, // 상점에서는 수량 1로 설정
                base_price, attack_bonus, defense_bonus, hp_bonus, mp_bonus, descriptionOffset);
            items.push_back(itemData);
        }

        auto itemsVector = builder.CreateVector(items);
        auto shopItemsResponse = CreateS2C_ShopItems(builder, ResultCode_SUCCESS, shop_id, itemsVector);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_ShopItems, shopItemsResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateShopItemsResponseFromDB failed: " + std::string(e.what()));
        return CreateShopItemsResponse(ResultCode_FAIL, shop_id, client_socket);
    }
}

std::vector<uint8_t> ServerPacketManager::CreateGameServerListResponseFromDB(MYSQL_RES* result, uint32_t client_socket)
{
    if (!IsValidMySQLResult(result)) {
        return CreateGameServerListResponse(ResultCode_FAIL, client_socket);
    }

    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        std::vector<flatbuffers::Offset<GameServerData>> servers;

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            uint32_t server_id = GetUintFromRow(row, 0);
            std::string server_name = GetStringFromRow(row, 1);
            std::string server_ip = GetStringFromRow(row, 2);
            uint32_t server_port = GetUintFromRow(row, 3);
            uint32_t owner_user_id = GetUintFromRow(row, 4);
            std::string owner_nickname = GetStringFromRow(row, 5);
            uint32_t current_players = GetUintFromRow(row, 6);
            uint32_t max_players = GetUintFromRow(row, 7);
            bool has_password = GetStringFromRow(row, 8) != ""; // password가 비어있지 않으면 true

            auto serverNameOffset = builder.CreateString(server_name);
            auto serverIpOffset = builder.CreateString(server_ip);
            auto ownerNicknameOffset = builder.CreateString(owner_nickname);

            auto gameServerData = CreateGameServerData(builder, server_id, serverNameOffset,
                serverIpOffset, server_port, owner_user_id, ownerNicknameOffset,
                current_players, max_players, has_password);
            servers.push_back(gameServerData);
        }

        auto serversVector = builder.CreateVector(servers);
        auto gameServerListResponse = CreateS2C_GameServerList(builder, ResultCode_SUCCESS, serversVector);
        auto packet = CreateDatabasePacket(builder, EventType_S2C_GameServerList, gameServerListResponse.Union(), client_socket);

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateGameServerListResponseFromDB failed: " + std::string(e.what()));
        return CreateGameServerListResponse(ResultCode_FAIL, client_socket);
    }
}

// === 간편한 에러 응답 생성 ===

std::vector<uint8_t> ServerPacketManager::CreateLoginErrorResponse(ResultCode error_code, uint32_t client_socket)
{
    return CreateLoginResponse(error_code, 0, "", "", 0, client_socket);
}

std::vector<uint8_t> ServerPacketManager::CreateAccountErrorResponse(ResultCode error_code, const std::string& message, uint32_t client_socket)
{
    return CreateAccountResponse(error_code, 0, message, client_socket);
}

std::vector<uint8_t> ServerPacketManager::CreatePlayerDataErrorResponse(ResultCode error_code, uint32_t client_socket)
{
    return CreatePlayerDataResponse(error_code, 0, "", "", 0, 0, 0, 0, 0, 0, 0, 0, 0.0f, 0.0f, client_socket);
}

std::vector<uint8_t> ServerPacketManager::CreateItemDataErrorResponse(ResultCode error_code, uint32_t user_id, uint32_t client_socket)
{
    return CreateItemDataResponse(error_code, user_id, 0, client_socket);
}

std::vector<uint8_t> ServerPacketManager::CreateShopListErrorResponse(ResultCode error_code, uint32_t client_socket)
{
    return CreateShopListResponse(error_code, client_socket);
}

std::vector<uint8_t> ServerPacketManager::CreateShopItemsErrorResponse(ResultCode error_code, uint32_t shop_id, uint32_t client_socket)
{
    return CreateShopItemsResponse(error_code, shop_id, client_socket);
}

std::vector<uint8_t> ServerPacketManager::CreateShopTransactionErrorResponse(ResultCode error_code, const std::string& message, uint32_t client_socket)
{
    return CreateShopTransactionResponse(error_code, message, 0, client_socket);
}

std::vector<uint8_t> ServerPacketManager::CreateGameServerErrorResponse(ResultCode error_code, const std::string& message, uint32_t client_socket)
{
    return CreateGameServerResponse(error_code, 0, message, client_socket);
}

std::vector<uint8_t> ServerPacketManager::CreateGameServerListErrorResponse(ResultCode error_code, uint32_t client_socket)
{
    return CreateGameServerListResponse(error_code, client_socket);
}

std::vector<uint8_t> ServerPacketManager::CreateJoinGameServerErrorResponse(ResultCode error_code, const std::string& message, uint32_t client_socket)
{
    return CreateJoinGameServerResponse(error_code, "", 0, message, client_socket);
}

std::vector<uint8_t> ServerPacketManager::CreateCloseGameServerErrorResponse(ResultCode error_code, const std::string& message, uint32_t client_socket)
{
    return CreateCloseGameServerResponse(error_code, message, client_socket);
}

std::vector<uint8_t> ServerPacketManager::CreateSavePlayerDataErrorResponse(ResultCode error_code, const std::string& message, uint32_t client_socket)
{
    return CreateSavePlayerDataResponse(error_code, message, client_socket);
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
    case EventType_S2C_ShopList:
        return CreateShopListErrorResponse(error_code, client_socket);
    case EventType_S2C_ShopItems:
        return CreateShopItemsErrorResponse(error_code, 0, client_socket);
    case EventType_S2C_ShopTransaction:
        return CreateShopTransactionErrorResponse(error_code, "Generic error", client_socket);
        // 게임 서버 관련 추가
    case EventType_S2C_CreateGameServer:
        return CreateGameServerErrorResponse(error_code, "Generic error", client_socket);
    case EventType_S2C_GameServerList:
        return CreateGameServerListErrorResponse(error_code, client_socket);
    case EventType_S2C_JoinGameServer:
        return CreateJoinGameServerErrorResponse(error_code, "Generic error", client_socket);
    case EventType_S2C_CloseGameServer:
        return CreateCloseGameServerErrorResponse(error_code, "Generic error", client_socket);
    case EventType_S2C_SavePlayerData:
        return CreateSavePlayerDataErrorResponse(error_code, "Generic error", client_socket);
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

    // request_type: 0 = 조회, 1 = 추가, 2 = 삭제, 3 = 사용
    if (request->request_type() > 3) {
        SetError("Invalid request type");
        return false;
    }

    return true;
}

bool ServerPacketManager::ValidateShopListRequest(const C2S_ShopList* request)
{
    if (!request) {
        SetError("Shop list request is null");
        return false;
    }

    // request_type: 0 = 상점목록조회
    if (request->request_type() > 0) {
        SetError("Invalid request type");
        return false;
    }

    return true;
}

bool ServerPacketManager::ValidateShopItemsRequest(const C2S_ShopItems* request)
{
    if (!request) {
        SetError("Shop items request is null");
        return false;
    }

    if (request->shop_id() == 0) {
        SetError("Invalid shop ID");
        return false;
    }

    return true;
}

bool ServerPacketManager::ValidateShopTransactionRequest(const C2S_ShopTransaction* request)
{
    if (!request) {
        SetError("Shop transaction request is null");
        return false;
    }

    if (request->user_id() == 0) {
        SetError("Invalid user ID");
        return false;
    }

    if (request->shop_id() == 0) {
        SetError("Invalid shop ID");
        return false;
    }

    if (request->item_id() == 0) {
        SetError("Invalid item ID");
        return false;
    }

    if (request->item_count() == 0) {
        SetError("Invalid item count");
        return false;
    }

    // transaction_type: 0 = 구매, 1 = 판매
    if (request->transaction_type() > 1) {
        SetError("Invalid transaction type");
        return false;
    }

    return true;
}

bool ServerPacketManager::ValidateCreateGameServerRequest(const C2S_CreateGameServer* request)
{
    if (!request) {
        SetError("Create game server request is null");
        return false;
    }

    if (request->user_id() == 0) {
        SetError("Invalid user ID");
        return false;
    }

    if (!request->server_name() || request->server_name()->size() == 0) {
        SetError("Server name is empty");
        return false;
    }

    if (request->server_name()->size() > 100) {
        SetError("Server name too long");
        return false;
    }

    if (!request->server_ip() || request->server_ip()->size() == 0) {
        SetError("Server IP is empty");
        return false;
    }

    if (request->server_port() == 0 || request->server_port() > 65535) {
        SetError("Invalid server port");
        return false;
    }

    if (request->max_players() == 0 || request->max_players() > 100) {
        SetError("Invalid max players count");
        return false;
    }

    return true;
}

bool ServerPacketManager::ValidateGameServerListRequest(const C2S_GameServerList* request)
{
    if (!request) {
        SetError("Game server list request is null");
        return false;
    }

    // request_type: 0 = 전체조회
    if (request->request_type() > 0) {
        SetError("Invalid request type");
        return false;
    }

    return true;
}

bool ServerPacketManager::ValidateJoinGameServerRequest(const C2S_JoinGameServer* request)
{
    if (!request) {
        SetError("Join game server request is null");
        return false;
    }

    if (request->user_id() == 0) {
        SetError("Invalid user ID");
        return false;
    }

    if (request->server_id() == 0) {
        SetError("Invalid server ID");
        return false;
    }

    return true;
}

bool ServerPacketManager::ValidateCloseGameServerRequest(const C2S_CloseGameServer* request)
{
    if (!request) {
        SetError("Close game server request is null");
        return false;
    }

    if (request->user_id() == 0) {
        SetError("Invalid user ID");
        return false;
    }

    if (request->server_id() == 0) {
        SetError("Invalid server ID");
        return false;
    }

    return true;
}

bool ServerPacketManager::ValidateSavePlayerDataRequest(const C2S_SavePlayerData* request)
{
    if (!request) {
        SetError("Save player data request is null");
        return false;
    }

    if (request->user_id() == 0) {
        SetError("Invalid user ID");
        return false;
    }

    // 레벨, 경험치 등의 유효성 검사
    if (request->level() == 0 || request->level() > 1000) {
        SetError("Invalid level");
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
    case EventType_C2S_ShopList: return "C2S_ShopList";
    case EventType_S2C_ShopList: return "S2C_ShopList";
    case EventType_C2S_ShopItems: return "C2S_ShopItems";
    case EventType_S2C_ShopItems: return "S2C_ShopItems";
    case EventType_C2S_ShopTransaction: return "C2S_ShopTransaction";
    case EventType_S2C_ShopTransaction: return "S2C_ShopTransaction";
    case EventType_C2S_CreateGameServer: return "C2S_CreateGameServer";
    case EventType_S2C_CreateGameServer: return "S2C_CreateGameServer";
    case EventType_C2S_GameServerList: return "C2S_GameServerList";
    case EventType_S2C_GameServerList: return "S2C_GameServerList";
    case EventType_C2S_JoinGameServer: return "C2S_JoinGameServer";
    case EventType_S2C_JoinGameServer: return "S2C_JoinGameServer";
    case EventType_C2S_CloseGameServer: return "C2S_CloseGameServer";
    case EventType_S2C_CloseGameServer: return "S2C_CloseGameServer";
    case EventType_C2S_SavePlayerData: return "C2S_SavePlayerData";
    case EventType_S2C_SavePlayerData: return "S2C_SavePlayerData";
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
    case ResultCode_INSUFFICIENT_GOLD: return "INSUFFICIENT_GOLD";
    case ResultCode_ITEM_NOT_FOUND: return "ITEM_NOT_FOUND";
    case ResultCode_SHOP_NOT_FOUND: return "SHOP_NOT_FOUND";
    default: return "Unknown";
    }
}