#define NOMINMAX

#include "DatabaseThread.h"

// 필요한 헤더들 포함
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
	std::cout << "[DatabaseThread] 중지 신호 전송..." << std::endl;
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

		// 기존 연결 정리
		_sql_connector.reset();
		_sql_connector = std::make_unique<MySqlConnector>();

		// 재연결 시도
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

void DatabaseThread::Run()
{
	std::cout << "[DatabaseThread] DB 처리 스레드 시작" << std::endl;

	auto last_connection_check = std::chrono::steady_clock::now();
	const auto connection_check_interval = std::chrono::seconds(30); // 30초마다 연결 상태 체크

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
			// 큐가 비어있으면 잠시 대기
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	std::cout << "[DatabaseThread] DB 처리 스레드 종료" << std::endl;
}

void DatabaseThread::ProcessTask(const Task& task)
{
	if (task.flatbuffer_data.empty()) {
		std::cerr << "[DatabaseThread] 빈 FlatBuffer 데이터" << std::endl;
		return;
	}

	// DB 연결 상태 체크
	if (!CheckDBConnection()) {
		std::cerr << "[DatabaseThread] DB 연결 없음, 재연결 시도..." << std::endl;
		if (!ReconnectIfNeeded()) {
			std::cerr << "[DatabaseThread] DB 재연결 실패, 태스크 스킵" << std::endl;
			return;
		}
	}

	// 패킷 유효성 검사
	if (!_packet_manager->IsValidPacket(task.flatbuffer_data.data(), task.flatbuffer_data.size())) {
		std::cerr << "[DatabaseThread] 잘못된 패킷: " << _packet_manager->GetLastError() << std::endl;
		return;
	}

	// 패킷 타입 확인
	EventType packetType = _packet_manager->GetPacketType(task.flatbuffer_data.data(), task.flatbuffer_data.size());
	std::cout << "[DatabaseThread] 처리 중인 패킷: " << _packet_manager->GetPacketTypeName(packetType) << std::endl;

	// 패킷 타입별 처리
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
	default:
		std::cout << "[DatabaseThread] 처리되지 않은 패킷 타입: " << static_cast<int>(packetType) << std::endl;
		break;
	}
}

void DatabaseThread::HandleLoginRequest(const Task& task)
{
	const C2S_Login* loginReq = _packet_manager->ParseLoginRequest(task.flatbuffer_data.data(), task.flatbuffer_data.size());

	if (!loginReq || !_packet_manager->ValidateLoginRequest(loginReq)) {
		std::cerr << "[DatabaseThread] 로그인 요청 검증 실패: " << _packet_manager->GetLastError() << std::endl;
		SendErrorResponse(task, EventType_S2C_Login, ResultCode_INVALID_USER);
		return;
	}

	std::cout << "[DatabaseThread] 로그인 요청 처리: " << loginReq->username()->c_str() << std::endl;

	if (!CheckDBConnection()) {
		std::cerr << "[DatabaseThread] 데이터베이스 연결 없음" << std::endl;
		SendErrorResponse(task, EventType_S2C_Login, ResultCode_FAIL);
		return;
	}

	// SQL 쿼리 준비 (사용자 인증 + 레벨 정보)
	std::stringstream query;
	query << "SELECT u.user_id, COALESCE(p.level, 1) as level "
		<< "FROM users u "
		<< "LEFT JOIN player_data p ON u.user_id = p.user_id "
		<< "WHERE u.username = '" << loginReq->username()->c_str()
		<< "' AND u.password = '" << loginReq->password()->c_str()
		<< "' AND u.is_active = 1";

	if (_sql_connector->ExecuteQuery(query.str())) {
		MYSQL_RES* result = _sql_connector->GetResult();
		if (result) {
			// MySQL 결과를 자동으로 FlatBuffer 응답으로 변환
			auto responsePacket = _packet_manager->CreateLoginResponseFromDB(result, loginReq->username()->str(), task.client_socket);

			// 마지막 로그인 시간 업데이트
			if (!responsePacket.empty()) {
				std::stringstream updateQuery;
				updateQuery << "UPDATE users SET last_login = NOW() WHERE username = '" << loginReq->username()->c_str() << "'";
				_sql_connector->ExecuteQuery(updateQuery.str());

				std::cout << "[DatabaseThread] 로그인 성공: " << loginReq->username()->c_str() << std::endl;
			}

			SendResponse(task, responsePacket);
			_sql_connector->FreeResult(result);
		}
		else {
			SendErrorResponse(task, EventType_S2C_Login, ResultCode_FAIL);
		}
	}
	else {
		std::cerr << "[DatabaseThread] 로그인 쿼리 실행 실패" << std::endl;
		SendErrorResponse(task, EventType_S2C_Login, ResultCode_FAIL);
	}
}

void DatabaseThread::HandleLogoutRequest(const Task& task)
{
	const C2S_Logout* logoutReq = _packet_manager->ParseLogoutRequest(task.flatbuffer_data.data(), task.flatbuffer_data.size());

	if (!logoutReq) {
		std::cerr << "[DatabaseThread] 로그아웃 요청 파싱 실패" << std::endl;
		SendErrorResponse(task, EventType_S2C_Logout, ResultCode_FAIL);
		return;
	}

	std::cout << "[DatabaseThread] 로그아웃 요청 처리: 사용자 ID " << logoutReq->user_id() << std::endl;

	// 로그아웃 시간 업데이트 (선택사항)
	if (CheckDBConnection()) {
		std::stringstream updateQuery;
		updateQuery << "UPDATE users SET last_logout = NOW() WHERE user_id = " << logoutReq->user_id();
		_sql_connector->ExecuteQuery(updateQuery.str());
	}

	// 로그아웃은 특별한 DB 작업이 필요하지 않으므로 성공 응답만 전송
	auto responsePacket = _packet_manager->CreateLogoutResponse(ResultCode_SUCCESS, "로그아웃 완료", task.client_socket);
	SendResponse(task, responsePacket);
}

void DatabaseThread::HandleCreateAccountRequest(const Task& task)
{
	const C2S_CreateAccount* accountReq = _packet_manager->ParseCreateAccountRequest(task.flatbuffer_data.data(), task.flatbuffer_data.size());

	if (!accountReq || !_packet_manager->ValidateCreateAccountRequest(accountReq)) {
		std::cerr << "[DatabaseThread] 계정 생성 요청 검증 실패: " << _packet_manager->GetLastError() << std::endl;
		SendErrorResponse(task, EventType_S2C_CreateAccount, ResultCode_INVALID_USER);
		return;
	}

	std::cout << "[DatabaseThread] 계정 생성 요청 처리: " << accountReq->username()->c_str() << std::endl;

	if (!CheckDBConnection()) {
		SendErrorResponse(task, EventType_S2C_CreateAccount, ResultCode_FAIL);
		return;
	}

	// 1. 중복 사용자명 확인
	std::stringstream checkQuery;
	checkQuery << "SELECT user_id FROM users WHERE username = '" << accountReq->username()->c_str() << "'";

	if (_sql_connector->ExecuteQuery(checkQuery.str())) {
		MYSQL_RES* result = _sql_connector->GetResult();
		if (result) {
			MYSQL_ROW row = mysql_fetch_row(result);
			if (row) {
				// 이미 존재하는 사용자
				_sql_connector->FreeResult(result);
				auto responsePacket = _packet_manager->CreateAccountErrorResponse(ResultCode_FAIL, "이미 존재하는 사용자명입니다", task.client_socket);
				SendResponse(task, responsePacket);
				return;
			}
			_sql_connector->FreeResult(result);
		}
	}

	// 2. 새 계정 생성
	std::stringstream insertQuery;
	insertQuery << "INSERT INTO users (username, password, nickname, created_at) VALUES ('"
		<< accountReq->username()->c_str() << "', '"
		<< accountReq->password()->c_str() << "', '"
		<< accountReq->nickname()->c_str() << "', NOW())";

	if (_sql_connector->ExecuteQuery(insertQuery.str())) {
		// 생성된 user_id 가져오기
		std::stringstream getIdQuery;
		getIdQuery << "SELECT LAST_INSERT_ID()";

		if (_sql_connector->ExecuteQuery(getIdQuery.str())) {
			MYSQL_RES* result = _sql_connector->GetResult();
			if (result) {
				MYSQL_ROW row = mysql_fetch_row(result);
				if (row) {
					uint32_t new_user_id = std::stoul(row[0]);

					// 기본 플레이어 데이터 생성
					std::stringstream playerInsert;
					playerInsert << "INSERT INTO player_data (user_id, level, exp, hp, mp, attack, defense, gold, map_id, pos_x, pos_y, created_at) VALUES ("
						<< new_user_id << ", 1, 0, 100, 50, 10, 5, 1000, 1, 0.0, 0.0, NOW())";
					_sql_connector->ExecuteQuery(playerInsert.str());

					// 기본 아이템 지급 (나무 검, 가죽 갑옷)
					std::stringstream itemInsert;
					itemInsert << "INSERT INTO player_inventory (user_id, item_id, item_count, created_at) VALUES "
						<< "(" << new_user_id << ", 1, 1, NOW()), "
						<< "(" << new_user_id << ", 3, 1, NOW())";
					_sql_connector->ExecuteQuery(itemInsert.str());

					auto responsePacket = _packet_manager->CreateAccountResponse(ResultCode_SUCCESS, new_user_id, "계정 생성 성공", task.client_socket);
					SendResponse(task, responsePacket);

					std::cout << "[DatabaseThread] 계정 생성 성공: " << accountReq->username()->c_str() << " (ID: " << new_user_id << ")" << std::endl;
				}
				_sql_connector->FreeResult(result);
			}
		}
	}
	else {
		auto responsePacket = _packet_manager->CreateAccountErrorResponse(ResultCode_FAIL, "계정 생성 실패", task.client_socket);
		SendResponse(task, responsePacket);
	}
}

void DatabaseThread::HandlePlayerDataRequest(const Task& task)
{
	const C2S_PlayerData* playerReq = _packet_manager->ParsePlayerDataRequest(task.flatbuffer_data.data(), task.flatbuffer_data.size());

	if (!playerReq || !_packet_manager->ValidatePlayerDataRequest(playerReq)) {
		std::cerr << "[DatabaseThread] 플레이어 데이터 요청 검증 실패: " << _packet_manager->GetLastError() << std::endl;
		SendErrorResponse(task, EventType_S2C_PlayerData, ResultCode_INVALID_USER);
		return;
	}

	std::cout << "[DatabaseThread] 플레이어 데이터 요청 처리 - 사용자 ID: " << playerReq->user_id()
		<< ", 타입: " << playerReq->request_type() << std::endl;

	if (!CheckDBConnection()) {
		SendErrorResponse(task, EventType_S2C_PlayerData, ResultCode_FAIL);
		return;
	}

	if (playerReq->request_type() == 0) {
		// 조회
		std::stringstream query;
		query << "SELECT u.username, p.level, p.exp, p.hp, p.mp, p.attack, p.defense, p.gold, p.map_id, p.pos_x, p.pos_y "
			<< "FROM users u JOIN player_data p ON u.user_id = p.user_id "
			<< "WHERE u.user_id = " << playerReq->user_id() << " AND u.is_active = 1";

		if (_sql_connector->ExecuteQuery(query.str())) {
			MYSQL_RES* result = _sql_connector->GetResult();
			if (result) {
				auto responsePacket = _packet_manager->CreatePlayerDataResponseFromDB(result, playerReq->user_id(), task.client_socket);
				SendResponse(task, responsePacket);
				_sql_connector->FreeResult(result);
			}
			else {
				SendErrorResponse(task, EventType_S2C_PlayerData, ResultCode_USER_NOT_FOUND);
			}
		}
		else {
			SendErrorResponse(task, EventType_S2C_PlayerData, ResultCode_FAIL);
		}
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
			auto responsePacket = _packet_manager->CreatePlayerDataResponse(ResultCode_SUCCESS, playerReq->user_id(), "",
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
		std::cerr << "[DatabaseThread] 아이템 데이터 요청 검증 실패: " << _packet_manager->GetLastError() << std::endl;
		SendErrorResponse(task, EventType_S2C_ItemData, ResultCode_INVALID_USER);
		return;
	}

	std::cout << "[DatabaseThread] 아이템 데이터 요청 처리 - 사용자 ID: " << itemReq->user_id()
		<< ", 타입: " << itemReq->request_type() << std::endl;

	if (!CheckDBConnection()) {
		SendErrorResponse(task, EventType_S2C_ItemData, ResultCode_FAIL);
		return;
	}

	if (itemReq->request_type() == 0) {
		// 인벤토리 조회
		std::stringstream query;
		query << "SELECT i.item_id, m.item_name, i.item_count, m.item_type, p.gold "
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
				_sql_connector->FreeResult(result);
			}
			else {
				SendErrorResponse(task, EventType_S2C_ItemData, ResultCode_USER_NOT_FOUND);
			}
		}
		else {
			SendErrorResponse(task, EventType_S2C_ItemData, ResultCode_FAIL);
		}
	}
	else if (itemReq->request_type() == 1) {
		// 아이템 추가
		std::stringstream query;
		query << "INSERT INTO player_inventory (user_id, item_id, item_count, created_at) VALUES ("
			<< itemReq->user_id() << ", " << itemReq->item_id() << ", "
			<< itemReq->item_count() << ", NOW()) "
			<< "ON DUPLICATE KEY UPDATE item_count = item_count + " << itemReq->item_count()
			<< ", updated_at = NOW()";

		if (_sql_connector->ExecuteQuery(query.str())) {
			// 업데이트된 인벤토리 다시 조회해서 응답
			HandleItemDataRequest(task); // request_type을 0으로 바꿔서 재귀 호출하는 것보다는
			std::cout << "[DatabaseThread] 아이템 추가 완료: 사용자 ID " << itemReq->user_id() << std::endl;
		}
		else {
			SendErrorResponse(task, EventType_S2C_ItemData, ResultCode_FAIL);
		}
	}
	else if (itemReq->request_type() == 2) {
		// 아이템 삭제
		std::stringstream query;
		query << "UPDATE player_inventory SET item_count = GREATEST(0, item_count - " << itemReq->item_count() << "), "
			<< "updated_at = NOW() WHERE user_id = " << itemReq->user_id()
			<< " AND item_id = " << itemReq->item_id();

		if (_sql_connector->ExecuteQuery(query.str())) {
			// 수량이 0이 된 아이템은 삭제
			std::stringstream deleteQuery;
			deleteQuery << "DELETE FROM player_inventory WHERE user_id = " << itemReq->user_id()
				<< " AND item_id = " << itemReq->item_id() << " AND item_count <= 0";
			_sql_connector->ExecuteQuery(deleteQuery.str());

			std::cout << "[DatabaseThread] 아이템 삭제 완료: 사용자 ID " << itemReq->user_id() << std::endl;
		}
		else {
			SendErrorResponse(task, EventType_S2C_ItemData, ResultCode_FAIL);
		}
	}
}

void DatabaseThread::HandleMonsterDataRequest(const Task& task)
{
	const C2S_MonsterData* monsterReq = _packet_manager->ParseMonsterDataRequest(task.flatbuffer_data.data(), task.flatbuffer_data.size());

	if (!monsterReq) {
		std::cerr << "[DatabaseThread] 몬스터 데이터 요청 파싱 실패" << std::endl;
		SendErrorResponse(task, EventType_S2C_MonsterData, ResultCode_FAIL);
		return;
	}

	std::cout << "[DatabaseThread] 몬스터 데이터 요청 처리 - 타입: " << monsterReq->request_type() << std::endl;

	if (!CheckDBConnection()) {
		SendErrorResponse(task, EventType_S2C_MonsterData, ResultCode_FAIL);
		return;
	}

	if (monsterReq->request_type() == 0) {
		// 모든 몬스터 조회
		std::string query = "SELECT monster_id, monster_name, level, hp, attack, defense, exp_reward, gold_reward "
			"FROM monster_master WHERE is_active = 1 ORDER BY level, monster_id";

		if (_sql_connector->ExecuteQuery(query)) {
			MYSQL_RES* result = _sql_connector->GetResult();
			if (result) {
				auto responsePacket = _packet_manager->CreateMonsterDataResponseFromDB(result, task.client_socket);
				SendResponse(task, responsePacket);
				_sql_connector->FreeResult(result);
			}
			else {
				SendErrorResponse(task, EventType_S2C_MonsterData, ResultCode_FAIL);
			}
		}
		else {
			SendErrorResponse(task, EventType_S2C_MonsterData, ResultCode_FAIL);
		}
	}
	else if (monsterReq->request_type() == 1) {
		// 몬스터 추가 (관리자 기능)
		if (monsterReq->monster_data()) {
			const MonsterData* monster = monsterReq->monster_data();
			std::stringstream insertQuery;
			insertQuery << "INSERT INTO monster_master (monster_name, level, hp, attack, defense, exp_reward, gold_reward, created_at) VALUES ('"
				<< monster->monster_name()->c_str() << "', "
				<< monster->level() << ", " << monster->hp() << ", "
				<< monster->attack() << ", " << monster->defense() << ", "
				<< monster->exp_reward() << ", " << monster->gold_reward() << ", NOW())";

			if (_sql_connector->ExecuteQuery(insertQuery.str())) {
				auto responsePacket = _packet_manager->CreateMonsterDataResponse(ResultCode_SUCCESS, task.client_socket);
				SendResponse(task, responsePacket);
				std::cout << "[DatabaseThread] 몬스터 추가 완료: " << monster->monster_name()->c_str() << std::endl;
			}
			else {
				SendErrorResponse(task, EventType_S2C_MonsterData, ResultCode_FAIL);
			}
		}
		else {
			SendErrorResponse(task, EventType_S2C_MonsterData, ResultCode_INVALID_USER);
		}
	}
	else if (monsterReq->request_type() == 2) {
		// 몬스터 삭제 (관리자 기능)
		std::stringstream deleteQuery;
		deleteQuery << "UPDATE monster_master SET is_active = 0, updated_at = NOW() WHERE monster_id = "
			<< monsterReq->monster_id();

		if (_sql_connector->ExecuteQuery(deleteQuery.str())) {
			auto responsePacket = _packet_manager->CreateMonsterDataResponse(ResultCode_SUCCESS, task.client_socket);
			SendResponse(task, responsePacket);
			std::cout << "[DatabaseThread] 몬스터 삭제 완료: ID " << monsterReq->monster_id() << std::endl;
		}
		else {
			SendErrorResponse(task, EventType_S2C_MonsterData, ResultCode_FAIL);
		}
	}
}

void DatabaseThread::HandlePlayerChatRequest(const Task& task)
{
	const C2S_PlayerChat* chatReq = _packet_manager->ParsePlayerChatRequest(task.flatbuffer_data.data(), task.flatbuffer_data.size());

	if (!chatReq) {
		std::cerr << "[DatabaseThread] 채팅 요청 파싱 실패" << std::endl;
		SendErrorResponse(task, EventType_S2C_PlayerChat, ResultCode_FAIL);
		return;
	}

	std::cout << "[DatabaseThread] 채팅 요청 처리 - 타입: " << chatReq->request_type() << std::endl;

	if (!CheckDBConnection()) {
		SendErrorResponse(task, EventType_S2C_PlayerChat, ResultCode_FAIL);
		return;
	}

	if (chatReq->request_type() == 0) {
		// 채팅 로그 조회
		std::stringstream query;
		query << "SELECT c.chat_id, c.sender_id, u.username, c.message, c.chat_type, UNIX_TIMESTAMP(c.timestamp) "
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
				_sql_connector->FreeResult(result);
			}
			else {
				SendErrorResponse(task, EventType_S2C_PlayerChat, ResultCode_FAIL);
			}
		}
		else {
			SendErrorResponse(task, EventType_S2C_PlayerChat, ResultCode_FAIL);
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

		// SQL 인젝션 방지를 위해 문자열 이스케이프 처리
		std::string escaped_message = chatReq->message()->str();
		// 간단한 이스케이프 처리 (실제로는 mysql_real_escape_string 사용 권장)
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
		}
		else {
			SendErrorResponse(task, EventType_S2C_PlayerChat, ResultCode_FAIL);
		}
	}
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

	// 응답 큐에 추가
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