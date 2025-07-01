#include "WorkerThread.h"

WorkerThread::WorkerThread(SOCKET ClientSocket)
{
	_do_thread = true;
	_user_fds.push_back(ClientSocket);

	// 스레드 실행
	thrad = new std::thread(&WorkerThread::RunOnServerThread, this);
	//thrad->detach(); // 혹은 joinable 확인 후 join
	
}

WorkerThread::~WorkerThread()
{
    // 쓰레드는 detach 상태라 소멸자에서 join 불가, 그냥 포인터만 해제
    if (thrad) {
        thrad->join();
        delete thrad;
        thrad = nullptr;
    }

    // 열려있는 소켓 모두 닫기
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

        //maxSock의경우 Linux환경에서 select함수의 첫번째 인자에 MaxSock +1값을 넣어주기때문에 존재
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
            // select 오류 발생 시 잠깐 쉼
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // 삭제 예정 소켓 iterator 보관
        //std::list<std::list<SOCKET>::iterator> eraseList;
        //이벤트 발생시 데이터 주고 받음
        for (auto iter = _user_fds.begin(); iter != _user_fds.end();)
        {
            SOCKET s = *iter;
            if (FD_ISSET(s, &readSet))
            {
                int len = recv(s, buffer, sizeof(buffer) - 1, 0);
                if (len <= 0)
                {
                    // 클라이언트 접속 종료
                    closesocket(s);
                    iter = _user_fds.erase(iter);
                    continue;
                }

                // 메시지 수신 및 echo 응답
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
