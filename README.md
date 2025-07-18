주제 : 멀티 쓰레드와 멀티 플렉싱을 이용한 간단한 DB Account서버 제작(UE5클라이언트랑 연동)

작업 기간 : 2025년 7월 2일 ~ 2025년 7월 7일

사용한 기술 : 멀티 쓰레드, 멀티 플렉싱, 소켓, FlatBuffer, 락프리 알고리즘, mutex

사용한 언어 : C++

사용한 DB 및 DB 툴: Maria DB, HeidiSQL

사용한 툴 : Visual Studio 2022

*프로그램 구조*
1. Server클래스를 싱글톤으로 만들어 1개의 서버만 켜지도록 작업
2. Socket을 통해 접속 시 접속자 50명 기준으로 Work thread추가하는 형식으로 제작, 접속자가 나가 0명이 된 쓰래드는 대기상태로 대기 하다 다시 접속자가 들어올 경우 재동작
3. DB Thread와 Send용 쓰래드 분리
4. Work Thread에서 Pakcet을 받아 ResvQueue(Lockfree Queue)에 push한 경우 DB Thread에서 ResvQueue에 들어 있는 데이터를 pop해 DB에서 작업 후 Send Queue에 Push
5. Send Thread에서 Send Queue에 있는 데이터를 Pop해서 클라이언트로 전하는 형식의 구조로 설계

패킷은 Flatbuffer를 이용하였으며, UE5로 제작된 멀티 레이드 컨텐츠용 DB서버로 사용하고자 제작된 프로젝트
UE5 클라이언트 코드 : https://github.com/sminhyeong/ProjectC.git
