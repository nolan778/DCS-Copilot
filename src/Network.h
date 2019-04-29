/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Header:       Network.h
Author:       Cory Parks
Date started: 11/2016

See LICENSE file for copyright and license information

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef NETWORK_H
#define NETWORK_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <string>
#include <vector>

#include "NetworkTypes.h"

#include "RakNetTypes.h"
#include "RakString.h"

#include <QObject>
#include <QString>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace Network {
    static const int MIN_CLIENTS = 1;
    static const int MAX_CLIENTS = 64;
    static const int MIN_PORT = 1024;
    static const int MAX_PORT = 65535;
    static const int MIN_TICK_TIME_MS = 5;
    static const int MAX_TICK_TIME_MS = 30;
    static const int MIN_TIMEOUT_TIME_MS = 1;
    static const int MAX_TIMEOUT_TIME_MS = 999999999;
    static const unsigned long long MIN_SPEED_BITS = 8192; //8kbps
    static const unsigned long long MAX_SPEED_BITS = 1073741824; //1gbps

    static const int MAX_CLIENT_NAME_LENGTH = 32;
}

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class MainWindow;

namespace Network {
struct ServerConfig
{
    int max_clients = 8;
    int tick_time_ms = 10;
    int timeout_time_ms = 10000;
    int port = 39640;
};

struct ClientInfo
{
    RakNet::RakNetGUID ID = RakNet::UNASSIGNED_RAKNET_GUID;
    std::string name = "Unknown Client";
    int ping = -1;
};


struct Client
{
    RakNet::RakNetGUID ID = RakNet::UNASSIGNED_RAKNET_GUID;
    RakNet::RakString name = "Unknown Client";
    int seatNumber = 0;
    int ping = -1;
};


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Network class wrapper using RakNet library.

@author Cory Parks
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class Network : public QObject
{    
    Q_OBJECT

signals:
    void receivedSeatChange(int seatNumber);
    void receivedNetCommand(int command);
    void receivedNetCommandValue(int command, float value, bool reliable);

public slots:
    void handleLocalConnected();
    void handleReceivedLocalCommand(int command);
    void handleReceivedLocalCommandValue(int command, float value, bool reliable);

public:
    /// Constructor
    Network(MainWindow* window_);
    /// Destructor
    ~Network();

    ///Main update function to call each frame to capture all received network packets since the last call
    void update();

    ///Attempt to start the network server as host with the given port, client name string, and given password string.
    ///Returns true if successful.
    bool startServer(unsigned short port, std::string clientName, std::string password = "");

    ///Attempt to connect to the server as a client with the given server ip, port, client name, and password.
    ///Returns true if attempt was successful, but this does not indicate a connection was made.
    bool connect(const char *ip, unsigned short port, std::string clientName, std::string password = "");

    ///Cancel a pending connection attempt
    void cancelConnect();

    ///If connected to a server, attempt to disconnect.
    void disconnect();

    ///Request the server to change seats
    void requestSeat(int seatNumber);

    ///Ping a remote unconnected system.
    bool ping(const char *ip, unsigned short port);

    ///////////////////
    // GET FUNCTIONS //
    ///////////////////

    ///Returns the network status of the peer
    ConnectionState getNetworkStatus() const;

    ///Returns boolean indicating whether the network instance is acting as the host or not.
    bool isHost() const;

    ///Returns boolean indicating whether the network instance is acting as a currently connected client or not.
    bool isClient() const;

    ///Returns the string containing the Server IP and Port. Ex: "127.0.0.1:34640".
    ///Returns "localhost:<Port>" if the client is the host and returns "N/A" if not connected.
    std::string getServerAddress() const;

    ///Returns the total amount of time elapsed in milliseconds since the connection was established.
    RakNet::Time getConnectionTime();

    ///Returns the number of clients currently connected to the server.  0 if not connected.
    int getNumClients() const;

    ///Returns the ping of the local client in milliseconds.  -1 if not connected.
    int getMyPing() const;

    ///Returns the tick time in milliseconds for the currently connected server.  -1 if not connected.
    int getServerTickTimeMS() const;

    ///Returns the maximum number of connected clients for the currently connected server.  0 if not connected.
    int getMaxClients() const;

    ///Returns the total packet loss of the local client from 0.0 (0.0%) to 1.0 (100.0%)
    float getMyPacketLoss();

    ///Returns the packet loss of the local client from the last second from 0.0 (0.0%) to 1.0 (100.0%)
    float getMyRecentPacketLoss();

    ///Returns the total bandwidth used in bytes
    uint64_t getBandwidth(ConnectionMetrics metric);

    ///Returns the bandwidth used from the last second in bytes
    uint64_t getRecentBandwidth(ConnectionMetrics metric);

    ///Returns the name of a client from the unique client ID number.
    ///Returns a null pointer if a client could not be found.
    std::string getClientNameByGUID(RakNet::RakNetGUID guid) const;

    ///Returns the client data structure for the client found with the given GUID
    Client* getClientByGUID(RakNet::RakNetGUID guid) const;

    ///Returns my client ID
    RakNet::RakNetGUID getMyGUID() const;

    ///Returns my client name string
    const std::string &getMyClientName() const;

    ///Returns my client's max outgoing speed per connection in bits per second
    unsigned long long Network::getMyMaxOutgoingSpeedPerConnection() const;

    ///Returns my server configuration
    const ServerConfig &getMyServerConfig() const;

    ///Returns a vector list of all connected clients' info (Does not contain IP Address)
    const std::vector<ClientInfo>& getClientInfo() const;


    ///////////////////
    // SET FUNCTIONS //
    ///////////////////

    ///Sets my client name string
    void setMyClientName(std::string clientName);

    ///Sets the server timeout time in ms
    void setTimeoutTimeMS(int time_ms);

    ///Sets the maximum number of simultaneously connected clients for the server
    void setMaxClients(int num_clients);

    ///Sets my client's max outgoing speed per connection in bits per second (default = 256 kbps)
    void setMyMaxOutgoingSpeedPerConnection(unsigned long long bits_per_second);

private:

    //void RakNetThreadUpdate(RakNet::RakPeerInterface *peer, void* data);
    unsigned short getNextClientIndex(unsigned short index);
    RakNet::RakNetGUID Network::getNextClientGUID(RakNet::RakNetGUID guid);

    RakNet::RakString readBitStreamString(RakNet::Packet *packet);
    const char* readBitStreamCharArray(RakNet::Packet *packet);

    void writeOutput(const QString& q) const;
    void updateServerStatus(int status) const;

    MainWindow* window = nullptr;

    // Make this object be noncopyable because it holds a pointer
    Network(const Network&);
    const Network &operator =(const Network &);

    class Impl;
    Impl* mImpl;
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

} // namespace Network

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif // NETWORK_H
