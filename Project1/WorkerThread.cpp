#include "WorkerThread.h"
#include "LockFreeQueue.h"
#include <iostream>
#include <chrono>
#include <vector>

// ���� ������ (���� ȣȯ��)
WorkerThread::WorkerThread(SOCKET ClientSocket)
	: _do_thread(true), _head(nullptr), _tail(nullptr), _client_count(0), _task_queue(nullptr)
{
	SocketNode* newNode = new SocketNode(ClientSocket);
	_head.store(newNode);
	_tail.store(newNode);
	_client_count.store(1);

	_thread = std::make_unique<std::thread>(&WorkerThread::RunOnServerThread, this);
}

// ���ο� ������ (Task ť ����)
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

	// ���� ���ϵ� ����
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
		SocketNode* next = tail->next.load();

		if (tail == _tail.load()) {  // tail�� ������� �ʾҴ��� Ȯ��
			if (next == nullptr) {
				// tail�� next�� �� ���� ���� �õ�
				if (tail->next.compare_exchange_weak(next, newNode))
				{
					// �����ϸ� tail�� �� ���� ������Ʈ
					_tail.compare_exchange_weak(tail, newNode);
					_client_count.fetch_add(1);
					printf("[WorkerThread] Ŭ���̾�Ʈ �߰�: %d\n", _client_count.load());
					break;
				}
			}
			else {
				// tail�� ���� �������� �ƴϸ� tail�� ������ �̵� �õ�
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
				// ù ��° ��� ����
				if (_head.compare_exchange_weak(current, next)) {
					if (next == nullptr) {
						_tail.store(nullptr);
					}
					closesocket(current->socket);
					delete current;
					_client_count.fetch_sub(1);
					printf("[WorkerThread] Ŭ���̾�Ʈ ����: %d\n", _client_count.load());
					return;
				}
			}
			else {
				// �߰� �Ǵ� ������ ��� ����
				if (prev->next.compare_exchange_weak(current, next)) {
					if (current == _tail.load()) {
						_tail.store(prev);
					}
					closesocket(current->socket);
					delete current;
					_client_count.fetch_sub(1);
					printf("[WorkerThread] Ŭ���̾�Ʈ ����: %d\n", _client_count.load());
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
		std::cerr << "[WorkerThread] ������ �����Ͱ� �������" << std::endl;
		return false;
	}

	// ���� ��Ŷ ũ�⸦ ���� (4����Ʈ ���)
	uint32_t packetSize = static_cast<uint32_t>(data.size());
	uint32_t networkSize = htonl(packetSize);  // ��Ʈ��ũ ����Ʈ ������ ��ȯ

	int headerResult = send(clientSocket, reinterpret_cast<const char*>(&networkSize), sizeof(networkSize), 0);
	if (headerResult <= 0) {
		std::cerr << "[WorkerThread] ��� ���� ����: " << WSAGetLastError() << std::endl;
		return false;
	}

	// ���� ��Ŷ ������ ����
	int totalSent = 0;
	int dataSize = static_cast<int>(data.size());

	while (totalSent < dataSize) {
		int sent = send(clientSocket,
			reinterpret_cast<const char*>(data.data()) + totalSent,
			dataSize - totalSent, 0);

		if (sent <= 0) {
			std::cerr << "[WorkerThread] ������ ���� ����: " << WSAGetLastError() << std::endl;
			return false;
		}

		totalSent += sent;
	}

	std::cout << "[WorkerThread] ������ ���� �Ϸ� - ����: " << clientSocket
		<< ", ũ��: " << data.size() << " bytes" << std::endl;
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
	printf("[WorkerThread] WorkerThread ����\n");

	while (_do_thread.load()) {
		if (_client_count.load() == 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		fd_set readSet;
		FD_ZERO(&readSet);

		SOCKET maxSock = 0;
		std::vector<SOCKET> socketList;

		// ���� ����� ���ϵ��� ���� (lock-free ��ȸ)
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

		// �̺�Ʈ �߻��� ���ϵ� ó��
		for (SOCKET s : socketList) {
			if (FD_ISSET(s, &readSet)) {
				char buffer[4096];
				ProcessClientData(s, buffer, sizeof(buffer));
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	printf("[WorkerThread] WorkerThread ����\n");
}

void WorkerThread::ProcessClientData(SOCKET clientSocket, char* buffer, int bufferSize)
{
	// ���� ��Ŷ ũ�� �б� (4����Ʈ ���)
	uint32_t packetSize = 0;
	int headerReceived = recv(clientSocket, reinterpret_cast<char*>(&packetSize), sizeof(packetSize), 0);

	if (headerReceived <= 0) {
		// Ŭ���̾�Ʈ ���� ����
		RemoveSocketFromList(clientSocket);
		printf("[WorkerThread] Ŭ���̾�Ʈ ���� ����. ���� Ŭ���̾�Ʈ: %d\n", _client_count.load());
		return;
	}

	if (headerReceived != sizeof(packetSize)) {
		std::cerr << "[WorkerThread] �ҿ����� ��� ����" << std::endl;
		return;
	}

	// ��Ʈ��ũ ����Ʈ �������� ȣ��Ʈ ����Ʈ ������ ��ȯ
	packetSize = ntohl(packetSize);

	if (packetSize == 0 || packetSize > 65536) { // �ִ� 64KB ����
		std::cerr << "[WorkerThread] �߸��� ��Ŷ ũ��: " << packetSize << std::endl;
		RemoveSocketFromList(clientSocket);
		return;
	}

	// ���� ��Ŷ ������ �б�
	std::vector<uint8_t> packetData(packetSize);
	int totalReceived = 0;

	while (totalReceived < static_cast<int>(packetSize)) {
		int received = recv(clientSocket,
			reinterpret_cast<char*>(packetData.data()) + totalReceived,
			packetSize - totalReceived, 0);

		if (received <= 0) {
			std::cerr << "[WorkerThread] ��Ŷ ������ ���� ����" << std::endl;
			RemoveSocketFromList(clientSocket);
			return;
		}

		totalReceived += received;
	}

	// Task ���� �� ť�� �߰�
	if (_task_queue) {
		Task task(clientSocket, 0, packetData.data(), packetData.size());

		_task_queue->enqueue(task);
			std::cout << "[WorkerThread] ��Ŷ ���� �Ϸ� - ����: " << clientSocket
			<< ", ũ��: " << packetSize << " bytes" << std::endl;

	}
	else {
		std::cerr << "[WorkerThread] Task ť�� �������� ����" << std::endl;
	}
}

void WorkerThread::StopThread()
{
	_do_thread.store(false);
}