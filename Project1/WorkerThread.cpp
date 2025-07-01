#include "WorkerThread.h"

WorkerThread::WorkerThread(SOCKET ClientSocket)
{
	_do_thread = true;
	_user_fds.push_back(ClientSocket);

	// ������ ����
	thrad = new std::thread(&WorkerThread::RunOnServerThread, this);
	//thrad->detach(); // Ȥ�� joinable Ȯ�� �� join
	
}

WorkerThread::~WorkerThread()
{
    // ������� detach ���¶� �Ҹ��ڿ��� join �Ұ�, �׳� �����͸� ����
    if (thrad) {
        thrad->join();
        delete thrad;
        thrad = nullptr;
    }

    // �����ִ� ���� ��� �ݱ�
    for (SOCKET s : _user_fds) {
        closesocket(s);
    }
    if(_user_fds.size() > 0)
        _user_fds.clear();
}

void WorkerThread::RunOnServerThread()
{
    _do_thread = true;
    char buffer[1024];
    printf("Create WorkerThread\n");
    while (true)
    {
        if (_user_fds.empty()) {
            break;
        }

        fd_set readSet;
        FD_ZERO(&readSet);

        //maxSock�ǰ�� Linuxȯ�濡�� select�Լ��� ù��° ���ڿ� MaxSock +1���� �־��ֱ⶧���� ����
        SOCKET maxSock = 0;
        for (SOCKET s : _user_fds)
        {
            FD_SET(s, &readSet);
            if (s > maxSock) maxSock = s;
        }

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000; // 10ms

        int result = select(0, &readSet, NULL, NULL, &timeout);

        if (result == SOCKET_ERROR) {
            // select ���� �߻� �� ��� ��
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // ���� ���� ���� iterator ����
        //std::list<std::list<SOCKET>::iterator> eraseList;
        //�̺�Ʈ �߻��� ������ �ְ� ����
        for (auto iter = _user_fds.begin(); iter != _user_fds.end();)
        {
            SOCKET s = *iter;
            if (FD_ISSET(s, &readSet))
            {
                int len = recv(s, buffer, sizeof(buffer) - 1, 0);
                if (len <= 0)
                {
                    // Ŭ���̾�Ʈ ���� ����
                    closesocket(s);
                    iter = _user_fds.erase(iter);
                    continue;
                }

                // �޽��� ���� �� echo ����
                buffer[len] = '\0';
                send(s, buffer, len, 0); // echo back
            }
            iter++;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    printf("ThreadBreak\n");
    _do_thread = false;
}

void WorkerThread::AddClient(SOCKET clientSocket)
{
    printf("Add Client %d\n", (int)_user_fds.size());

	_user_fds.push_back(clientSocket);
}
