## Server
Language : C++  
IDE : VS 2017  
Others : Google Protocol Buffers(데이터 직렬화), WireShark

- IOCP 관련 공부
    - Non-Blocking IO 
    - Async IO  
    - Overlapped IO 
    - IO Completion Port(IOCP) 생성 및 등록
- Lobby Event
    - 클라이언트가 최초 접속하면 Room List를 전송한다.
        - 이후에는 Refresh 버튼을 누르면 갱신된다.
    - Game Room 생성
        - 클라이언트는 서버에게 요청을 보냄 { type, Room Name, Limits, User Name }
        - Room Name을 Key로 사용하여 Room 정보를 관리하는 Map에서 이미 존재하는지 여부를 판단.
            - if(map.find(roomName) != map.end()) -> 클라이언트에게 Error 메시지를 전송
        - Room 객체를 생성, Map에 등록 -> 클라이언트에게 관련 정보 전송
            
    - Room 입장.
        - 클라이언트는 서버에게 요청을 보냄 { type, Room Name, User Name }
        - Room Name을 이용해 Map에서 Room Id를 획득.
        - Room Id를 이용해 Room List Map에서 해당 Room이 존재하는지 여부 판단.
            - if(roomList.find(roomIdToEnter) == roomList.end()) -> Error 메시지 전송
        - 해당 Room이 시작하였는지 여부 판단.
            - 시작했다면 -> Error 메시지 전송
        - 해당 Room의 빈 자리가 있는지 여부 판단.
            - 방이 꽉 찼다면 -> Error 메시지 전송
        - Client 객체를 만들고, Room 객체와 Room Info 객체에 해당 Client정보를 등록
        - 방 안에 있는 모든 Client들에게 새로 갱신된 Room Info를 브로드캐스트

- Waiting Event
    - Ready Button (After Entering the room)
        - 클라이언트는 요청을 보냄 { type = READY_EVENT }
        - 해당 클라이언트가 Room에 존재하는지 여부를 판단
        - Ready 처리
            - 만약 Ready 상태라면, Unset
            - 그렇지 않다면, Set
        - 방 안에 있는 모든 Client들에게 새로 갱신된 Room Info를 브로드캐스트

    - Team Change
        - 클라이언트는 요청을 보냄  { type = TEAM_CHANGE }
        - 해당 클라이언트가 Room에 존재하는지 여부를 판단
        - 팀 변경 처리
            - 현재 팀 Array에서 제거하고, 상대 팀 Array에 요청을 보낸 Client를 추가한다.
            - 만약 상대팀 인원이 꽉 찼을 경우, 요청을 거절한다.
        - 방 안에 있는 모든 Client들에게 새로 갱신된 Room Info를 브로드캐스트

    - Room 나가기
        - 클라이언트는 요청을 보냄 { type = LEAVE_GAMEROOM }
        - 해당 클라이언트가 Room에 존재하는지 여부를 판단
        - 방 나가기 처리
            - 클라이언트가 속한 팀에서 해당 Client관련 정보를 제거
            - 만약 Room에 남아있는 인원이 0이면, Room관련 정보를 제거
            - 만약 나간 클라이언트가 Host일 경우, 새로운 host를 설정            
    - Chat
        - 클라이언트로부터 메시지 { type, Chat Message } 가 오면, 다른 클라이언트에게 브로드 캐스트 

- Game Event
    - 클라이언트는 FPS만큼 자신의 상태값을 서버에 전송하면, 서버는 다른 클라이언트에게 브로드 캐스트
    - 다른 클라이언트 상태값을 수신하면, 해당 클라이언트의 상태값을 갱신하고 화면에 보여준다.
