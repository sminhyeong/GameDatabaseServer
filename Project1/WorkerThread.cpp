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

	// 먼저 패킷 크기를 전송 (4바이트 헤더)
	uint32_t packetSize = static_cast<uint32_t>(data.size());
	uint32_t networkSize = htonl(packetSize);  // 네트워크 바이트 순서로 변환

	int headerResult = send(clientSocket, reinterpret_cast<const char*>(&networkSize), sizeof(networkSize), 0);
	if (headerResult <= 0) {
		std::cerr << "[WorkerThread] 헤더 전송 실패: " << WSAGetLastError() << std::endl;
		return false;
	}

	// 실제 패킷 데이터 전송
	int totalSent = 0;
	int dataSize = static_cast<int>(data.size());

	while (totalSent < dataSize) {
		int sent = send(clientSocket,
			reinterpret_cast<const char*>(data.data()) + totalSent,
			dataSize - totalSent, 0);

		if (sent <= 0) {
			std::cerr << "[WorkerThread] 데이터 전송 실패: " << WSAGetLastError() << std::endl;
			return false;
		}

		totalSent += sent;
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

		fd_set readSet;
		FD_ZERO(&readSet);

		SOCKET maxSock = 0;
		std::vector<SOCKET> socketList;

		// 현재 연결된 소켓들을 복사 (lock-free 순회)
		SocketNode* current = _head.load();
		while (current != nullptr) {
			SOCKET s = current->socket;
			socketList.push_back(s);
			FD_SET(s, &readSet);
			if (s > maxSock) maxSock = s;
			current = current->next.load();
		}

		if (socketList.empty()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 10000; // 10ms

		int result = select(0, &readSet, NULL, NULL, &timeout);

		if (result == SOCKET_ERROR) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
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
		// 클라이언트 연결 종료
		RemoveSocketFromList(clientSocket);
		printf("[WorkerThread] 클라이언트 연결 종료. 남은 클라이언트: %d\n", _client_count.load());
		return;
	}

	if (headerReceived != sizeof(packetSize)) {
		std::cerr << "[WorkerThread] 불완전한 헤더 수신" << std::endl;
		return;
	}

	// 네트워크 바이트 순서에서 호스트 바이트 순서로 변환
	packetSize = ntohl(packetSize);

	if (packetSize == 0 || packetSize > 65536) { // 최대 64KB 제한
		std::cerr << "[WorkerThread] 잘못된 패킷 크기: " << packetSize << std::endl;
		RemoveSocketFromList(clientSocket);
		return;
	}

	// 실제 패킷 데이터 읽기
	std::vector<uint8_t> packetData(packetSize);
	int totalReceived = 0;

	while (totalReceived < static_cast<int>(packetSize)) {
		int received = recv(clientSocket,
			reinterpret_cast<char*>(packetData.data()) + totalReceived,
			packetSize - totalReceived, 0);

		if (received <= 0) {
			std::cerr << "[WorkerThread] 패킷 데이터 수신 실패" << std::endl;
			RemoveSocketFromList(clientSocket);
			return;
		}

		totalReceived += received;
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