#pragma once

//NEW includes
#include "enet/enet.h"
#include "network.h"
#include <unordered_map>

#include "timer.h"

#include "../projects/spacegame/code/spaceship.h"

//RENEWED CLIENT 
class GameClient
{
public:
    static GameClient& Instance()
    {
        static GameClient instance;
        return instance;
    }

    void Create();
    bool ConnectToServer(const char* ip, const uint16_t port);
    void Update();
    void SendInput(const FlatBufferBuilder& builder);
    void DisconnectFromServer();

    std::unordered_map<uint32_t, Game::ClientSpaceship> spaceships; //all spaceships
    std::unordered_map<uint32_t, Game::ClientLaser> lasers; //all lasers
    uint32_t myPlayerID = -1; //Player controlled spaceship indentifier
    ENetPeer* GetPeer() const { return peer; }

    //TESTING
    uint64_t getServerTimeZero() const { return serverTime; }
    uint64_t GetClientSyncTime()
    {
        uint64_t clientNow = serverTime + (Time::Now() - clientTimeZero);
        return clientNow;
    }


private:
    ENetHost* client;
    ENetPeer* peer; //server peer
    bool isActive = false;

    //time (synchronize time elapsed with server)
    uint64_t currentTime = 0;
    uint64_t serverTime = 0; //ASSIGNED IN CONNECT BUT NEVER UPDATES THE TIME AFTER
    uint64_t clientTimeZero = 0; //client Time when it connected to server
    uint64_t lastUpdate = 0;

    void OnRecievepacket(ENetPacket* packet);

};

extern GameClient gameClient;