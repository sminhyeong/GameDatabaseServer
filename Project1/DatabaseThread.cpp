#define NOMINMAX

#include "DatabaseThread.h"
#include "LockFreeQueue.h"
#include "Packet.h"
#include "MySqlConnector.h"
#include "ServerPacketManager.h"
#include "UserEvent_generated.h"

#include <thread>
#include <sstream>
#include <iomanip>

DatabaseThread::DatabaseThread(LockFreeQueue<Task>* InRecvQueue, LockFreeQueue<DBResponse>* InSendQueue)
	: RecvQueue(InRecvQueue), SendQueue(InSendQueue), _is_running(false), _port(3306)
{
	_sql_connector = std::make_unique<MySqlConnector>();
	_packet_manager = std::make_unique<ServerPacketManager>();

	// 기본 연결 정보 설정
	_host = "127.0.0.1";
	_user = "root";
	_password = "1234";
	_database = "gamedb";
}

DatabaseThread::~DatabaseThread()
{
	Stop();
	if (_db_thread.joinable()) {
		_db_thread.join();
	}
}

void DatabaseThread::SetConnectionInfo(const std::string& host, const std::string& user,
	const std::string& password, const std::string& database, int port)
{
	_host = host;
	_user = user;
	_password = password;
	_database = database;
	_port = port;
}

bool DatabaseThread::ConnectDB()
{
	try {
		std::cout << "[DatabaseThread] 데이터베이스 연결 시도중..." << std::endl;

		bool result = _sql_connector->Init();
		if (result) {
			result = _sql_connector->Connect(_host, _user, _password, _port, _database);
		}

		if (result) {
			std::cout << "[DatabaseThread] 데이터베이스 연결 성공!" << std::endl;

			// 사용자 세션 테이블 확인 및 초기화
			if (!InitializeUserSessions()) {
				std::cerr << "[DatabaseThread] 사용자 세션 초기화 실패" << std::endl;
				return false;
			}

			// 연결 성공 시 스레드 시작
			_is_running = true;
			_db_thread = std::thread(&DatabaseThread::Run, this);
			return true;
		}
		else {
			std::cerr << "[DatabaseThread] 데이터베이스 연결 실패!" << std::endl;
			return false;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "[DatabaseThread] 예외 발생: " << e.what() << std::endl;
		return false;
	}
}

void DatabaseThread::Stop()
{
	std::cout << "[DatabaseThread] 정지 신호 전송..." << std::endl;
	DisconnectAllUsers();
	_is_running.store(false);
}

bool DatabaseThread::CheckDBConnection()
{
	return _sql_connector && _sql_connector->IsConnected();
}

bool DatabaseThread::ReconnectIfNeeded()
{
	if (!CheckDBConnection()) {
		std::cout << "[DatabaseThread] DB 연결 끊어짐, 재연결 시도..." << std::endl;

		_sql_connector.reset();
		_sql_connector = std::make_unique<MySqlConnector>();

		if (_sql_connector->Init() &&
			_sql_connector->Connect(_host, _user, _password, _port, _database)) {
			std::cout << "[DatabaseThread] DB 재연결 성공!" << std::endl;
			return true;
		}
		else {
			std::cerr << "[DatabaseThread] DB 재연결 실패!" << std::endl;
			return false;
		}
	}
	return true;
}

bool DatabaseThread::IsDBConnected() const
{
	return _sql_connector && _sql_connector->IsConnected();
}

std::string DatabaseThread::GetStatus() const
{
	std::stringstream ss;
	ss << "[DatabaseThread Status] Running: " << (_is_running.load() ? "YES" : "NO")
		<< ", DB Connected: " << (IsDBConnected() ? "YES" : "NO")
		<< ", Host: " << _host << ":" << _port
		<< ", Database: " << _database;
	return ss.str();
}

// === 간소화된 세션 관리 ===

bool DatabaseThread::InitializeUserSessions()
{
	if (!CheckDBConnection()) {
		return false;
	}

	try {
		// 모든 사용자를 오프라인으로 설정
		std::string resetQuery = "UPDATE user_sessions SET is_online = FALSE, client_socket = 0, last_activity = NOW()";
		if (_sql_connector->ExecuteQuery(resetQuery)) {
			int affected = _sql_connector->GetAffectedRows();
			std::cout << "[DatabaseThread] " << affected << "개의 기존 세션을 오프라인으로 초기화했습니다." << std::endl;
			return true;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "[DatabaseThread] 세션 초기화 실패: " << e.what() << std::endl;
	}
	return false;
}

bool DatabaseThread::SetUserOnlineStatus(uint32_t user_id, bool is_online)
{
	if (!CheckDBConnection()) {
		return false;
	}

	try {
		std::stringstream query;
		if (is_online) {
			query << "INSERT INTO user_sessions (user_id, is_online, login_time, last_activity, client_socket) "
				<< "VALUES (" << user_id << ", 1, NOW(), NOW(), 0) "
				<< "ON DUPLICATE KEY UPDATE is_online = 1, login_time = NOW(), last_activity = NOW()";
		}
		else {
			query << "UPDATE user_sessions SET is_online = 0, last_activity = NOW(), client_socket = 0 "
				<< "WHERE user_id = " << user_id;
		}

		return _sql_connector->ExecuteQuery(query.str());
	}
	catch (const std::exception& e) {
		std::cerr << "[DatabaseThread] SetUserOnlineStatus 예외: " << e.what() << std::endl;
		return false;
	}
}

bool DatabaseThread::ForceLogoutExistingSession(uint32_t user_id)
{
	try {
		std::cout << "[DatabaseThread] 기존 세션 강제 종료 처리: 사용자 ID " << user_id << std::endl;

		// 단순히 DB에서 해당 사용자를 오프라인으로 설정
		std::stringstream logoutQuery;
		logoutQuery << "UPDATE user_sessions SET "
			<< "is_online = FALSE, "
			<< "client_socket = 0, "
			<< "last_activity = NOW() "
			<< "WHERE user_id = " << user_id << " AND is_online = TRUE";

		if (_sql_connector->ExecuteQuery(logoutQuery.str())) {
			int affected_rows = _sql_connector->GetAffectedRows();

			if (affected_rows > 0) {
				std::cout << "[DatabaseThread] 기존 세션 강제 종료 성공: " << affected_rows << "개 세션 처리" << std::endl;
				return true;
			}
			else {
				std::cout << "[DatabaseThread] 처리할 활성 세션 없음: 사용자 ID " << user_id << std::endl;
				return true; // 이미 오프라인이므로 성공으로 처리
			}
		}
		else {
			std::cerr << "[DatabaseThread] 기존 세션 강제 종료 쿼리 실패" << std::endl;
			return false;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "[DatabaseThread] 기존 세션 강제 종료 중 예외: " << e.what() << std::endl;
		return false;
	}
}

void DatabaseThread::DisconnectAllUsers()
{
	if (!CheckDBConnection()) return;

	try {
		std::string query = "UPDATE user_sessions SET is_online = FALSE, client_socket = 0, last_activity = NOW()";
		if (_sql_connector->ExecuteQuery(query)) {
			int affected = _sql_connector->GetAffectedRows();
			std::cout << "[DatabaseThread] 서버 종료 - " << affected << "명의 사용자를 오프라인으로 설정했습니다." << std::endl;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "[DatabaseThread] DisconnectAllUsers 예외: " << e.what() << std::endl;
	}
}

void DatabaseThread::DisconnectUser(uint32_t user_id)
{
	SetUserOnlineStatus(user_id, false);
}

// === 스레드 실행 및 태스크 처리 ===

void DatabaseThread::Run()
{
	std::cout << "[DatabaseThread] DB 처리 스레드 시작" << std::endl;

	auto last_connection_check = std::chrono::steady_clock::now();
	const auto connection_check_interval = std::chrono::seconds(30);

	while (_is_running.load()) {
		// 주기적으로 DB 연결 상태 체크
		auto now = std::chrono::steady_clock::now();
		if (now - last_connection_check >= connection_check_interval) {
			if (!ReconnectIfNeeded()) {
				std::this_thread::sleep_for(std::chrono::seconds(5));
				continue;
			}
			last_connection_check = now;
		}

		// 큐에서 태스크 처리
		Task task;
		if (RecvQueue->dequeue(task)) {
			try {
				ProcessTask(task);
			}
			catch (const std::exception& e) {
				std::cerr << "[DatabaseThread] 태스크 처리 중 예외 발생: " << e.what() << std::endl;
				SendErrorResponse(task, EventType_NONE, ResultCode_FAIL);
			}
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	std::cout << "[DatabaseThread] DB 처리 스레드 종료" << std::endl;
}

void DatabaseThread::ProcessTask(const Task& task)
{
	if (!task.query.empty() && task.query.find("FORCE_LOGOUT:") == 0) {
		return;
	}

	if (task.flatbuffer_data.empty()) {
		std::cerr << "[DatabaseThread] 빈 FlatBuffer 데이터" << std::endl;
		return;
	}

	if (!CheckDBConnection() && !ReconnectIfNeeded()) {
		std::cerr << "[DatabaseThread] DB 연결 실패, 태스크 스킵" << std::endl;
		return;
	}

	if (!_packet_manager->IsValidPacket(task.flatbuffer_data.data(), task.flatbuffer_data.size())) {
		std::cerr << "[DatabaseThread] 잘못된 패킷: " << _packet_manager->GetLastError() << std::endl;
		return;
	}

	EventType packetType = _packet_manager->GetPacketType(task.flatbuffer_data.data(), task.flatbuffer_data.size());
	std::cout << "[DatabaseThread] 처리 중인 패킷: " << _packet_manager->GetPacketTypeName(packetType) << std::endl;

	switch (packetType) {
	case EventType_C2S_Login:
		HandleLoginRequest(task);
		break;
	case EventType_C2S_Logout:
		HandleLogoutRequest(task);
		break;
	case EventType_C2S_CreateAccount:
		HandleCreateAccountRequest(task);
		break;
	case EventType_C2S_PlayerData:
		HandlePlayerDataRequest(task);
		break;
	case EventType_C2S_ItemData:
		HandleItemDataRequest(task);
		break;
	case EventType_C2S_MonsterData:
		HandleMonsterDataRequest(task);
		break;
	case EventType_C2S_PlayerChat:
		HandlePlayerChatRequest(task);
		break;
	case EventType_C2S_ShopList:
		HandleShopListRequest(task);
		break;
	case EventType_C2S_ShopItems:
		HandleShopItemsRequest(task);
		break;
	case EventType_C2S_ShopTransaction:
		HandleShopTransactionRequest(task);
		break;
	default:
		std::cout << "[DatabaseThread] 처리되지 않은 패킷 타입: " << static_cast<int>(packetType) << std::endl;
		break;
	}
}

// === 간소화된 핸들러들 ===

void DatabaseThread::HandleLoginRequest(const Task& task)
{
	const C2S_Login* loginReq = _packet_manager->ParseLoginRequest(task.flatbuffer_data.data(), task.flatbuffer_data.size());

	if (!loginReq || !_packet_manager->ValidateLoginRequest(loginReq)) {
		std::cerr << "[DatabaseThread] 로그인 요청 검증 실패: " << _packet_manager->GetLastError() << std::endl;
		SendErrorResponse(task, EventType_S2C_Login, ResultCode_INVALID_USER);
		return;
	}

	std::cout << "[DatabaseThread] 로그인 요청 처리: " << loginReq->username()->c_str() << std::endl;

	// 간소화된 로그인 처리: 인증과 중복 로그인 체크를 한 번에
	std::stringstream query;
	query << "SELECT u.user_id, u.nickname, COALESCE(p.level, 1) as level, "
		<< "COALESCE(s.is_online, 0) as is_online "
		<< "FROM users u "
		<< "LEFT JOIN player_data p ON u.user_id = p.user_id "
		<< "LEFT JOIN user_sessions s ON u.user_id = s.user_id "
		<< "WHERE u.username = '" << loginReq->username()->c_str()
		<< "' AND u.password = '" << loginReq->password()->c_str()
		<< "' AND u.is_active = 1";

	if (!_sql_connector->ExecuteQuery(query.str())) {
		std::cerr << "[DatabaseThread] 로그인 쿼리 실행 실패" << std::endl;
		SendErrorResponse(task, EventType_S2C_Login, ResultCode_FAIL);
		return;
	}

	MYSQL_RES* result = _sql_connector->GetResult();
	if (!result) {
		SendErrorResponse(task, EventType_S2C_Login, ResultCode_FAIL);
		return;
	}

	MYSQL_ROW row = mysql_fetch_row(result);
	if (!row) {
		_sql_connector->FreeResult(result);
		std::cout << "[DatabaseThread] 잘못된 사용자명 또는 비밀번호" << std::endl;
		SendErrorResponse(task, EventType_S2C_Login, ResultCode_INVALID_USER);
		return;
	}

	uint32_t user_id = std::stoul(row[0]);
	std::string nickname = row[1] ? row[1] : "";
	uint32_t level = std::stoul(row[2]);
	bool is_online = row[3] && std::string(row[3]) == "1";

	_sql_connector->FreeResult(result);

	// 중복 로그인 시 기존 세션 강제 종료 + 새 로그인 차단
	if (is_online) {
		std::cout << "[DatabaseThread] 중복 로그인 감지: 사용자 ID " << user_id << std::endl;
		std::cout << "[DatabaseThread] → 기존 세션 강제 종료 처리" << std::endl;
		std::cout << "[DatabaseThread] → 새 로그인 시도 차단" << std::endl;

		// 기존 세션을 강제 종료
		if (ForceLogoutExistingSession(user_id)) {
			std::cout << "[DatabaseThread] 기존 세션 강제 종료 완료" << std::endl;
		}
		else {
			std::cerr << "[DatabaseThread] 기존 세션 강제 종료 실패" << std::endl;
		}

		// 새 로그인 시도를 차단
		auto responsePacket = _packet_manager->CreateLoginErrorResponse(
			ResultCode_FAIL, task.client_socket);
		SendResponse(task, responsePacket);

		std::cout << "[DatabaseThread] 중복 로그인 차단 완료: " << loginReq->username()->c_str() << std::endl;
		return;
	}

	// 새로운 로그인 처리 (간소화됨)
	if (SetUserOnlineStatus(user_id, true)) {
		auto responsePacket = _packet_manager->CreateLoginResponse(
			ResultCode_SUCCESS, user_id, loginReq->username()->str(), nickname, level, task.client_socket);
		SendResponse(task, responsePacket);

		std::cout << "[DatabaseThread] 로그인 성공: " << loginReq->username()->c_str() << std::endl;
	}
	else {
		std::cerr << "[DatabaseThread] 온라인 상태 설정 실패" << std::endl;
		SendErrorResponse(task, EventType_S2C_Login, ResultCode_FAIL);
	}
}

void DatabaseThread::HandleLogoutRequest(const Task& task)
{
	std::cout << "[DatabaseThread] =================== 로그아웃 요청 시작 ===================" << std::endl;

	const C2S_Logout* logoutReq = _packet_manager->ParseLogoutRequest(task.flatbuffer_data.data(), task.flatbuffer_data.size());

	if (!logoutReq) {
		std::cerr << "[DatabaseThread] 로그아웃 요청 파싱 실패" << std::endl;
		SendErrorResponse(task, EventType_S2C_Logout, ResultCode_FAIL);
		return;
	}

	uint32_t user_id = logoutReq->user_id();
	std::cout << "[DatabaseThread] 로그아웃 요청 처리: 사용자 ID " << user_id << std::endl;
	std::cout << "[DatabaseThread] 클라이언트 소켓: " << task.client_socket << std::endl;

	// 사용자 오프라인 상태로 설정
	bool offline_result = SetUserOnlineStatus(user_id, false);
	std::cout << "[DatabaseThread] 오프라인 상태 설정 결과: " << (offline_result ? "성공" : "실패") << std::endl;

	// 항상 성공 응답 생성
	std::cout << "[DatabaseThread] 로그아웃 응답 패킷 생성 중..." << std::endl;
	auto responsePacket = _packet_manager->CreateLogoutResponse(
		ResultCode_SUCCESS, "로그아웃 완료", task.client_socket);

	if (responsePacket.empty()) {
		std::cerr << "[DatabaseThread] 로그아웃 응답 패킷 생성 실패!" << std::endl;
		return;
	}

	std::cout << "[DatabaseThread] 로그아웃 응답 패킷 크기: " << responsePacket.size() << " bytes" << std::endl;

	// 응답 전송
	SendResponse(task, responsePacket);

	std::cout << "[DatabaseThread] 로그아웃 처리 완료: 사용자 ID " << user_id << std::endl;
	std::cout << "[DatabaseThread] =================== 로그아웃 요청 종료 ===================" << std::endl;
}

void DatabaseThread::HandleCreateAccountRequest(const Task& task)
{
	const C2S_CreateAccount* accountReq = _packet_manager->ParseCreateAccountRequest(task.flatbuffer_data.data(), task.flatbuffer_data.size());

	if (!accountReq || !_packet_manager->ValidateCreateAccountRequest(accountReq)) {
		SendErrorResponse(task, EventType_S2C_CreateAccount, ResultCode_INVALID_USER);
		return;
	}

	std::cout << "[DatabaseThread] 계정 생성 요청 처리: " << accountReq->username()->c_str() << std::endl;

	// 중복 사용자명 확인과 계정 생성을 한 번에 처리
	std::stringstream query;
	query << "INSERT INTO users (username, password, nickname, created_at) "
		<< "SELECT '" << accountReq->username()->c_str() << "', '"
		<< accountReq->password()->c_str() << "', '"
		<< accountReq->nickname()->c_str() << "', NOW() "
		<< "WHERE NOT EXISTS (SELECT 1 FROM users WHERE username = '" << accountReq->username()->c_str() << "')";

	if (_sql_connector->ExecuteQuery(query.str()) && _sql_connector->GetAffectedRows() > 0) {
		// 생성된 user_id 가져오기
		std::string getIdQuery = "SELECT LAST_INSERT_ID()";
		if (_sql_connector->ExecuteQuery(getIdQuery)) {
			MYSQL_RES* result = _sql_connector->GetResult();
			if (result) {
				MYSQL_ROW row = mysql_fetch_row(result);
				if (row) {
					uint32_t new_user_id = std::stoul(row[0]);
					_sql_connector->FreeResult(result);

					// 기본 플레이어 데이터 생성
					CreateDefaultPlayerData(new_user_id);

					auto responsePacket = _packet_manager->CreateAccountResponse(
						ResultCode_SUCCESS, new_user_id, "계정 생성 성공", task.client_socket);
					SendResponse(task, responsePacket);

					std::cout << "[DatabaseThread] 계정 생성 성공: " << accountReq->username()->c_str()
						<< " (ID: " << new_user_id << ")" << std::endl;
					return;
				}
				_sql_connector->FreeResult(result);
			}
		}
	}

	auto responsePacket = _packet_manager->CreateAccountErrorResponse(
		ResultCode_FAIL, "이미 존재하는 사용자명이거나 계정 생성에 실패했습니다", task.client_socket);
	SendResponse(task, responsePacket);
}

void DatabaseThread::CreateDefaultPlayerData(uint32_t user_id)
{
	// 기본 플레이어 데이터 생성
	std::stringstream playerInsert;
	playerInsert << "INSERT INTO player_data (user_id, level, exp, hp, mp, attack, defense, gold, map_id, pos_x, pos_y) "
		<< "VALUES (" << user_id << ", 1, 0, 100, 50, 10, 5, 1000, 1, 0.0, 0.0)";
	_sql_connector->ExecuteQuery(playerInsert.str());

	// 기본 아이템 지급
	std::stringstream itemInsert;
	itemInsert << "INSERT INTO player_inventory (user_id, item_id, item_count, acquired_at) VALUES "
		<< "(" << user_id << ", 1, 1, NOW()), "
		<< "(" << user_id << ", 3, 1, NOW())";
	_sql_connector->ExecuteQuery(itemInsert.str());

	// 사용자 세션 초기 생성
	std::stringstream sessionInsert;
	sessionInsert << "INSERT INTO user_sessions (user_id, is_online, login_time, last_activity, client_socket) "
		<< "VALUES (" << user_id << ", FALSE, NOW(), NOW(), 0)";
	_sql_connector->ExecuteQuery(sessionInsert.str());
}

void DatabaseThread::HandlePlayerDataRequest(const Task& task)
{
	const C2S_PlayerData* playerReq = _packet_manager->ParsePlayerDataRequest(task.flatbuffer_data.data(), task.flatbuffer_data.size());

	if (!playerReq || !_packet_manager->ValidatePlayerDataRequest(playerReq)) {
		SendErrorResponse(task, EventType_S2C_PlayerData, ResultCode_INVALID_USER);
		return;
	}

	if (playerReq->request_type() == 0) {
		// 조회
		std::stringstream query;
		query << "SELECT u.username, u.nickname, p.level, p.exp, p.hp, p.mp, p.attack, p.defense, p.gold, p.map_id, p.pos_x, p.pos_y "
			<< "FROM users u JOIN player_data p ON u.user_id = p.user_id "
			<< "WHERE u.user_id = " << playerReq->user_id() << " AND u.is_active = 1";

		if (_sql_connector->ExecuteQuery(query.str())) {
			MYSQL_RES* result = _sql_connector->GetResult();
			if (result) {
				auto responsePacket = _packet_manager->CreatePlayerDataResponseFromDB(result, playerReq->user_id(), task.client_socket);
				SendResponse(task, responsePacket);
				return;
			}
		}
		SendErrorResponse(task, EventType_S2C_PlayerData, ResultCode_USER_NOT_FOUND);
	}
	else if (playerReq->request_type() == 1) {
		// 업데이트
		std::stringstream query;
		query << "UPDATE player_data SET "
			<< "level = " << playerReq->level() << ", "
			<< "exp = " << playerReq->exp() << ", "
			<< "hp = " << playerReq->hp() << ", "
			<< "mp = " << playerReq->mp() << ", "
			<< "pos_x = " << playerReq->pos_x() << ", "
			<< "pos_y = " << playerReq->pos_y() << ", "
			<< "updated_at = NOW() "
			<< "WHERE user_id = " << playerReq->user_id();

		if (_sql_connector->ExecuteQuery(query.str())) {
			auto responsePacket = _packet_manager->CreatePlayerDataResponse(
				ResultCode_SUCCESS, playerReq->user_id(), "", "",
				playerReq->level(), playerReq->exp(), playerReq->hp(),
				playerReq->mp(), 0, 0, 0, 0, playerReq->pos_x(),
				playerReq->pos_y(), task.client_socket);
			SendResponse(task, responsePacket);
			std::cout << "[DatabaseThread] 플레이어 데이터 업데이트 완료: 사용자 ID " << playerReq->user_id() << std::endl;
		}
		else {
			SendErrorResponse(task, EventType_S2C_PlayerData, ResultCode_FAIL);
		}
	}
}

void DatabaseThread::HandleItemDataRequest(const Task& task)
{
	const C2S_ItemData* itemReq = _packet_manager->ParseItemDataRequest(task.flatbuffer_data.data(), task.flatbuffer_data.size());

	if (!itemReq || !_packet_manager->ValidateItemDataRequest(itemReq)) {
		SendErrorResponse(task, EventType_S2C_ItemData, ResultCode_INVALID_USER);
		return;
	}

	if (itemReq->request_type() == 0) {
		// 인벤토리 조회
		std::stringstream query;
		query << "SELECT i.item_id, m.item_name, i.item_count, m.item_type, m.base_price, "
			<< "m.attack_bonus, m.defense_bonus, m.hp_bonus, m.mp_bonus, m.description, p.gold "
			<< "FROM player_inventory i "
			<< "JOIN item_master m ON i.item_id = m.item_id "
			<< "JOIN player_data p ON i.user_id = p.user_id "
			<< "WHERE i.user_id = " << itemReq->user_id()
			<< " ORDER BY i.item_id";

		if (_sql_connector->ExecuteQuery(query.str())) {
			MYSQL_RES* result = _sql_connector->GetResult();
			if (result) {
				auto responsePacket = _packet_manager->CreateItemDataResponseFromDB(result, itemReq->user_id(), task.client_socket);
				SendResponse(task, responsePacket);
				return;
			}
		}
		// 인벤토리가 비어있는 경우
		auto responsePacket = _packet_manager->CreateItemDataResponse(ResultCode_SUCCESS, itemReq->user_id(), 0, task.client_socket);
		SendResponse(task, responsePacket);
	}
	else {
		// 아이템 추가/제거 처리는 기존과 동일하게 유지
		HandleItemModification(task, itemReq);
	}
}

void DatabaseThread::HandleItemModification(const Task& task, const C2S_ItemData* itemReq)
{
	std::stringstream query;

	if (itemReq->request_type() == 1) {
		// 아이템 추가
		query << "INSERT INTO player_inventory (user_id, item_id, item_count, acquired_at) VALUES ("
			<< itemReq->user_id() << ", " << itemReq->item_id() << ", "
			<< itemReq->item_count() << ", NOW()) "
			<< "ON DUPLICATE KEY UPDATE item_count = item_count + " << itemReq->item_count();
	}
	else if (itemReq->request_type() == 2) {
		// 아이템 제거
		query << "UPDATE player_inventory SET item_count = GREATEST(0, item_count - " << itemReq->item_count() << ") "
			<< "WHERE user_id = " << itemReq->user_id()
			<< " AND item_id = " << itemReq->item_id()
			<< "; DELETE FROM player_inventory WHERE user_id = " << itemReq->user_id()
			<< " AND item_id = " << itemReq->item_id() << " AND item_count <= 0";
	}

	if (_sql_connector->ExecuteQuery(query.str())) {
		auto responsePacket = _packet_manager->CreateItemDataResponse(ResultCode_SUCCESS, itemReq->user_id(), 0, task.client_socket);
		SendResponse(task, responsePacket);
		std::cout << "[DatabaseThread] 아이템 수정 완료: 사용자 ID " << itemReq->user_id() << std::endl;
	}
	else {
		SendErrorResponse(task, EventType_S2C_ItemData, ResultCode_FAIL);
	}
}

void DatabaseThread::HandleMonsterDataRequest(const Task& task)
{
	const C2S_MonsterData* monsterReq = _packet_manager->ParseMonsterDataRequest(task.flatbuffer_data.data(), task.flatbuffer_data.size());

	if (!monsterReq) {
		SendErrorResponse(task, EventType_S2C_MonsterData, ResultCode_FAIL);
		return;
	}

	if (monsterReq->request_type() == 0) {
		std::string query = "SELECT monster_id, monster_name, level, hp, attack, defense, exp_reward, gold_reward "
			"FROM monster_master ORDER BY level, monster_id";

		if (_sql_connector->ExecuteQuery(query)) {
			MYSQL_RES* result = _sql_connector->GetResult();
			if (result) {
				auto responsePacket = _packet_manager->CreateMonsterDataResponseFromDB(result, task.client_socket);
				SendResponse(task, responsePacket);
				return;
			}
		}
	}
	SendErrorResponse(task, EventType_S2C_MonsterData, ResultCode_FAIL);
}

void DatabaseThread::HandlePlayerChatRequest(const Task& task)
{
	const C2S_PlayerChat* chatReq = _packet_manager->ParsePlayerChatRequest(task.flatbuffer_data.data(), task.flatbuffer_data.size());

	if (!chatReq) {
		SendErrorResponse(task, EventType_S2C_PlayerChat, ResultCode_FAIL);
		return;
	}

	if (chatReq->request_type() == 0) {
		// 채팅 로그 조회
		std::stringstream query;
		query << "SELECT c.chat_id, c.sender_id, u.nickname, c.message, c.chat_type, UNIX_TIMESTAMP(c.timestamp) "
			<< "FROM chat_logs c "
			<< "JOIN users u ON c.sender_id = u.user_id "
			<< "WHERE c.chat_type = " << chatReq->chat_type();

		if (chatReq->receiver_id() != 0) {
			query << " AND (c.receiver_id = " << chatReq->receiver_id()
				<< " OR c.sender_id = " << chatReq->receiver_id() << ")";
		}

		query << " ORDER BY c.timestamp DESC LIMIT 50";

		if (_sql_connector->ExecuteQuery(query.str())) {
			MYSQL_RES* result = _sql_connector->GetResult();
			if (result) {
				auto responsePacket = _packet_manager->CreatePlayerChatResponseFromDB(result, task.client_socket);
				SendResponse(task, responsePacket);
				return;
			}
		}
	}
	else if (chatReq->request_type() == 1) {
		// 채팅 메시지 저장
		std::stringstream insertQuery;
		insertQuery << "INSERT INTO chat_logs (sender_id, receiver_id, message, chat_type, timestamp) VALUES ("
			<< chatReq->sender_id() << ", ";

		if (chatReq->receiver_id() == 0) {
			insertQuery << "NULL, ";
		}
		else {
			insertQuery << chatReq->receiver_id() << ", ";
		}

		// 간단한 이스케이프 처리
		std::string escaped_message = chatReq->message()->str();
		size_t pos = 0;
		while ((pos = escaped_message.find("'", pos)) != std::string::npos) {
			escaped_message.replace(pos, 1, "\\'");
			pos += 2;
		}

		insertQuery << "'" << escaped_message << "', " << chatReq->chat_type() << ", NOW())";

		if (_sql_connector->ExecuteQuery(insertQuery.str())) {
			auto responsePacket = _packet_manager->CreatePlayerChatResponse(ResultCode_SUCCESS, task.client_socket);
			SendResponse(task, responsePacket);
			std::cout << "[DatabaseThread] 채팅 메시지 저장 완료 - 발신자: " << chatReq->sender_id() << std::endl;
			return;
		}
	}
	SendErrorResponse(task, EventType_S2C_PlayerChat, ResultCode_FAIL);
}

void DatabaseThread::HandleShopListRequest(const Task& task)
{
	const C2S_ShopList* shopReq = _packet_manager->ParseShopListRequest(task.flatbuffer_data.data(), task.flatbuffer_data.size());

	if (!shopReq || !_packet_manager->ValidateShopListRequest(shopReq)) {
		SendErrorResponse(task, EventType_S2C_ShopList, ResultCode_FAIL);
		return;
	}

	std::stringstream query;
	query << "SELECT shop_id, shop_name, shop_type, map_id, pos_x, pos_y "
		<< "FROM shop_master WHERE is_active = 1";

	if (shopReq->map_id() != 0) {
		query << " AND map_id = " << shopReq->map_id();
	}

	query << " ORDER BY shop_id";

	if (_sql_connector->ExecuteQuery(query.str())) {
		MYSQL_RES* result = _sql_connector->GetResult();
		if (result) {
			auto responsePacket = _packet_manager->CreateShopListResponseFromDB(result, task.client_socket);
			SendResponse(task, responsePacket);
			return;
		}
	}
	SendErrorResponse(task, EventType_S2C_ShopList, ResultCode_SHOP_NOT_FOUND);
}

void DatabaseThread::HandleShopItemsRequest(const Task& task)
{
	const C2S_ShopItems* shopItemsReq = _packet_manager->ParseShopItemsRequest(task.flatbuffer_data.data(), task.flatbuffer_data.size());

	if (!shopItemsReq || !_packet_manager->ValidateShopItemsRequest(shopItemsReq)) {
		SendErrorResponse(task, EventType_S2C_ShopItems, ResultCode_FAIL);
		return;
	}

	std::stringstream query;
	query << "SELECT i.item_id, i.item_name, i.item_type, i.base_price, "
		<< "i.attack_bonus, i.defense_bonus, i.hp_bonus, i.mp_bonus, i.description "
		<< "FROM shop_items s "
		<< "JOIN item_master i ON s.item_id = i.item_id "
		<< "WHERE s.shop_id = " << shopItemsReq->shop_id()
		<< " ORDER BY i.item_id";

	if (_sql_connector->ExecuteQuery(query.str())) {
		MYSQL_RES* result = _sql_connector->GetResult();
		if (result) {
			auto responsePacket = _packet_manager->CreateShopItemsResponseFromDB(result, shopItemsReq->shop_id(), task.client_socket);
			SendResponse(task, responsePacket);
			return;
		}
	}
	SendErrorResponse(task, EventType_S2C_ShopItems, ResultCode_ITEM_NOT_FOUND);
}

void DatabaseThread::HandleShopTransactionRequest(const Task& task)
{
	const C2S_ShopTransaction* transReq = _packet_manager->ParseShopTransactionRequest(task.flatbuffer_data.data(), task.flatbuffer_data.size());

	if (!transReq || !_packet_manager->ValidateShopTransactionRequest(transReq)) {
		SendErrorResponse(task, EventType_S2C_ShopTransaction, ResultCode_FAIL);
		return;
	}

	if (transReq->transaction_type() == 0) {
		HandleShopPurchase(task, transReq);
	}
	else if (transReq->transaction_type() == 1) {
		HandleShopSell(task, transReq);
	}
}

void DatabaseThread::HandleShopPurchase(const Task& task, const C2S_ShopTransaction* transReq)
{
	// 아이템 가격과 플레이어 골드를 한 번에 조회
	std::stringstream query;
	query << "SELECT i.base_price, p.gold FROM item_master i, player_data p "
		<< "WHERE i.item_id = " << transReq->item_id()
		<< " AND p.user_id = " << transReq->user_id();

	if (!_sql_connector->ExecuteQuery(query.str())) {
		SendErrorResponse(task, EventType_S2C_ShopTransaction, ResultCode_FAIL);
		return;
	}

	MYSQL_RES* result = _sql_connector->GetResult();
	if (!result) {
		SendErrorResponse(task, EventType_S2C_ShopTransaction, ResultCode_ITEM_NOT_FOUND);
		return;
	}

	MYSQL_ROW row = mysql_fetch_row(result);
	if (!row) {
		_sql_connector->FreeResult(result);
		SendErrorResponse(task, EventType_S2C_ShopTransaction, ResultCode_ITEM_NOT_FOUND);
		return;
	}

	uint32_t item_price = std::stoul(row[0]);
	uint32_t current_gold = std::stoul(row[1]);
	uint32_t total_price = item_price * transReq->item_count();

	_sql_connector->FreeResult(result);

	if (current_gold < total_price) {
		auto responsePacket = _packet_manager->CreateShopTransactionErrorResponse(
			ResultCode_INSUFFICIENT_GOLD, "골드가 부족합니다", task.client_socket);
		SendResponse(task, responsePacket);
		return;
	}

	// 트랜잭션으로 골드 차감과 아이템 추가를 한 번에 처리
	std::stringstream transactionQuery;
	transactionQuery << "START TRANSACTION; "
		<< "UPDATE player_data SET gold = gold - " << total_price << " WHERE user_id = " << transReq->user_id() << "; "
		<< "INSERT INTO player_inventory (user_id, item_id, item_count, acquired_at) VALUES ("
		<< transReq->user_id() << ", " << transReq->item_id() << ", "
		<< transReq->item_count() << ", NOW()) "
		<< "ON DUPLICATE KEY UPDATE item_count = item_count + " << transReq->item_count() << "; "
		<< "COMMIT;";

	if (_sql_connector->ExecuteQuery(transactionQuery.str())) {
		uint32_t new_gold = current_gold - total_price;
		auto responsePacket = _packet_manager->CreateShopTransactionResponse(
			ResultCode_SUCCESS, "구매 완료", new_gold, task.client_socket);
		SendResponse(task, responsePacket);
		std::cout << "[DatabaseThread] 아이템 구매 완료: 사용자 ID " << transReq->user_id() << std::endl;
	}
	else {
		SendErrorResponse(task, EventType_S2C_ShopTransaction, ResultCode_FAIL);
	}
}

void DatabaseThread::HandleShopSell(const Task& task, const C2S_ShopTransaction* transReq)
{
	// 플레이어가 보유한 아이템 수량과 아이템 가격을 한 번에 조회
	std::stringstream query;
	query << "SELECT i.item_count, m.base_price FROM player_inventory i "
		<< "JOIN item_master m ON i.item_id = m.item_id "
		<< "WHERE i.user_id = " << transReq->user_id()
		<< " AND i.item_id = " << transReq->item_id();

	if (!_sql_connector->ExecuteQuery(query.str())) {
		SendErrorResponse(task, EventType_S2C_ShopTransaction, ResultCode_FAIL);
		return;
	}

	MYSQL_RES* result = _sql_connector->GetResult();
	if (!result) {
		auto responsePacket = _packet_manager->CreateShopTransactionErrorResponse(
			ResultCode_ITEM_NOT_FOUND, "아이템을 보유하고 있지 않습니다", task.client_socket);
		SendResponse(task, responsePacket);
		return;
	}

	MYSQL_ROW row = mysql_fetch_row(result);
	if (!row) {
		_sql_connector->FreeResult(result);
		auto responsePacket = _packet_manager->CreateShopTransactionErrorResponse(
			ResultCode_ITEM_NOT_FOUND, "아이템을 보유하고 있지 않습니다", task.client_socket);
		SendResponse(task, responsePacket);
		return;
	}

	uint32_t owned_count = std::stoul(row[0]);
	uint32_t item_price = std::stoul(row[1]);
	uint32_t sell_price = (item_price / 2) * transReq->item_count();

	_sql_connector->FreeResult(result);

	if (owned_count < transReq->item_count()) {
		auto responsePacket = _packet_manager->CreateShopTransactionErrorResponse(
			ResultCode_ITEM_NOT_FOUND, "보유 아이템이 부족합니다", task.client_socket);
		SendResponse(task, responsePacket);
		return;
	}

	// 트랜잭션으로 아이템 제거와 골드 추가를 한 번에 처리
	std::stringstream transactionQuery;
	transactionQuery << "START TRANSACTION; "
		<< "UPDATE player_inventory SET item_count = item_count - " << transReq->item_count()
		<< " WHERE user_id = " << transReq->user_id() << " AND item_id = " << transReq->item_id() << "; "
		<< "DELETE FROM player_inventory WHERE user_id = " << transReq->user_id()
		<< " AND item_id = " << transReq->item_id() << " AND item_count <= 0; "
		<< "UPDATE player_data SET gold = gold + " << sell_price
		<< " WHERE user_id = " << transReq->user_id() << "; "
		<< "COMMIT;";

	if (_sql_connector->ExecuteQuery(transactionQuery.str())) {
		// 현재 골드 조회
		std::string goldQuery = "SELECT gold FROM player_data WHERE user_id = " + std::to_string(transReq->user_id());
		if (_sql_connector->ExecuteQuery(goldQuery)) {
			MYSQL_RES* goldResult = _sql_connector->GetResult();
			if (goldResult) {
				MYSQL_ROW goldRow = mysql_fetch_row(goldResult);
				if (goldRow) {
					uint32_t current_gold = std::stoul(goldRow[0]);
					_sql_connector->FreeResult(goldResult);

					auto responsePacket = _packet_manager->CreateShopTransactionResponse(
						ResultCode_SUCCESS, "판매 완료", current_gold, task.client_socket);
					SendResponse(task, responsePacket);
					std::cout << "[DatabaseThread] 아이템 판매 완료: 사용자 ID " << transReq->user_id() << std::endl;
					return;
				}
				_sql_connector->FreeResult(goldResult);
			}
		}
	}
	SendErrorResponse(task, EventType_S2C_ShopTransaction, ResultCode_FAIL);
}

void DatabaseThread::SendResponse(const Task& task, const std::vector<uint8_t>& responsePacket)
{
	if (responsePacket.empty()) {
		std::cerr << "[DatabaseThread] 빈 응답 패킷" << std::endl;
		return;
	}

	DBResponse response;
	response.client_socket = task.client_socket;
	response.worker_thread_id = task.worker_thread_id;
	response.task_id = task.id;
	response.success = true;
	response.response_data = responsePacket;

	SendQueue->enqueue(response);
	std::cout << "[DatabaseThread] 응답 전송 완료 - 클라이언트: " << task.client_socket
		<< ", 패킷 크기: " << responsePacket.size() << " bytes" << std::endl;
}

void DatabaseThread::SendErrorResponse(const Task& task, EventType responseType, ResultCode errorCode)
{
	auto errorPacket = _packet_manager->CreateGenericErrorResponse(responseType, errorCode, task.client_socket);

	DBResponse response;
	response.client_socket = task.client_socket;
	response.worker_thread_id = task.worker_thread_id;
	response.task_id = task.id;
	response.success = false;
	response.error_message = _packet_manager->GetResultCodeName(errorCode);
	response.response_data = errorPacket;

	SendQueue->enqueue(response);
	std::cout << "[DatabaseThread] 에러 응답 전송 - 클라이언트: " << task.client_socket
		<< ", 에러 코드: " << _packet_manager->GetResultCodeName(errorCode) << std::endl;
}

void DatabaseThread::ShowSessionDebugInfo()
{
	if (!CheckDBConnection()) return;

	try {
		std::string query = R"(
			SELECT s.user_id, u.username, s.is_online, s.login_time, s.last_activity 
			FROM user_sessions s 
			JOIN users u ON s.user_id = u.user_id 
			ORDER BY s.last_activity DESC 
			LIMIT 10
		)";

		if (_sql_connector->ExecuteQuery(query)) {
			MYSQL_RES* result = _sql_connector->GetResult();
			if (result) {
				std::cout << "\n[DEBUG] 최근 세션 상태 (최대 10개):" << std::endl;
				std::cout << "UserID | Username | Online | LoginTime | LastActivity" << std::endl;
				std::cout << "-------|----------|--------|-----------|-------------" << std::endl;

				MYSQL_ROW row;
				while ((row = mysql_fetch_row(result))) {
					std::cout << row[0] << " | " << row[1] << " | "
						<< (std::string(row[2]) == "1" ? "Y" : "N") << " | "
						<< (row[3] ? row[3] : "NULL") << " | "
						<< (row[4] ? row[4] : "NULL") << std::endl;
				}
				_sql_connector->FreeResult(result);
			}
		}
	}
	catch (const std::exception& e) {
		std::cerr << "[DatabaseThread] ShowSessionDebugInfo 예외: " << e.what() << std::endl;
	}
}