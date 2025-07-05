#define NOMINMAX

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#include "ClientPacketManager.h"
#include "flatbuffers/flatbuffers.h"
#include "UserEvent_generated.h"

ClientPacketManager::ClientPacketManager()
{
    ClearError();
}

ClientPacketManager::~ClientPacketManager()
{
}

void ClientPacketManager::SetError(const std::string& error)
{
    _last_error = error;
}

void ClientPacketManager::ClearError()
{
    _last_error.clear();
}

bool ClientPacketManager::VerifyPacket(const uint8_t* data, size_t size)
{
    if (!data || size == 0) {
        SetError("Invalid packet data");
        return false;
    }

    flatbuffers::Verifier verifier(data, size);
    return VerifyDatabasePacketBuffer(verifier);
}

// === 클라이언트 요청 패킷 생성 (C2S) ===

std::vector<uint8_t> ClientPacketManager::CreateLoginRequest(const std::string& username, const std::string& password)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto usernameOffset = builder.CreateString(username);
        auto passwordOffset = builder.CreateString(password);

        auto loginRequest = CreateC2S_Login(builder, usernameOffset, passwordOffset);
        auto packet = CreateDatabasePacket(builder, EventType_C2S_Login, loginRequest.Union());

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateLoginRequest failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ClientPacketManager::CreateLogoutRequest(uint32_t user_id)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto logoutRequest = CreateC2S_Logout(builder, user_id);
        auto packet = CreateDatabasePacket(builder, EventType_C2S_Logout, logoutRequest.Union());

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateLogoutRequest failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ClientPacketManager::CreateAccountRequest(const std::string& username, const std::string& password, const std::string& nickname)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto usernameOffset = builder.CreateString(username);
        auto passwordOffset = builder.CreateString(password);
        auto nicknameOffset = builder.CreateString(nickname);

        auto accountRequest = CreateC2S_CreateAccount(builder, usernameOffset, passwordOffset, nicknameOffset);
        auto packet = CreateDatabasePacket(builder, EventType_C2S_CreateAccount, accountRequest.Union());

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateAccountRequest failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ClientPacketManager::CreateItemDataRequest(uint32_t user_id, uint32_t request_type, uint32_t item_id, uint32_t item_count)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto itemRequest = CreateC2S_ItemData(builder, user_id, request_type, item_id, item_count);
        auto packet = CreateDatabasePacket(builder, EventType_C2S_ItemData, itemRequest.Union());

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateItemDataRequest failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ClientPacketManager::CreatePlayerDataQueryRequest(uint32_t user_id)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto playerRequest = CreateC2S_PlayerData(builder, user_id, 0); // request_type = 0 (조회)
        auto packet = CreateDatabasePacket(builder, EventType_C2S_PlayerData, playerRequest.Union());

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreatePlayerDataQueryRequest failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ClientPacketManager::CreatePlayerDataUpdateRequest(uint32_t user_id, uint32_t level, uint32_t exp, uint32_t hp, uint32_t mp, float pos_x, float pos_y)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto playerRequest = CreateC2S_PlayerData(builder, user_id, 1, level, exp, hp, mp, pos_x, pos_y); // request_type = 1 (업데이트)
        auto packet = CreateDatabasePacket(builder, EventType_C2S_PlayerData, playerRequest.Union());

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreatePlayerDataUpdateRequest failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ClientPacketManager::CreateMonsterDataRequest(uint32_t request_type, uint32_t monster_id)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto monsterRequest = CreateC2S_MonsterData(builder, request_type, monster_id);
        auto packet = CreateDatabasePacket(builder, EventType_C2S_MonsterData, monsterRequest.Union());

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateMonsterDataRequest failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ClientPacketManager::CreateChatSendRequest(uint32_t sender_id, uint32_t receiver_id, const std::string& message, uint32_t chat_type)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto messageOffset = builder.CreateString(message);
        auto chatRequest = CreateC2S_PlayerChat(builder, 1, sender_id, receiver_id, messageOffset, chat_type); // request_type = 1 (전송)
        auto packet = CreateDatabasePacket(builder, EventType_C2S_PlayerChat, chatRequest.Union());

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateChatSendRequest failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ClientPacketManager::CreateChatQueryRequest(uint32_t request_type, uint32_t sender_id, uint32_t receiver_id, uint32_t chat_type)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto chatRequest = CreateC2S_PlayerChat(builder, 0, sender_id, receiver_id, 0, chat_type); // request_type = 0 (조회)
        auto packet = CreateDatabasePacket(builder, EventType_C2S_PlayerChat, chatRequest.Union());

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateChatQueryRequest failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ClientPacketManager::CreateShopListRequest(uint32_t request_type, uint32_t map_id)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto shopListRequest = CreateC2S_ShopList(builder, request_type, map_id);
        auto packet = CreateDatabasePacket(builder, EventType_C2S_ShopList, shopListRequest.Union());

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateShopListRequest failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ClientPacketManager::CreateShopItemsRequest(uint32_t shop_id)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto shopItemsRequest = CreateC2S_ShopItems(builder, shop_id);
        auto packet = CreateDatabasePacket(builder, EventType_C2S_ShopItems, shopItemsRequest.Union());

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateShopItemsRequest failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ClientPacketManager::CreateShopTransactionRequest(uint32_t user_id, uint32_t shop_id, uint32_t item_id, uint32_t item_count, uint32_t transaction_type)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto transactionRequest = CreateC2S_ShopTransaction(builder, user_id, shop_id, item_id, item_count, transaction_type);
        auto packet = CreateDatabasePacket(builder, EventType_C2S_ShopTransaction, transactionRequest.Union());

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateShopTransactionRequest failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

// === 서버 응답 패킷 파싱 (S2C) ===

const S2C_Login* ClientPacketManager::ParseLoginResponse(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_S2C_Login) {
        SetError("Invalid login response packet");
        return nullptr;
    }

    return packet->packet_event_as_S2C_Login();
}

const S2C_Logout* ClientPacketManager::ParseLogoutResponse(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_S2C_Logout) {
        SetError("Invalid logout response packet");
        return nullptr;
    }

    return packet->packet_event_as_S2C_Logout();
}

const S2C_CreateAccount* ClientPacketManager::ParseCreateAccountResponse(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_S2C_CreateAccount) {
        SetError("Invalid create account response packet");
        return nullptr;
    }

    return packet->packet_event_as_S2C_CreateAccount();
}

const S2C_ItemData* ClientPacketManager::ParseItemDataResponse(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_S2C_ItemData) {
        SetError("Invalid item data response packet");
        return nullptr;
    }

    return packet->packet_event_as_S2C_ItemData();
}

const S2C_PlayerData* ClientPacketManager::ParsePlayerDataResponse(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_S2C_PlayerData) {
        SetError("Invalid player data response packet");
        return nullptr;
    }

    return packet->packet_event_as_S2C_PlayerData();
}

const S2C_MonsterData* ClientPacketManager::ParseMonsterDataResponse(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_S2C_MonsterData) {
        SetError("Invalid monster data response packet");
        return nullptr;
    }

    return packet->packet_event_as_S2C_MonsterData();
}

const S2C_PlayerChat* ClientPacketManager::ParsePlayerChatResponse(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_S2C_PlayerChat) {
        SetError("Invalid player chat response packet");
        return nullptr;
    }

    return packet->packet_event_as_S2C_PlayerChat();
}

const S2C_ShopList* ClientPacketManager::ParseShopListResponse(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_S2C_ShopList) {
        SetError("Invalid shop list response packet");
        return nullptr;
    }

    return packet->packet_event_as_S2C_ShopList();
}

const S2C_ShopItems* ClientPacketManager::ParseShopItemsResponse(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_S2C_ShopItems) {
        SetError("Invalid shop items response packet");
        return nullptr;
    }

    return packet->packet_event_as_S2C_ShopItems();
}

const S2C_ShopTransaction* ClientPacketManager::ParseShopTransactionResponse(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_S2C_ShopTransaction) {
        SetError("Invalid shop transaction response packet");
        return nullptr;
    }

    return packet->packet_event_as_S2C_ShopTransaction();
}

// === 편의 함수들 ===

bool ClientPacketManager::IsLoginSuccess(const S2C_Login* response)
{
    return response && response->result() == ResultCode_SUCCESS;
}

bool ClientPacketManager::IsCreateAccountSuccess(const S2C_CreateAccount* response)
{
    return response && response->result() == ResultCode_SUCCESS;
}

bool ClientPacketManager::IsPlayerDataValid(const S2C_PlayerData* response)
{
    return response && response->result() == ResultCode_SUCCESS;
}

bool ClientPacketManager::IsItemDataValid(const S2C_ItemData* response)
{
    return response && response->result() == ResultCode_SUCCESS;
}

bool ClientPacketManager::IsShopListValid(const S2C_ShopList* response)
{
    return response && response->result() == ResultCode_SUCCESS;
}

bool ClientPacketManager::IsShopItemsValid(const S2C_ShopItems* response)
{
    return response && response->result() == ResultCode_SUCCESS;
}

bool ClientPacketManager::IsShopTransactionSuccess(const S2C_ShopTransaction* response)
{
    return response && response->result() == ResultCode_SUCCESS;
}

// === 유틸리티 함수들 ===

EventType ClientPacketManager::GetPacketType(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return EventType_NONE;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    return packet ? packet->packet_event_type() : EventType_NONE;
}

uint32_t ClientPacketManager::GetClientSocket(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return 0;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    return packet ? packet->client_socket() : 0;
}

bool ClientPacketManager::IsValidPacket(const uint8_t* data, size_t size)
{
    return VerifyPacket(data, size);
}


// === 게임 서버 관련 클라이언트 요청 패킷 생성 (C2S) ===

std::vector<uint8_t> ClientPacketManager::CreateGameServerRequest(uint32_t user_id, const std::string& server_name,
    const std::string& server_password, const std::string& server_ip, uint32_t server_port, uint32_t max_players)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto serverNameOffset = builder.CreateString(server_name);
        auto serverPasswordOffset = builder.CreateString(server_password);
        auto serverIpOffset = builder.CreateString(server_ip);

        auto gameServerRequest = CreateC2S_CreateGameServer(builder, user_id, serverNameOffset,
            serverPasswordOffset, serverIpOffset, server_port, max_players);
        auto packet = CreateDatabasePacket(builder, EventType_C2S_CreateGameServer, gameServerRequest.Union());

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateGameServerRequest failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ClientPacketManager::CreateGameServerListRequest(uint32_t request_type)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto gameServerListRequest = CreateC2S_GameServerList(builder, request_type);
        auto packet = CreateDatabasePacket(builder, EventType_C2S_GameServerList, gameServerListRequest.Union());

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateGameServerListRequest failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ClientPacketManager::CreateJoinGameServerRequest(uint32_t user_id, uint32_t server_id, const std::string& server_password)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto serverPasswordOffset = builder.CreateString(server_password);
        auto joinGameServerRequest = CreateC2S_JoinGameServer(builder, user_id, server_id, serverPasswordOffset);
        auto packet = CreateDatabasePacket(builder, EventType_C2S_JoinGameServer, joinGameServerRequest.Union());

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateJoinGameServerRequest failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ClientPacketManager::CreateCloseGameServerRequest(uint32_t user_id, uint32_t server_id)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto closeGameServerRequest = CreateC2S_CloseGameServer(builder, user_id, server_id);
        auto packet = CreateDatabasePacket(builder, EventType_C2S_CloseGameServer, closeGameServerRequest.Union());

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateCloseGameServerRequest failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ClientPacketManager::CreateSavePlayerDataRequest(uint32_t user_id, uint32_t level, uint32_t exp,
    uint32_t hp, uint32_t mp, uint32_t gold, float pos_x, float pos_y)
{
    ClearError();
    try {
        flatbuffers::FlatBufferBuilder builder;

        auto savePlayerDataRequest = CreateC2S_SavePlayerData(builder, user_id, level, exp, hp, mp, gold, pos_x, pos_y);
        auto packet = CreateDatabasePacket(builder, EventType_C2S_SavePlayerData, savePlayerDataRequest.Union());

        builder.Finish(packet);

        uint8_t* bufferPointer = builder.GetBufferPointer();
        size_t bufferSize = builder.GetSize();

        return std::vector<uint8_t>(bufferPointer, bufferPointer + bufferSize);
    }
    catch (const std::exception& e) {
        SetError("CreateSavePlayerDataRequest failed: " + std::string(e.what()));
        return std::vector<uint8_t>();
    }
}

// === 게임 서버 관련 서버 응답 패킷 파싱 (S2C) ===

const S2C_CreateGameServer* ClientPacketManager::ParseCreateGameServerResponse(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_S2C_CreateGameServer) {
        SetError("Invalid create game server response packet");
        return nullptr;
    }

    return packet->packet_event_as_S2C_CreateGameServer();
}

const S2C_GameServerList* ClientPacketManager::ParseGameServerListResponse(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_S2C_GameServerList) {
        SetError("Invalid game server list response packet");
        return nullptr;
    }

    return packet->packet_event_as_S2C_GameServerList();
}

const S2C_JoinGameServer* ClientPacketManager::ParseJoinGameServerResponse(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_S2C_JoinGameServer) {
        SetError("Invalid join game server response packet");
        return nullptr;
    }

    return packet->packet_event_as_S2C_JoinGameServer();
}

const S2C_CloseGameServer* ClientPacketManager::ParseCloseGameServerResponse(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_S2C_CloseGameServer) {
        SetError("Invalid close game server response packet");
        return nullptr;
    }

    return packet->packet_event_as_S2C_CloseGameServer();
}

const S2C_SavePlayerData* ClientPacketManager::ParseSavePlayerDataResponse(const uint8_t* data, size_t size)
{
    if (!VerifyPacket(data, size)) {
        return nullptr;
    }

    const DatabasePacket* packet = GetDatabasePacket(data);
    if (!packet || packet->packet_event_type() != EventType_S2C_SavePlayerData) {
        SetError("Invalid save player data response packet");
        return nullptr;
    }

    return packet->packet_event_as_S2C_SavePlayerData();
}

// === 게임 서버 관련 편의 함수들 ===

bool ClientPacketManager::IsCreateGameServerSuccess(const S2C_CreateGameServer* response)
{
    return response && response->result() == ResultCode_SUCCESS;
}

bool ClientPacketManager::IsGameServerListValid(const S2C_GameServerList* response)
{
    return response && response->result() == ResultCode_SUCCESS;
}

bool ClientPacketManager::IsJoinGameServerSuccess(const S2C_JoinGameServer* response)
{
    return response && response->result() == ResultCode_SUCCESS;
}

bool ClientPacketManager::IsCloseGameServerSuccess(const S2C_CloseGameServer* response)
{
    return response && response->result() == ResultCode_SUCCESS;
}

bool ClientPacketManager::IsSavePlayerDataSuccess(const S2C_SavePlayerData* response)
{
    return response && response->result() == ResultCode_SUCCESS;
}

// GetPacketTypeName에도 새로운 케이스들 추가

std::string ClientPacketManager::GetPacketTypeName(EventType packet_type)
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

std::string ClientPacketManager::GetResultCodeName(ResultCode result)
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