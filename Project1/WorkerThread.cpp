#include "WorkerThread.h"
#include "LockFreeQueue.h"
#include <iostream>
#include <chrono>
#include <vector>

// 기존 생성자 (하위 호환성)
WorkerThread::WorkerThread(SOCKET ClientSocket)
	: _do_thread(true), _head(nullptr), _tail(nullptr), _client_count(0), _task_queue(nullptr)
{
	SocketNode* newNode = new SocketNode(ClientSocket);
	_head.store(newNode);
	_tail.store(newNode);
	_client_count.store(1);

	_thread = std::make_unique<std::thread>(&WorkerThread::RunOnServerThread, this);
}

// 새로운 생성자 (Task 큐 포함)
WorkerThread::WorkerThread(SOCKET ClientSocket, LockFreeQueue<Task>* taskQueue)
	: _do_thread(true), _head(nullptr), _tail(nullptr), _client_count(0), _task_queue(taskQueue)
{
	SocketNode* newNode = new SocketNode(ClientSocket);
	_head.store(newNode);
	_tail.store(newNode);
	_client_count.store(1);

	_thread = std::make_unique<std::thread>(&WorkerThread::RunOnServerThread, this);
}

WorkerThread::~WorkerThread()
{
	StopThread();

	if (_thread && _thread->joinable()) {
		_thread->join();
	}

	// 남은 소켓들 정리
	SocketNode* current = _head.load();
	while (current != nullptr) {
		SocketNode* next = current->next.load();
		closesocket(current->socket);
		delete current;
		current = next;
	}
}

void WorkerThread::AddClient(SOCKET clientSocket)
{
	if (_client_count.load() >= MAX_CLIENT_COUNT) {
		closesocket(clientSocket);
		return;
	}

	SocketNode* newNode = new SocketNode(clientSocket);

	// Lock-free tail insertion
	while (true) {
		SocketNode* tail = _tail.load();
		
		if (tail == nullptr)
		{
			SocketNode* newNode = new SocketNode(clientSocket);
			_head.store(newNode);
			_tail.store(newNode);
			_client_count.store(1);
			break;
		}

		SocketNode* next = tail->next.load();

		if (tail == _tail.load()) {  // tail이 변경되지 않았는지 확인
			if (next == nullptr) {
				// tail의 next를 새 노드로 설정 시도
				if (tail->next.compare_exchange_weak(next, newNode))
				{
					// 성공하면 tail을 새 노드로 업데이트
					_tail.compare_exchange_weak(tail, newNode);
					_client_count.fetch_add(1);
					printf("[WorkerThread] 클라이언트 추가: %d\n", _client_count.load());
					break;
				}
			}
			else {
				// tail이 실제 마지막이 아니면 tail을 앞으로 이동 시도
				_tail.compare_exchange_weak(tail, next);
			}
		}
	}
}

void WorkerThread::RemoveSocketFromList(SOCKET target_socket)
{
	SocketNode* prev = nullptr;
	SocketNode* current = _head.load();

	while (current != nullptr) {
		if (current->socket == target_socket) {
			SocketNode* next = current->next.load();

			if (prev == nullptr) {
				// 첫 번째 노드 제거
				if (_head.compare_exchange_weak(current, next)) {
					if (next == nullptr) {
						_tail.store(nullptr);
					}
					closesocket(current->socket);
					delete current;
					_client_count.fetch_sub(1);
					printf("[WorkerThread] 클라이언트 제거: %d\n", _client_count.load());
					return;
				}
			}
			else {
				// 중간 또는 마지막 노드 제거
				if (prev->next.compare_exchange_weak(current, next)) {
					if (current == _tail.load()) {
						_tail.store(prev);
					}
					closesocket(current->socket);
					delete current;
					_client_count.fetch_sub(1);
					printf("[WorkerThread] 클라이언트 제거: %d\n", _client_count.load());
					return;
				}
			}
		}
		prev = current;
		current = current->next.load();
	}
}

bool WorkerThread::SendToClient(SOCKET clientSocket, const std::vector<uint8_t>& data)
{
	std::lock_guard<std::mutex> lock(_send_mutex);

	if (data.empty()) {
		std::cerr << "[WorkerThread] 전송할 데이터가 비어있음" << std::endl;
		return false;
	}

	// 소켓 유효성 검사 추가
	if (!HasClient(clientSocket)) {
		std::cerr << "[WorkerThread] 유효하지 않은 클라이언트 소켓: " << clientSocket << std::endl;
		return false;
	}

	// 먼저 패킷 크기를 전송 (4바이트 헤더)
	uint32_t packetSize = static_cast<uint32_t>(data.size());
	uint32_t networkSize = htonl(packetSize);

	// 헤더 전송 개선
	int headerResult = send(clientSocket, reinterpret_cast<const char*>(&networkSize), sizeof(networkSize), 0);
	if (headerResult <= 0) {
		int error = WSAGetLastError();
		if (error != WSAEWOULDBLOCK) {
			std::cerr << "[WorkerThread] 헤더 전송 실패: " << error << std::endl;
			RemoveSocketFromList(clientSocket);
			return false;
		}
		// WSAEWOULDBLOCK인 경우 재시도 로직 추가 가능
		return false;
	}

	// 실제 패킷 데이터 전송 개선
	int totalSent = 0;
	int dataSize = static_cast<int>(data.size());
	int retryCount = 0;
	const int MAX_SEND_RETRY = 3;

	while (totalSent < dataSize && retryCount < MAX_SEND_RETRY) {
		int sent = send(clientSocket,
			reinterpret_cast<const char*>(data.data()) + totalSent,
			dataSize - totalSent, 0);

		if (sent > 0) {
			totalSent += sent;
			retryCount = 0;
		}
		else {
			int error = WSAGetLastError();
			if (error == WSAEWOULDBLOCK) {
				retryCount++;
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}
			else {
				std::cerr << "[WorkerThread] 데이터 전송 실패: " << error << std::endl;
				RemoveSocketFromList(clientSocket);
				return false;
			}
		}
	}

	if (totalSent < dataSize) {
		std::cerr << "[WorkerThread] 데이터 전송 불완료: " << totalSent << "/" << dataSize << " bytes" << std::endl;
		return false;
	}

	std::cout << "[WorkerThread] 데이터 전송 완료 - 소켓: " << clientSocket
		<< ", 크기: " << data.size() << " bytes" << std::endl;
	return true;
}

bool WorkerThread::HasClient(SOCKET clientSocket) const
{
	SocketNode* current = _head.load();
	while (current != nullptr) {
		if (current->socket == clientSocket) {
			return true;
		}
		current = current->next.load();
	}
	return false;
}

void WorkerThread::RunOnServerThread()
{
	printf("[WorkerThread] WorkerThread 시작\n");

	while (_do_thread.load()) {
		if (_client_count.load() == 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		fd_set readSet, errorSet; // 에러 셋 추가
		FD_ZERO(&readSet);
		FD_ZERO(&errorSet);

		SOCKET maxSock = 0;
		std::vector<SOCKET> socketList;

		// 현재 연결된 소켓들을 복사 (lock-free 순회)
		SocketNode* current = _head.load();
		while (current != nullptr) {
			SOCKET s = current->socket;
			if (s != INVALID_SOCKET) { // 유효한 소켓만 추가
				socketList.push_back(s);
				FD_SET(s, &readSet);
				FD_SET(s, &errorSet); // 에러 모니터링 추가
				if (s > maxSock) maxSock = s;
			}
			current = current->next.load();
		}

		if (socketList.empty()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 10000; // 10ms

		int result = select(0, &readSet, NULL, &errorSet, &timeout);

		if (result == SOCKET_ERROR) {
			int error = WSAGetLastError();
			if (error != WSAEINTR) { // 인터럽트가 아닌 실제 에러만 로깅
				std::cerr << "[WorkerThread] select 에러: " << error << std::endl;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		// 에러 발생한 소켓들 먼저 처리
		for (SOCKET s : socketList) {
			if (FD_ISSET(s, &errorSet)) {
				std::cout << "[WorkerThread] 소켓 에러 감지, 제거: " << s << std::endl;
				RemoveSocketFromList(s);
			}
		}

		// 이벤트 발생한 소켓들 처리
		for (SOCKET s : socketList) {
			if (FD_ISSET(s, &readSet)) {
				char buffer[4096];
				ProcessClientData(s, buffer, sizeof(buffer));
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	printf("[WorkerThread] WorkerThread 종료\n");
}

void WorkerThread::ProcessClientData(SOCKET clientSocket, char* buffer, int bufferSize)
{
	// 먼저 패킷 크기 읽기 (4바이트 헤더)
	uint32_t packetSize = 0;
	int headerReceived = recv(clientSocket, reinterpret_cast<char*>(&packetSize), sizeof(packetSize), 0);

	if (headerReceived <= 0) {
		int error = WSAGetLastError();
		// WSAEWOULDBLOCK은 non-blocking 소켓에서 정상적인 상황
		if (error == WSAEWOULDBLOCK) {
			return; // 단순히 리턴, 연결 끊지 않음
		}

		// 실제 연결 종료나 에러인 경우만 제거
		std::cout << "[WorkerThread] 클라이언트 연결 종료/에러 - 에러코드: " << error << std::endl;
		RemoveSocketFromList(clientSocket);
		return;
	}

	if (headerReceived != sizeof(packetSize)) {
		std::cerr << "[WorkerThread] 불완전한 헤더 수신: " << headerReceived << " bytes" << std::endl;
		return; // 재시도 가능하도록 연결 유지
	}

	// 네트워크 바이트 순서에서 호스트 바이트 순서로 변환
	packetSize = ntohl(packetSize);

	// 패킷 크기 검증 개선
	if (packetSize == 0) {
		std::cerr << "[WorkerThread] 빈 패킷 수신" << std::endl;
		return; // 빈 패킷은 무시하고 연결 유지
	}

	if (packetSize > 65536) { // 최대 64KB 제한
		std::cerr << "[WorkerThread] 패킷 크기 초과: " << packetSize << " bytes" << std::endl;
		RemoveSocketFromList(clientSocket);
		return;
	}

	// 실제 패킷 데이터 읽기 - 타임아웃 추가
	std::vector<uint8_t> packetData(packetSize);
	int totalReceived = 0;
	int retryCount = 0;
	const int MAX_RETRY = 5;

	while (totalReceived < static_cast<int>(packetSize) && retryCount < MAX_RETRY) {
		int received = recv(clientSocket,
			reinterpret_cast<char*>(packetData.data()) + totalReceived,
			packetSize - totalReceived, 0);

		if (received > 0) {
			totalReceived += received;
			retryCount = 0; // 성공 시 재시도 카운트 리셋
		}
		else if (received == 0) {
			// 연결이 정상적으로 종료됨
			std::cout << "[WorkerThread] 클라이언트가 연결을 정상 종료" << std::endl;
			RemoveSocketFromList(clientSocket);
			return;
		}
		else {
			int error = WSAGetLastError();
			if (error == WSAEWOULDBLOCK) {
				// non-blocking 상황, 잠시 대기 후 재시도
				retryCount++;
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}
			else {
				std::cerr << "[WorkerThread] 패킷 데이터 수신 실패 - 에러: " << error << std::endl;
				RemoveSocketFromList(clientSocket);
				return;
			}
		}
	}

	if (totalReceived < static_cast<int>(packetSize)) {
		std::cerr << "[WorkerThread] 패킷 수신 불완료: " << totalReceived << "/" << packetSize << " bytes" << std::endl;
		return; // 부분 수신된 경우 재시도를 위해 연결 유지
	}

	// Task 생성 및 큐에 추가
	if (_task_queue) {
		Task task(clientSocket, 0, packetData.data(), packetData.size());
		_task_queue->enqueue(task);
		std::cout << "[WorkerThread] 패킷 수신 완료 - 소켓: " << clientSocket
			<< ", 크기: " << packetSize << " bytes" << std::endl;
	}
	else {
		std::cerr << "[WorkerThread] Task 큐가 설정되지 않음" << std::endl;
	}
}

void WorkerThread::StopThread()
{
	_do_thread.store(false);
}