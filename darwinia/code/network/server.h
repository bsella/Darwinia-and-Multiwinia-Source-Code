#ifndef SERVER_H
#define SERVER_H

#include "lib/llist.h"
#include "lib/darray.h"


class NetLib;
class NetMutex;
class NetSocketListner;
class ServerToClient;
class ServerToClientLetter;
class NetworkUpdate;


class ServerTeam
{
public:
    int m_clientId;

    ServerTeam(int _clientId);
};


class Server
{
private:
    NetLib	        *m_netLib;

    LList           <ServerToClientLetter *> m_history;

public:
    int             m_sequenceId;

    DArray          <ServerToClient *> m_clients;
    DArray          <ServerTeam *> m_teams;

    NetMutex         *m_inboxMutex;
    NetMutex         *m_outboxMutex;
    LList           <NetworkUpdate *> m_inbox;
    LList           <ServerToClientLetter *> m_outbox;

    DArray          <unsigned char> m_sync;                                     // Synchronisation values for each sequenceId

public:
    Server();
    ~Server();

    void Initialise			();

    NetworkUpdate *GetNextLetter();

    void ReceiveLetter      ( NetworkUpdate *update, char *fromIP );
    void SendLetter         ( ServerToClientLetter *letter );

	int  GetClientId        ( const char *_ip );
	void RegisterNewClient  ( const char *_ip );
	void RemoveClient       ( const char *_ip );
	void RegisterNewTeam    ( const char *_ip, int _teamType, int _desiredTeamId );

	void AdvanceSender		();
    void Advance			();

	void LoadHistory        ( const char *_filename );
	void SaveHistory        ( const char *_filename );

    static int   ConvertIPToInt( const char *_ip );
    static char *ConvertIntToIP( const int _ip );
};


#endif

