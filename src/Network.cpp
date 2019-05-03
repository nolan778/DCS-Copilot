/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module:       Network.cpp
Author:       Cory Parks
Date started: 09/2015
Purpose:      Network Class

See LICENSE file for copyright and license information

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
Network class handles network communication between DCS Copilot client applications.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "Network.h"
#include <iostream>
#include <sstream>

#include <string>
#include <map>
#include <array>
#include <bitset>

#include "RakPeerInterface.h"
#include "RakNetStatistics.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "GetTime.h"

#include "mainwindow.h"

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

enum CustomNetworkMessages
{
    ID_NET_MESSAGE_1 = ID_USER_PACKET_ENUM + 1,
    ID_NET_VERSION_MISMATCH,
    ID_NET_CLIENT_CONNECTED_NAME,
    ID_NET_CLIENT_CONNECTED_BROADCAST,
    ID_NET_CLIENT_DISCONNECTED_BROADCAST,
    ID_NET_CLIENT_LOST_CONNECTION_BROADCAST,
    ID_NET_CLIENT_LIST,
    ID_NET_CLIENT_INFO,
    ID_NET_CLIENT_SEAT_REQUEST,
    ID_NET_CLIENT_SEAT_BROADCAST,
    ID_NET_COMMAND,
    ID_NET_COMMAND_VALUE,
    ID_NET_CORRECTION_COMMAND_VALUE,
    ID_NET_EVENT,
};

namespace Network {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

const float Network::maxValueRate = 100.0f;

class Network::Impl
{
public:

    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    Impl()
    {
        peer = RakNet::RakPeerInterface::GetInstance();
        peer->SetOccasionalPing(true);
        myGUID = peer->GetMyGUID();

        hostClientIndexList.fill(RakNet::UNASSIGNED_RAKNET_GUID);
        currentTime = RakNet::GetTime();
        pingTimeCtr = currentTime;
        hostPingTimeCtr = currentTime;
        myStatistics = new RakNet::RakNetStatistics;
    }

    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    ~Impl()
    {
        delete myStatistics;

        RakNet::RakPeerInterface::DestroyInstance(peer);
    }

    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    void resetServerInfo()
    {
        isHost = false;
        serverGUID = RakNet::UNASSIGNED_RAKNET_GUID;

        clientNameMap.clear();
        clientMap.clear();
        clientInfoList.clear();

        for (int i = 0; i < MAX_CLIENTS; i++) {
            hostClientIndexList[i] = RakNet::UNASSIGNED_RAKNET_GUID;
        }
    }

    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    //remove the given client from lists and return the client's name
    std::string removeClient(RakNet::RakNetGUID guid)
    {
        {
            auto it = clientMap.find(guid);
            if (it != clientMap.end()) {
                //client found in list, so remove
                clientMap.erase(it);
            }
        }
        std::string clientNameStr = "Unknown Client";
        {
            auto it = clientNameMap.find(guid);
            if (it != clientNameMap.end()) {
                //client found in list, so remove
                clientNameStr = it->second;
                clientNameMap.erase(it);
            }
        }

        return clientNameStr;
    }

    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    void addClient(Client client)
    {
        clientMap.insert(std::pair<RakNet::RakNetGUID, Client>(client.ID, client));

        if (client.name != "Unknown Client")
            clientNameMap.insert(std::pair<RakNet::RakNetGUID, std::string>(client.ID, std::string(client.name.C_String())));
    }

    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    RakNet::RakPeerInterface *peer = nullptr;
    RakNet::Packet *packet = nullptr;

    RakNet::SystemAddress currentConnectionAttemptAddress;
    RakNet::SystemAddress serverAddress;

    RakNet::RakNetGUID serverGUID = RakNet::UNASSIGNED_RAKNET_GUID;

    bool isAttemptingConnection = false;
    bool isHost = false;
    bool statisticsUpdated = false;
    RakNet::RakNetGUID myGUID = RakNet::UNASSIGNED_RAKNET_GUID;
    int mySeat = 0;
    int myPing = -1;
    RakNet::RakNetStatistics *myStatistics;

    ServerConfig serverConfig;
    std::string client_name = "UnknownClient";
    unsigned long long max_outgoing_speed_per_connection = 256 * 1024; //256kbps

    RakNet::Time pingTimeCtr;
    RakNet::Time hostPingTimeCtr;
    RakNet::Time currentTime;
    RakNet::Time serverStartTime;
    unsigned short lastClientIndexUpdated = MAX_CLIENTS - 1;

    ConnectionState lastStatus = IS_NOT_CONNECTED;
    ConnectionState currentStatus = IS_NOT_CONNECTED;

    double modelTime = 0;

    //live information about the connected server and its clients
    std::map<RakNet::RakNetGUID, std::string> clientNameMap;
    std::map<RakNet::RakNetGUID, Client> clientMap;
    std::vector<ClientInfo> clientInfoList; //updated for each client in ID_NET_CLIENT_INFO packet

    //special for host so that GUID can be determined from an already disconnected client
    std::array<RakNet::RakNetGUID, MAX_CLIENTS> hostClientIndexList;

};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Network::Network(MainWindow* window_) : mImpl(new Impl)
{
    window = window_;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Network::~Network()
{
    delete mImpl;
    mImpl = 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool Network::ping(const char *ip, unsigned short port)
{
    if (mImpl->peer)
        return mImpl->peer->Ping(ip, port, false);

    return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Network::writeOutput(const QString& q) const
{
    std::cout << q.toStdString().c_str() << std::endl;
    window->logMessage(q);
}

void Network::updateServerStatus(int status) const
{
    window->updateServerStatus(status);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool Network::startServer(unsigned short port, std::string clientName, std::string password)
{
    //cancel any ongoing connections
    mImpl->peer->Shutdown(100);
    mImpl->resetServerInfo();

    mImpl->client_name = clientName;
    mImpl->serverConfig.port = port;

    RakNet::SocketDescriptor sd(mImpl->serverConfig.port, 0);
    int maxClients = mImpl->serverConfig.max_clients;
    RakNet::StartupResult result = mImpl->peer->Startup(maxClients, mImpl->serverConfig.tick_time_ms, &sd, 1);

    if (result == RakNet::RAKNET_STARTED) {

        writeOutput(QString("Server started successfully on port: %1").arg(mImpl->serverConfig.port));
        //QLabel* label =  window->findChild<QLabel*>("Listener_status_label");
        updateServerStatus(SS_HOSTING);




        mImpl->isHost = true;
        mImpl->serverGUID = mImpl->myGUID;
        mImpl->peer->SetMaximumIncomingConnections(mImpl->serverConfig.max_clients);
        //mImpl->peer->SetPerConnectionOutgoingBandwidthLimit(mImpl->max_outgoing_speed_per_connection);
        const char* pwd = 0;
        if (password.length() > 0)
            pwd = password.c_str();

        mImpl->peer->SetIncomingPassword(pwd, (int)password.length());
        mImpl->peer->SetTimeoutTime(mImpl->serverConfig.timeout_time_ms, RakNet::UNASSIGNED_SYSTEM_ADDRESS);

        mImpl->serverStartTime = RakNet::GetTime();

        //add self
        Client client;
        client.ID = mImpl->myGUID;
        client.name = mImpl->client_name.c_str();
        client.ping = -1;

        mImpl->clientMap.insert(std::pair<RakNet::RakNetGUID, Client>(client.ID, client));        

        std::string clientListName = mImpl->client_name.c_str();
        clientListName += " (Host)";

        window->addClient(client.ID.ToString(), clientListName.c_str());

        window->setServerIP(getServerAddress().c_str());
        window->setMaxSeats(getMaxClients());

        return true;
    }
    else {

        switch (result)
        {
        case RakNet::RAKNET_ALREADY_STARTED:
            writeOutput("<font color='red'>ERROR:</font> Server already started.");
            break;
        case RakNet::INVALID_SOCKET_DESCRIPTORS:
            writeOutput("<font color='red'>ERROR:</font> Server socket descriptors invalid.");
            break;
        case RakNet::INVALID_MAX_CONNECTIONS:
            writeOutput(QString("<font color='red'>ERROR:</font> Server max connections invalid: %1").arg(mImpl->serverConfig.max_clients));
            break;
        case RakNet::SOCKET_FAMILY_NOT_SUPPORTED:
            writeOutput("<font color='red'>ERROR:</font> Server socket family not supported.");
            break;
        case RakNet::SOCKET_PORT_ALREADY_IN_USE:
            writeOutput(QString("<font color='red'>ERROR:</font> Server socket port already in use: %1").arg(mImpl->serverConfig.port));
            break;
        case RakNet::SOCKET_FAILED_TO_BIND:
            writeOutput("<font color='red'>ERROR:</font> Server socket failed to bind.");
            break;
        case RakNet::SOCKET_FAILED_TEST_SEND:
            writeOutput("<font color='red'>ERROR:</font> Server socket failed test send.");
            break;
        case RakNet::PORT_CANNOT_BE_ZERO:
            writeOutput("<font color='red'>ERROR:</font> Server port cannot be zero.");
            break;
        case RakNet::FAILED_TO_CREATE_NETWORK_THREAD:
            writeOutput("<font color='red'>ERROR:</font> Server failed to create network thread.");
            break;
        case RakNet::COULD_NOT_GENERATE_GUID:
            writeOutput("<font color='red'>ERROR:</font> Server could not generate GUID.");
            break;
        case RakNet::STARTUP_OTHER_FAILURE:
            writeOutput("<font color='red'>ERROR:</font> Server - Other failure.");
            break;
        default:
            writeOutput("<font color='red'>ERROR:</font> Server - Unknown startup error.");
        }
        updateServerStatus(SS_NOT_CONNECTED);
    }

    return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool Network::connect(const char *ip, unsigned short port, std::string clientName, std::string password)
{
    if (!mImpl->isAttemptingConnection) {
        //cancel any ongoing connections
        mImpl->peer->Shutdown(100);
        mImpl->resetServerInfo();


        RakNet::SocketDescriptor sd;
        mImpl->peer->Startup(1, 10, &sd, 1);

        mImpl->client_name = clientName;


        writeOutput("Starting the network client ...");
        mImpl->currentConnectionAttemptAddress = RakNet::SystemAddress(ip, port);
        RakNet::ConnectionAttemptResult result = mImpl->peer->Connect(ip, port, password.c_str(), (int)password.length());

        if (result == RakNet::CONNECTION_ATTEMPT_STARTED) {

            writeOutput(QString("Client connection attempt started for server: %1:%2").arg(ip, QString::number(port)));
            //mImpl->peer->SetPerConnectionOutgoingBandwidthLimit(mImpl->max_outgoing_speed_per_connection);
            mImpl->isAttemptingConnection = true;
            updateServerStatus(SS_IS_CONNECTING);
            return true;
        }
        else {
            mImpl->isAttemptingConnection = false;
            switch (result) {
            case RakNet::INVALID_PARAMETER:
                writeOutput("<font color='red'>ERROR:</font> Client - Invalid parameter.");
                break;
            case RakNet::CANNOT_RESOLVE_DOMAIN_NAME:
                writeOutput("<font color='red'>ERROR:</font> Client cannot resolve domain name.");
                break;
            case RakNet::ALREADY_CONNECTED_TO_ENDPOINT:
                writeOutput("<font color='red'>ERROR:</font> Client already connected to endpoint.");
                break;
            case RakNet::CONNECTION_ATTEMPT_ALREADY_IN_PROGRESS:
                writeOutput("<font color='red'>ERROR:</font> Client connection attempt already in progress.");
                break;
            case RakNet::SECURITY_INITIALIZATION_FAILED:
                writeOutput("<font color='red'>ERROR:</font> Client - Security initialization error.");
                break;
            default:
                writeOutput("<font color='red'>ERROR:</font> Client - Unknown client error.");
            }
        }
    }

    return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Network::cancelConnect()
{
    //cancel any ongoing connection attempts
    mImpl->peer->CancelConnectionAttempt(mImpl->currentConnectionAttemptAddress);
    updateServerStatus(SS_NOT_CONNECTED);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Network::disconnect()
{
    mImpl->peer->Shutdown(300);
    updateServerStatus(SS_NOT_CONNECTED);
    writeOutput("Disconnecting from the server ...");
    mImpl->resetServerInfo();
    window->clearClients();
    window->resetStatistics();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Network::requestSeat(int seatNumber)
{
    if (mImpl->isHost) {
        if (seatNumber <= getMaxClients()) {
            //Host has authority to grant its own permission

            auto it = mImpl->clientMap.find(mImpl->myGUID);
            if (it != mImpl->clientMap.end()) {
                bool seatRequestAccepted = true;
                if (it->second.seatNumber == seatNumber) {
                    seatRequestAccepted = false;
                }
                else if (seatNumber > 0) {
                    for (auto& it2 : mImpl->clientMap)
                    {
                        if (it2.second.seatNumber == seatNumber) {
                            //Someone already occupying the seat requested
                            seatRequestAccepted = false;
                            break;
                        }
                    }
                }

                if (seatRequestAccepted) {
                    it->second.seatNumber = seatNumber;
                    emit receivedSeatChange(seatNumber);
                    mImpl->mySeat = seatNumber;

                    window->setSeat(mImpl->myGUID.ToString(), seatNumber);

                    //Inform all clients except ourselves
                    {
                        RakNet::BitStream bsOut;
                        bsOut.Write((RakNet::MessageID)ID_NET_CLIENT_SEAT_BROADCAST);
                        bsOut.Write(mImpl->myGUID);
                        bsOut.WriteBitsFromIntegerRange(seatNumber, 0, MAX_CLIENTS);
                        mImpl->peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mImpl->serverAddress, true);
                    }
                }
            }
        }
    }
    else {
        bool sendRequest = true;
        //Do not send request if same as current seat
        if (mImpl->mySeat == seatNumber) {
            sendRequest = false;
        }

        if (sendRequest) {
            //Request seat change from the server
            RakNet::BitStream bsOut;
            bsOut.Write((RakNet::MessageID)ID_NET_CLIENT_SEAT_REQUEST);
            bsOut.WriteBitsFromIntegerRange(seatNumber, 0, MAX_CLIENTS);
            mImpl->peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mImpl->serverAddress, false);
        }
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

RakNet::RakString Network::readBitStreamString(RakNet::Packet *packet)
{
    RakNet::RakString rs;
    RakNet::BitStream bsIn(packet->data, packet->length, false);
    bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
    bsIn.Read(rs);
    return rs;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const char* Network::readBitStreamCharArray(RakNet::Packet *packet)
{
    return readBitStreamString(packet).C_String();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Network::update()
{
    //shortcuts
    RakNet::Packet* packet = mImpl->packet;
    RakNet::RakPeerInterface* peer = mImpl->peer;

    //packet checking loop
    for (packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
    {
        mImpl->isAttemptingConnection = false;
        switch (packet->data[0])
        {
        case ID_UNCONNECTED_PONG:
        {
            //unsigned int dataLength;
            RakNet::TimeMS time;
            RakNet::BitStream bsIn(packet->data, packet->length, false);
            bsIn.IgnoreBytes(1);
            bsIn.Read(time);
            //dataLength = packet->length - sizeof(unsigned char) - sizeof(RakNet::TimeMS);
            //printf("ID_UNCONNECTED_PONG from SystemAddress %s.\n", packet->systemAddress.ToString(true));
            //printf("Time is %i\n", time);
            writeOutput(QString("Ping: %1 ms").arg((unsigned int)(RakNet::GetTimeMS() - time)));
            //printf("Data is %i bytes long.\n", dataLength);
            //if (dataLength > 0)
            //	printf("Data is %s\n", packet->data + sizeof(unsigned char) + sizeof(RakNet::TimeMS));

            break;
        }
        case ID_CONNECTED_PING:
            //Core::FSLog::message("Ping from %s", packet->systemAddress.ToString(true));
            break;
        case ID_UNCONNECTED_PING:
            //Core::FSLog::message("Ping from %s", packet->systemAddress.ToString(true));
            break;
        case ID_CONNECTION_REQUEST_ACCEPTED:
        {
            writeOutput("CONNECTION SUCCESS: Connection request has been accepted.");
            updateServerStatus(SS_CONNECTED);

            //add myself to client map
            Client client;
            client.ID = mImpl->myGUID;
            client.name = mImpl->client_name.c_str();
            mImpl->clientMap.insert(std::pair<RakNet::RakNetGUID, Client>(client.ID, client));
            mImpl->clientNameMap.insert(std::pair<RakNet::RakNetGUID, std::string>(client.ID, std::string(client.name.C_String())));

            window->addClient(client.ID.ToString(), client.name.C_String());

            mImpl->serverAddress = mImpl->currentConnectionAttemptAddress;
            mImpl->serverGUID = mImpl->peer->GetGuidFromSystemAddress(mImpl->serverAddress);

            //pass the server our name
            RakNet::BitStream bsOut;
            bsOut.Write((RakNet::MessageID)ID_NET_CLIENT_CONNECTED_NAME);
            bsOut.Write(mImpl->client_name.c_str());
            peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
            break;
        }
        case ID_NEW_INCOMING_CONNECTION:
        {
            //received by the host only
            if (mImpl->isHost) {
                Client client;
                client.ID = peer->GetGuidFromSystemAddress(packet->systemAddress);

                mImpl->addClient(client);

                //mImpl->clientMap.insert(std::pair<RakNet::RakNetGUID, Client>(client.ID, client));

                //mImpl->newClientConnectList.push_back(client.ID);

                int index = peer->GetIndexFromSystemAddress(packet->systemAddress);
                mImpl->hostClientIndexList[index] = client.ID;
                writeOutput("A client has connected.");
                //////////////////////////////////////////
                //inform new client of all other clients
                RakNet::BitStream bsOut;
                bsOut.Write((RakNet::MessageID)ID_NET_CLIENT_LIST);
                unsigned short numberOfClients = (unsigned short)mImpl->clientMap.size();
                bsOut.Write(numberOfClients);
                for (auto& it : mImpl->clientMap) {
                    Client *client = &it.second;
                    bsOut.Write(client->ID);
                    bsOut.Write(client->name);
                    bsOut.WriteBitsFromIntegerRange(client->seatNumber, 0, MAX_CLIENTS);
                }

                peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
            }
            break;
        }
        case ID_CONNECTION_ATTEMPT_FAILED:
            writeOutput("<font color='red'>ERROR:</font> CONNECTION FAILURE: Unable to connect to server.");
            updateServerStatus(SS_NOT_CONNECTED);
            break;
        case ID_ALREADY_CONNECTED:
            writeOutput("<font color='red'>ERROR:</font> CONNECTION FAILURE: Already connected to server.");
            updateServerStatus(SS_NOT_CONNECTED);
            break;
        case ID_NO_FREE_INCOMING_CONNECTIONS:
            writeOutput("<font color='red'>ERROR:</font> CONNECTION FAIULRE: Server is full.");
            updateServerStatus(SS_NOT_CONNECTED);
            break;
        case ID_CONNECTION_BANNED:
            writeOutput("<font color='red'>ERROR:</font> CONNECTION FAIULRE: You are banned from this server.");
            updateServerStatus(SS_NOT_CONNECTED);
            break;
        case ID_INVALID_PASSWORD:
            writeOutput("<font color='red'>ERROR:</font> CONNECTION FAIULRE: Wrong password!");
            updateServerStatus(SS_NOT_CONNECTED);
            break;
        case ID_INCOMPATIBLE_PROTOCOL_VERSION:
            writeOutput("<font color='red'>ERROR:</font> CONNECTION FAILURE: Incompatible Protocol Version with the server.");
            updateServerStatus(SS_NOT_CONNECTED);
            break;
        case ID_DISCONNECTION_NOTIFICATION:
        {
            if (mImpl->isHost) {
                int index = peer->GetIndexFromSystemAddress(packet->systemAddress);
                RakNet::RakNetGUID guid = mImpl->hostClientIndexList[index];
                mImpl->hostClientIndexList[index] = RakNet::UNASSIGNED_RAKNET_GUID;

                //mImpl->newClientDisconnectList.push_back(guid);

                if (guid != RakNet::UNASSIGNED_RAKNET_GUID) {
                    std::string clientNameStr = mImpl->removeClient(guid);

                    window->removeClient(guid.ToString());

                    writeOutput(QString("\"%1\" has disconnected - GUID: %2").arg(clientNameStr.c_str(), guid.ToString()));
                    //////////////////////////////////////////
                    //inform all clients of the disconnection
                    RakNet::BitStream bsOut;
                    bsOut.Write((RakNet::MessageID)ID_NET_CLIENT_DISCONNECTED_BROADCAST);
                    bsOut.Write(guid);
                    peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
                }
            }
            else {
                mImpl->resetServerInfo();
                writeOutput("Disconnected from the server.");
                updateServerStatus(SS_NOT_CONNECTED);
                window->clearClients();
                window->resetStatistics();
            }
            break;
        }
        case ID_CONNECTION_LOST:
        {
            if (mImpl->isHost) {
                int index = peer->GetIndexFromSystemAddress(packet->systemAddress);
                RakNet::RakNetGUID guid = mImpl->hostClientIndexList[index];
                mImpl->hostClientIndexList[index] = RakNet::UNASSIGNED_RAKNET_GUID;

                //mImpl->newClientDisconnectList.push_back(guid);

                if (guid != RakNet::UNASSIGNED_RAKNET_GUID) {
                    std::string clientNameStr = mImpl->removeClient(guid);

                    window->removeClient(guid.ToString());

                    writeOutput(QString("\"%1\" has lost the connection - GUID: %2").arg(clientNameStr.c_str(), guid.ToString()));
                    //////////////////////////////////////////
                    //inform all clients of the lost connection
                    RakNet::BitStream bsOut;
                    bsOut.Write((RakNet::MessageID)ID_NET_CLIENT_LOST_CONNECTION_BROADCAST);
                    bsOut.Write(guid);
                    peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
                }
            }
            else {
                mImpl->resetServerInfo();
                writeOutput("<font color='red'>ERROR:</font> Connection to the server was lost.");
                updateServerStatus(SS_NOT_CONNECTED);
                window->clearClients();
                window->resetStatistics();
            }
            break;
        }

        ////////////////////////////////////////////////
        ///////////    CUSTOM MESSAGE IDS    ///////////
        ////////////////////////////////////////////////

        case ID_NET_CLIENT_CONNECTED_NAME:
        {
            //received by the host only
            if (mImpl->isHost) {
                int clientIndex = peer->GetIndexFromSystemAddress(packet->systemAddress);
                if (clientIndex >= 0) {
                    RakNet::RakString rs = readBitStreamString(packet);
                    const char * clientName = rs.C_String();
                    std::string clientNameStr = std::string(clientName);
                    RakNet::RakNetGUID guid = peer->GetGuidFromSystemAddress(packet->systemAddress);

                    mImpl->clientNameMap.insert(std::pair<RakNet::RakNetGUID, std::string>(guid, clientNameStr));

                    Client *client = getClientByGUID(guid);
                    if (client) {
                        client->name = clientNameStr.c_str();
                    }

                    window->addClient(guid.ToString(), clientNameStr.c_str());

                    writeOutput(QString("\"%1\" has joined the server - IP: %2    GUID: %3").arg(clientName, packet->systemAddress.ToString(false), guid.ToString()));

                    //////////////////////////////////////////
                    //inform all clients of the new client connection with name and GUID
                    {
                        RakNet::BitStream bsOut;
                        bsOut.Write((RakNet::MessageID)ID_NET_CLIENT_CONNECTED_BROADCAST);
                        bsOut.Write(guid);
                        bsOut.Write(rs);
                        peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
                    }
                }
            }
            break;
        }
        case ID_NET_MESSAGE_1:
        {
            writeOutput(QString("%1").arg(readBitStreamCharArray(packet)));
            break;
        }
        case ID_NET_CLIENT_CONNECTED_BROADCAST:
        {
            //received by clients only
            if (!mImpl->isHost) {
                Client client;
                RakNet::RakString rs;
                RakNet::BitStream bsIn(packet->data, packet->length, false);
                bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
                bsIn.Read(client.ID);
                bsIn.Read(rs);
                const char * clientName = rs.C_String();
                client.name = rs;

                mImpl->addClient(client);

                window->addClient(client.ID.ToString(), clientName);

                writeOutput(QString("\"%1\" has joined the server - GUID: %2").arg(clientName, client.ID.ToString()));
            }
            break;
        }
        case ID_NET_CLIENT_DISCONNECTED_BROADCAST:
        {
            //received by clients only
            if (!mImpl->isHost) {
                RakNet::RakNetGUID guid;
                RakNet::BitStream bsIn(packet->data, packet->length, false);
                bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
                bsIn.Read(guid);

                //mImpl->newClientDisconnectList.push_back(guid);

                std::string clientNameStr = mImpl->removeClient(guid);

                window->removeClient(guid.ToString());

                writeOutput(QString("\"%1\" has disconnected - GUID: %2").arg(clientNameStr.c_str(), guid.ToString()));
            }
            break;
        }
        case ID_NET_CLIENT_LOST_CONNECTION_BROADCAST:
        {
            //received by clients only
            if (!mImpl->isHost) {
                RakNet::RakNetGUID guid;
                RakNet::BitStream bsIn(packet->data, packet->length, false);
                bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
                bsIn.Read(guid);

                //mImpl->newClientDisconnectList.push_back(guid);

                std::string clientNameStr = mImpl->removeClient(guid);

                window->removeClient(guid.ToString());

                writeOutput(QString("\"%1\" has lost connection - GUID: %2").arg(clientNameStr.c_str(), guid.ToString()));
            }
            break;
        }
        case ID_NET_CLIENT_LIST:
        {
            //received by clients only
            if (!mImpl->isHost) {
                RakNet::BitStream bsIn(packet->data, packet->length, false);
                bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
                unsigned short numberOfClients;
                bsIn.Read(numberOfClients);
                for (unsigned short us = 0; us < numberOfClients; us++) {
                    Client client;
                    bsIn.Read(client.ID);
                    bsIn.Read(client.name);
                    bsIn.ReadBitsFromIntegerRange(client.seatNumber, 0, MAX_CLIENTS);

                    //skip my own client, as this is stored at connection
                    if (client.ID != mImpl->myGUID) {
                        mImpl->clientMap.insert(std::pair<RakNet::RakNetGUID, Client>(client.ID, client));
                        mImpl->clientNameMap.insert(std::pair<RakNet::RakNetGUID, std::string>(client.ID, std::string(client.name.C_String())));

                        std::string clientListName = client.name.C_String();
                        if (client.ID == mImpl->serverGUID) {
                            clientListName += " (Host)";
                        }

                        window->addClient(client.ID.ToString(), clientListName.c_str(), client.seatNumber);
                    }
                }

                window->setServerIP(getServerAddress().c_str());
                //window->setMaxSeats(getMaxClients());
            }
            break;
        }
        case ID_NET_CLIENT_INFO:
        {
            //received by clients only
            if (!mImpl->isHost) {
                mImpl->clientInfoList.clear();
                RakNet::BitStream bsIn(packet->data, packet->length, false);
                bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
                unsigned short numberOfClients;
                bsIn.Read(numberOfClients);
                //printf("\n\n\n-------------------\n");
                //printf("Client Information:\n");
                //printf("-------------------\n");
                //printf(" # Name (ID)\t\t\tPing (ms)\n");
                for (unsigned short us = 0; us < numberOfClients; us++) {
                    ClientInfo clientInfo;
                    bsIn.Read(clientInfo.ID);
                    bsIn.ReadBitsFromIntegerRange(clientInfo.ping, -1, 2046);
                    if (clientInfo.ping >= 2046) clientInfo.ping = 9999;
                    clientInfo.name = getClientNameByGUID(clientInfo.ID);
                    mImpl->clientInfoList.push_back(clientInfo);
                    //printf("[%hu] %s (%s)\t\t%d\n", us, clientInfo.name.c_str(), clientInfo.ID.ToString(), clientInfo.ping);

                    if (clientInfo.ID == mImpl->myGUID) {
                        mImpl->myPing = clientInfo.ping;
                        window->setMyPing(mImpl->myPing);
                    }

                    //add latest ping to client map info
                    auto it = mImpl->clientMap.find(clientInfo.ID);
                    if (it != mImpl->clientMap.end()) {
                        it->second.ping = clientInfo.ping;
                    }

                    window->setPing(clientInfo.ID.ToString(), clientInfo.ping);
                }
                //for (auto fncn : mImpl->clientUpdateCallbackFunctions) {
                //    fncn->run();
                //}
            }
            break;
        }
        case ID_NET_CLIENT_SEAT_REQUEST:
        {
            //received by the host only
            if (mImpl->isHost) {
                int index = peer->GetIndexFromSystemAddress(packet->systemAddress);
                RakNet::RakNetGUID guid = mImpl->hostClientIndexList[index];

                RakNet::BitStream bsIn(packet->data, packet->length, false);
                bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
                int seatNumber = 0;
                bsIn.ReadBitsFromIntegerRange(seatNumber, 0, MAX_CLIENTS);
                writeOutput(QString("Seat Request (%1) by: ").arg(seatNumber)+guid.ToString());

                if (guid != RakNet::UNASSIGNED_RAKNET_GUID) {
                    //add latest seat info to client
                    if (seatNumber <= getMaxClients()) {
                        auto it = mImpl->clientMap.find(guid);
                        if (it != mImpl->clientMap.end()) {
                            bool seatRequestAccepted = true;
                            if (it->second.seatNumber == seatNumber) {
                                seatRequestAccepted = false;
                            }
                            else if (seatNumber > 0) {
                                for (auto& it2 : mImpl->clientMap)
                                {
                                    if (it2.second.seatNumber == seatNumber) {
                                        //Someone already occupying the seat requested
                                        seatRequestAccepted = false;
                                        break;
                                    }
                                }
                            }

                            if (seatRequestAccepted) {
                                it->second.seatNumber = seatNumber;
                                window->setSeat(guid.ToString(), seatNumber);

                                //Inform all clients, even the requestor
                                {
                                    RakNet::BitStream bsOut;
                                    bsOut.Write((RakNet::MessageID)ID_NET_CLIENT_SEAT_BROADCAST);
                                    bsOut.Write(guid);
                                    bsOut.WriteBitsFromIntegerRange(seatNumber, 0, MAX_CLIENTS);
                                    peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_RAKNET_GUID, true);
                                }
                            }
                        }
                    }
                }
            }
            break;
        }
        case ID_NET_CLIENT_SEAT_BROADCAST:
        {
            //received by clients only
            if (!mImpl->isHost) {
                RakNet::RakNetGUID guid;
                RakNet::BitStream bsIn(packet->data, packet->length, false);
                bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
                int seatNumber;
                bsIn.Read(guid);
                bsIn.ReadBitsFromIntegerRange(seatNumber, 0, MAX_CLIENTS);
                writeOutput(QString("Seat Change (%1) by: ").arg(seatNumber)+guid.ToString());

                if (guid == mImpl->myGUID) {
                    emit receivedSeatChange(seatNumber);
                    mImpl->mySeat = seatNumber;
                }

                window->setSeat(guid.ToString(), seatNumber);
            }
            break;
        }
        case ID_NET_COMMAND:
        {
            if (mImpl->mySeat > 0 || mImpl->isHost)
            {
                unsigned short command = 0;
                char orderingChannel = 0;
                unsigned char priority= 1;
                unsigned char reliability = 3;
                unsigned char packetInfo;

                RakNet::BitStream bsIn(packet->data, packet->length, false);

                bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
                bsIn.Read(orderingChannel);

                bsIn.Read(packetInfo);
                //bsIn.ReadBits((unsigned char *)(&(priorityChar)), 2);
                //bsIn.ReadBits((unsigned char *)(&(reliabilityChar)), 3);
                bsIn.Read(command);
                priority = READFROM(packetInfo,0,2);
                reliability = READFROM(packetInfo,2,3);

                emit receivedNetCommand(command);
                writeOutput(QString("Net Command (%1)").arg(command));

                if (mImpl->isHost) {
                    //pass along to all other clients except the sender
                    RakNet::BitStream bsOut;
                    bsOut.Write((RakNet::MessageID)ID_NET_COMMAND);
                    bsOut.Write(command);
                    bsOut.Write(orderingChannel);
                    //bsOut.WriteBits((const unsigned char *)&priorityChar, 2);
                    //bsOut.WriteBits((const unsigned char *)&reliabilityChar, 3);
                    //packetInfo = priorityChar | (unsigned char)(reliabilityChar << 2);
                    bsOut.Write(packetInfo);
                    mImpl->peer->Send(&bsOut, static_cast<PacketPriority>(priority), static_cast<PacketReliability>(reliability), orderingChannel, packet->systemAddress, true);
                }
            }
            break;
        }
       case ID_NET_COMMAND_VALUE:
       {
           if (mImpl->mySeat > 0 || mImpl->isHost)
           {
               unsigned short command = 0;
               unsigned char bitValue = 0;
               float value = 0.f;
               float valueRate = 0.f;
               char orderingChannel = 0;
               unsigned char priority;
               unsigned char reliability;
               unsigned char compressionTypeChar;
               unsigned char packetInfo;
               bool deadReckoned = false;

               RakNet::BitStream bsIn(packet->data, packet->length, false);
               bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
               bsIn.Read(orderingChannel);
               bsIn.Read(packetInfo);
               //bsIn.ReadBits((unsigned char *)(&(priorityChar)), 2);
               //bsIn.ReadBits((unsigned char *)(&(reliabilityChar)), 3);
               //bsIn.ReadBits((unsigned char *)(&(compressionTypeChar)), 3);
               bsIn.Read(command);
               priority = READFROM(packetInfo,0,2);
               reliability = READFROM(packetInfo,2,3);
               compressionTypeChar = READFROM(packetInfo,5,3);

               NetCompressionTypes compressionType = static_cast<NetCompressionTypes>(compressionTypeChar);

               switch (compressionType)
               {
                   case BINARY:
                       bitValue = bsIn.ReadBit();
                       value = (bitValue > 0) ? 1.0f : 0.0f;
                       break;
                   case FLOAT16:
                       bsIn.ReadFloat16(value, -1.0f, 1.0f);
                       if (bsIn.GetNumberOfUnreadBits() > 7) {
                           deadReckoned = true;
                           bsIn.ReadFloat16(valueRate, -maxValueRate, maxValueRate);
                           if (fabsf(valueRate) < 0.002f) {
                               valueRate = 0.0f;
                           }
                       }
                       break;
                   case FLOAT32:
                       bsIn.Read(value);
                       if (bsIn.GetNumberOfUnreadBits() > 7) {
                           deadReckoned = true;
                           bsIn.Read(valueRate);
                       }
                       break;
                   //case FLOAT64:
                   //	bsIn.Read(dValue);
                   //	break;
                   case NEG_ONE_ZERO_ONE:
                   {
                       bsIn.ReadBits((unsigned char *)(&(bitValue)), 2);
                       if (bitValue == 0) {
                           value = -1.0f;
                       }
                       else if (bitValue == 1) {
                           value = 0.0f;
                       }
                       else {
                           value = 1.0f;
                       }
                       break;
                   }
                   case ZERO_HALF_ONE:
                   {
                       bsIn.ReadBits((unsigned char *)(&(bitValue)), 2);
                       if (bitValue == 0) {
                           value = 0.0f;
                       }
                       else if (bitValue == 1) {
                           value = 0.5f;
                       }
                       else {
                           value = 1.0f;
                       }
                       break;
                   }
                   case NEG_ONE_ZERO_HALF_ONE:
                   {
                       bsIn.ReadBits((unsigned char *)(&(bitValue)), 3);
                       if (bitValue == 0) {
                           value = -1.0f;
                       }
                       else if (bitValue == 1) {
                           value = 0.0f;
                       }
                       else if (bitValue == 2) {
                           value = 0.5f;
                       }
                       else {
                           value = 1.0f;
                       }
                       break;
                   }
                   case NEG_TWO_NEG_ONE_ZERO_ONE:
                   {
                       bsIn.ReadBits((unsigned char *)(&(bitValue)), 3);
                       if (bitValue == 0) {
                           value = -2.0f;
                       }
                       else if (bitValue == 1) {
                           value = -1.0f;
                       }
                       else if (bitValue == 2) {
                           value = 0.0f;
                       }
                       else {
                           value = 1.0f;
                       }
                       break;
                   }
                   default:
                       bsIn.Read(value);
                       break;
               }

               emit receivedNetCommandValue(command, value, deadReckoned, valueRate);
               writeOutput(QString("Net Command (%1): ").arg(command)+QString::number((double)value)+(deadReckoned ? QString(", ") + QString::number((double)valueRate) : ""));

               if (mImpl->isHost) {
                   //pass along to all other clients except the sender
                   RakNet::BitStream bsOut;
                   bsOut.Write((RakNet::MessageID)ID_NET_COMMAND_VALUE);
                   bsOut.Write(orderingChannel);
                   //bsOut.WriteBits((const unsigned char *)&priorityChar, 2);
                   //bsOut.WriteBits((const unsigned char *)&reliabilityChar, 3);
                   //bsOut.WriteBits((const unsigned char *)&compressionTypeChar, 3);
                   //packetInfo = priorityChar | (unsigned char)(reliabilityChar << 2) | (unsigned char)(compressionTypeChar << 5);
                   bsOut.Write(packetInfo);
                   bsOut.Write(command);
                   switch (compressionType)
                   {
                       case BINARY:
                           value > 0.0f ? bsOut.Write1() : bsOut.Write0();
                           break;
                       case FLOAT16:
                           bsOut.WriteFloat16(value, -1.0f, 1.0f);
                           if (deadReckoned) {
                               bsOut.WriteFloat16(valueRate, -maxValueRate, maxValueRate);
                           }
                           break;
                       case FLOAT32:
                           bsOut.Write(value);
                           if (deadReckoned) {
                               bsOut.Write(valueRate);
                           }
                           break;
                       //case FLOAT64:
                       //	bsOut.Write(dValue);
                       //	break;
                       case NEG_ONE_ZERO_ONE:
                       {
                           unsigned char bitValue = 0;
                           if (value < 0.0f) {
                               bitValue = 0;
                           }
                           else if (value == 0.0f) {
                               bitValue = 1;
                           }
                           else {
                               bitValue = 2;
                           }
                           bsOut.WriteBits((const unsigned char *)&bitValue, 2);
                           break;
                       }
                       case ZERO_HALF_ONE:
                       {
                           unsigned char bitValue = 0;
                           if (value == 0.0f) {
                               bitValue = 0;
                           }
                           else if (value > 0.99999f) {
                               bitValue = 2;
                           }
                           else {
                               bitValue = 1;
                           }
                           bsOut.WriteBits((const unsigned char *)&bitValue, 2);
                           break;
                       }
                       case NEG_ONE_ZERO_HALF_ONE:
                       {
                           unsigned char bitValue = 0;
                           if (value < -0.99999f) {
                               bitValue = 0;
                           }
                           else if (value == 0.0f) {
                               bitValue = 1;
                           }
                           else if (value > 0.99999f) {
                               bitValue = 3;
                           }
                           else {
                               bitValue = 2;
                           }
                           bsOut.WriteBits((const unsigned char *)&bitValue, 3);
                           break;
                       }
                       case NEG_TWO_NEG_ONE_ZERO_ONE:
                       {
                           unsigned char bitValue = 0;
                           if (value < -1.99999f) {
                               bitValue = 0;
                           }
                           else if (value < -0.99999f) {
                               bitValue = 1;
                           }
                           else if (value == 0.0f) {
                               bitValue = 2;
                           }
                           else if (value > 0.99999f) {
                               bitValue = 3;
                           }
                           bsOut.WriteBits((const unsigned char *)&bitValue, 3);
                           break;
                       }
                       default:
                           bsOut.Write(value);
                           break;
                   }

                   mImpl->peer->Send(&bsOut, static_cast<PacketPriority>(priority), static_cast<PacketReliability>(reliability), orderingChannel, packet->systemAddress, true);
               }
           }
           break;
        }
        case ID_NET_CORRECTION_COMMAND_VALUE:
        {
            if (mImpl->mySeat > 0 || mImpl->isHost)
            {
                unsigned short command = 0;
                float value = 0.0f;

                RakNet::BitStream bsIn(packet->data, packet->length, false);

                bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
                bsIn.Read(command);
                bsIn.Read(value);

                emit receivedNetCommandValue(command, value, false, 0.0f);
                writeOutput(QString("Net Command (%1): ").arg(command)+QString::number((double)value)+" (Corrected)");

                if (mImpl->isHost) {
                    //pass along to all other clients except the sender
                    RakNet::BitStream bsOut;
                    bsOut.Write((RakNet::MessageID)ID_NET_CORRECTION_COMMAND_VALUE);
                    bsOut.Write(command);
                    bsOut.Write(value);
                    mImpl->peer->Send(&bsOut, LOW_PRIORITY, RELIABLE, 0, packet->systemAddress, true);
                }
            }
            break;
        }
        case ID_NET_EVENT:
        {
            if (mImpl->mySeat > 0 || mImpl->isHost)
            {
                unsigned char eventID = 0;

                RakNet::BitStream bsIn(packet->data, packet->length, false);

                bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
                bsIn.Read(eventID);

                emit receivedNetEvent(eventID);
                writeOutput(QString("Net Event (%1)").arg((int)eventID));

                if (mImpl->isHost) {
                    //pass along to all other clients except the sender
                    RakNet::BitStream bsOut;
                    bsOut.Write((RakNet::MessageID)ID_NET_EVENT);
                    bsOut.Write(eventID);
                    mImpl->peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, ORDERING_CHANNEL_EVENTS, packet->systemAddress, true);
                }
            }
            break;
        }
        default:
            writeOutput(QString("Message with identifier %1 has arrived.").arg(packet->data[0]));
            break;
        }
    }

    //Now loop through all the movement packets
    /*
    {
        for (auto& networkObject : mImpl->positionPackets)
        {
            if (networkObject.second.used == false)
            {
                networkObject.second.used = true;
                mImpl->moveNetworkObject(networkObject.second);

                //////////////////////////////////////////
                // pass on latest position packets to all clients
                if (mImpl->isHost) {
                    RakNet::BitStream bsOut;
                    bsOut.Write((RakNet::MessageID)ID_NET_OBJECT_MOVE);
                    bsOut.Write(networkObject.second.timestamp);
                    bsOut.Write(networkObject.second.networkID);
                    bsOut.Write(networkObject.second.positionECEF.x);
                    bsOut.Write(networkObject.second.positionECEF.y);
                    bsOut.Write(networkObject.second.positionECEF.z);
                    bsOut.WriteVector(
                        networkObject.second.velocityLocal.x,
                        networkObject.second.velocityLocal.y,
                        networkObject.second.velocityLocal.z);
                    bsOut.WriteNormQuat(
                        networkObject.second.quaternionLocal.w,
                        networkObject.second.quaternionLocal.x,
                        networkObject.second.quaternionLocal.y,
                        networkObject.second.quaternionLocal.z);

                    mImpl->peer->Send(&bsOut, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0, networkObject.second.sender, true);
                }
            }
        }

    }
    */

    //run callbacks for network status changes
    if (mImpl->isHost)
        mImpl->currentStatus = IS_HOSTING;
    else
        mImpl->currentStatus = static_cast<ConnectionState>(mImpl->peer->GetConnectionState(mImpl->peer->GetSystemAddressFromIndex(0)));

    mImpl->lastStatus = mImpl->currentStatus;


    mImpl->currentTime = RakNet::GetTime();

    //update client info as host more often
    RakNet::Time hostPingUpdateIntervalMS = 500; //every 0.5 second
    if (mImpl->isHost) {
        if (mImpl->currentTime - mImpl->hostPingTimeCtr > hostPingUpdateIntervalMS) {
            mImpl->clientInfoList.clear();
            {                
                //add host info first
                ClientInfo client;
                client.ID = mImpl->myGUID;
                client.ping = -1;
                client.name = mImpl->client_name;
                mImpl->clientInfoList.push_back(client);
                //window->setPing(mImpl->myGUID.ToString(), client.ping);
            }
            for (auto const &it : mImpl->clientNameMap) {
                //add each client
                ClientInfo client;
                client.ID = it.first;
                client.ping = peer->GetLastPing(client.ID);
                client.name = getClientNameByGUID(client.ID);
                mImpl->clientInfoList.push_back(client);
                window->setPing(client.ID.ToString(), client.ping);
            }
            //for (auto fncn : mImpl->clientUpdateCallbackFunctions) {
            //    fncn->run();
            //}

            mImpl->hostPingTimeCtr = mImpl->currentTime;
        }
    }


    //periodically update each client with all the other client's info
    //make sure each client gets a ping update about every 10 seconds
    if (mImpl->isHost)
    {
        size_t numberOfClientsReceiving = mImpl->clientMap.size() - 1;
        if (numberOfClientsReceiving > 0) {
            RakNet::Time pingUpdateIntervalMS = 10000 / numberOfClientsReceiving;
            if (mImpl->currentTime - mImpl->pingTimeCtr > pingUpdateIntervalMS) {
                unsigned short clientIndex = getNextClientIndex(mImpl->lastClientIndexUpdated);
                RakNet::RakNetGUID clientGUID = mImpl->hostClientIndexList[clientIndex];
                if (clientGUID != RakNet::UNASSIGNED_RAKNET_GUID) {
                    RakNet::BitStream bsOut;
                    bsOut.Write((RakNet::MessageID)ID_NET_CLIENT_INFO);
                    //Skip host info
                    unsigned short numberOfClients = (unsigned short)mImpl->clientInfoList.size() - 1;
                    bsOut.Write(numberOfClients);
                    for (auto& clientInfo : mImpl->clientInfoList) {
                        ClientInfo *client = &clientInfo;
                        if (client->ID != mImpl->myGUID) {
                            bsOut.Write(client->ID);
                            bsOut.WriteBitsFromIntegerRange(client->ping, -1, 2046);
                        }
                    }
                    mImpl->peer->Send(&bsOut, LOW_PRIORITY, UNRELIABLE, 0, clientGUID, false);
                    mImpl->lastClientIndexUpdated = clientIndex;
                }
                mImpl->pingTimeCtr = mImpl->currentTime;
            }
        }
    }

    int numClients = 0;
    float myPacketLoss = 0;
    uint64_t bandwidthSendRate = 0;
    uint64_t bandwidthReceiveRate = 0;
    uint64_t bandwidthSentTotal = 0;
    uint64_t bandwidthReceivedTotal = 0;
    uint64_t connectionTime = 0;

    if (mImpl->currentStatus == IS_CONNECTED || mImpl->isHost) {
        numClients = getNumClients();

        bandwidthSendRate = getRecentBandwidth(ACTUAL_BYTES_SENT);
        bandwidthReceiveRate = getRecentBandwidth(ACTUAL_BYTES_RECEIVED);
        bandwidthSentTotal = getBandwidth(ACTUAL_BYTES_SENT);
        bandwidthReceivedTotal = getBandwidth(ACTUAL_BYTES_RECEIVED);

        connectionTime = getConnectionTime();
        if (!mImpl->isHost) {
            myPacketLoss = getMyRecentPacketLoss();
        }

        window->setStatistics(numClients, bandwidthSendRate, bandwidthReceiveRate, bandwidthSentTotal, bandwidthReceivedTotal, connectionTime, myPacketLoss);
    }

    mImpl->statisticsUpdated = false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Network::handleLocalConnected()
{
    if (mImpl->mySeat > 0)
    {
        emit receivedSeatChange(mImpl->mySeat);
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Network::handleReceivedLocalEvent(unsigned char eventID)
{
    if (mImpl->mySeat > 0)
    {
        if (mImpl->currentStatus == IS_CONNECTED || mImpl->isHost)
        {
            RakNet::BitStream bsOut;
            bsOut.Write((RakNet::MessageID)ID_NET_EVENT);
            bsOut.Write(eventID);

            //if host, broadcast to everyone, else send to host only
            RakNet::SystemAddress skipAddress = mImpl->isHost ? RakNet::UNASSIGNED_SYSTEM_ADDRESS : mImpl->peer->GetSystemAddressFromIndex(0);
            mImpl->peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, ORDERING_CHANNEL_EVENTS, skipAddress, mImpl->isHost);
        }
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Network::handleReceivedLocalCommand(unsigned short command, unsigned char priority, unsigned char reliability, char orderingChannel)
{
    if (mImpl->mySeat > 0)
    {
        if (mImpl->currentStatus == IS_CONNECTED || mImpl->isHost)
        {
            unsigned char compressionType = BINARY;

            RakNet::BitStream bsOut;
            bsOut.Write((RakNet::MessageID)ID_NET_COMMAND);
            bsOut.Write(orderingChannel);
            //bsOut.WriteBits((const unsigned char *)&priorityChar, 2);
            //bsOut.WriteBits((const unsigned char *)&reliabilityChar, 3);
            //bsOut.WriteBits((const unsigned char *)&compressionTypeChar, 3);
            unsigned char packetInfo = priority | (unsigned char)(reliability << 2) | (unsigned char)(compressionType << 5);
            bsOut.Write(packetInfo);
            bsOut.Write(command);

            //if host, broadcast to everyone, else send to host only
            RakNet::SystemAddress skipAddress = mImpl->isHost ? RakNet::UNASSIGNED_SYSTEM_ADDRESS : mImpl->peer->GetSystemAddressFromIndex(0);
            mImpl->peer->Send(&bsOut, static_cast<PacketPriority>(priority), static_cast<PacketReliability>(reliability), orderingChannel, skipAddress, mImpl->isHost);
        }
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Network::handleReceivedLocalCommandValue(unsigned short command, unsigned char priority, unsigned char reliability, char orderingChannel, unsigned char compressionType, float value, bool deadReckoned, float valueRate)
{
    if (mImpl->mySeat > 0)
    {
        if (mImpl->currentStatus == IS_CONNECTED || mImpl->isHost)
        {
            RakNet::BitStream bsOut;
            bsOut.Write((RakNet::MessageID)ID_NET_COMMAND_VALUE);
            bsOut.Write(orderingChannel);
            //bsOut.WriteBits((const unsigned char *)&priorityChar, 2);
            //bsOut.WriteBits((const unsigned char *)&reliabilityChar, 3);
            //bsOut.WriteBits((const unsigned char *)&compressionTypeChar, 3);
            unsigned char packetInfo = priority | (unsigned char)(reliability << 2) | (unsigned char)(compressionType << 5);
            bsOut.Write(packetInfo);
            bsOut.Write(command);

            switch (compressionType)
            {
                case BINARY:
                    value > 0.0f ? bsOut.Write1() : bsOut.Write0();
                    break;
                case FLOAT16:
                    bsOut.WriteFloat16(value, -1.0f, 1.0f);
                    if (deadReckoned) {
                        bsOut.WriteFloat16(valueRate, -maxValueRate, maxValueRate);
                    }
                    break;
                case FLOAT32:
                    bsOut.Write(value);
                    if (deadReckoned) {
                        bsOut.Write(valueRate);
                    }
                    break;
                //case FLOAT64:
                //	bsOut.Write(dValue);
                //	break;
                case NEG_ONE_ZERO_ONE:
                {
                    unsigned char bitValue = 0;
                    if (value < 0.0f) {
                        bitValue = 0;
                    }
                    else if (value == 0.0f) {
                        bitValue = 1;
                    }
                    else {
                        bitValue = 2;
                    }
                    bsOut.WriteBits((const unsigned char *)&bitValue, 2);
                    break;
                }
                case ZERO_HALF_ONE:
                {
                    unsigned char bitValue = 0;
                    if (value == 0.0f) {
                        bitValue = 0;
                    }
                    else if (value > 0.99999f) {
                        bitValue = 2;
                    }
                    else {
                        bitValue = 1;
                    }
                    bsOut.WriteBits((const unsigned char *)&bitValue, 2);
                    break;
                }
                case NEG_ONE_ZERO_HALF_ONE:
                {
                    unsigned char bitValue = 0;
                    if (value < -0.99999f) {
                        bitValue = 0;
                    }
                    else if (value == 0.0f) {
                        bitValue = 1;
                    }
                    else if (value > 0.99999f) {
                        bitValue = 3;
                    }
                    else {
                        bitValue = 2;
                    }
                    bsOut.WriteBits((const unsigned char *)&bitValue, 3);
                    break;
                }
                case NEG_TWO_NEG_ONE_ZERO_ONE:
                {
                    unsigned char bitValue = 0;
                    if (value < -1.99999f) {
                        bitValue = 0;
                    }
                    else if (value < -0.99999f) {
                        bitValue = 1;
                    }
                    else if (value == 0.0f) {
                        bitValue = 2;
                    }
                    else if (value > 0.99999f) {
                        bitValue = 3;
                    }
                    bsOut.WriteBits((const unsigned char *)&bitValue, 3);
                    break;
                }
                default:
                    bsOut.Write(value);
                    break;
            }

            bsOut.PrintBits();

            //if host, broadcast to everyone, else send to host only
            RakNet::SystemAddress skipAddress = mImpl->isHost ? RakNet::UNASSIGNED_SYSTEM_ADDRESS : mImpl->peer->GetSystemAddressFromIndex(0);
            std::cout << "isHost = " << mImpl->isHost << ", skipAddress = " << skipAddress.ToString(true,':') << std::endl;;
            mImpl->peer->Send(&bsOut, static_cast<PacketPriority>(priority), static_cast<PacketReliability>(reliability), orderingChannel, skipAddress, mImpl->isHost);
        }
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Network::handleReceivedLocalCorrectionCommandValue(unsigned short command, float value)
{
    if (mImpl->mySeat > 0)
    {
        if (mImpl->currentStatus == IS_CONNECTED || mImpl->isHost) {

            RakNet::BitStream bsOut;
            bsOut.Write((RakNet::MessageID)ID_NET_CORRECTION_COMMAND_VALUE);

            bsOut.Write(command);
            bsOut.Write(value);

            //if host, broadcast to everyone, else send to host only
            RakNet::SystemAddress skipAddress = mImpl->isHost ? RakNet::UNASSIGNED_SYSTEM_ADDRESS : mImpl->peer->GetSystemAddressFromIndex(0);
            mImpl->peer->Send(&bsOut, LOW_PRIORITY, RELIABLE, 0, skipAddress, mImpl->isHost);
        }
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

ConnectionState Network::getNetworkStatus() const
{
    return mImpl->currentStatus;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool Network::isHost() const
{
    return mImpl->isHost;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool Network::isClient() const
{
    return (!mImpl->isHost && mImpl->currentStatus == IS_CONNECTED);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

template<typename T>
inline std::string toString(const T& val) {
    std::ostringstream o;
    o << val;
    return o.str();
}

std::string Network::getServerAddress() const
{
    if (mImpl->isHost) {
        return std::string("localhost:") + toString(mImpl->serverConfig.port);
    }
    else if (mImpl->currentStatus == IS_CONNECTED) {
        const char *serverAddress = mImpl->serverAddress.ToString(true, ':');
        return std::string(serverAddress);
    }
    else return std::string("N/A");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

RakNet::Time Network::getConnectionTime()
{
    if (mImpl->isHost)
        return mImpl->currentTime - mImpl->serverStartTime;
    else if (mImpl->currentStatus == IS_CONNECTED) {
        if (!mImpl->statisticsUpdated) {
            if (mImpl->peer->GetStatistics(0, mImpl->myStatistics))
                mImpl->statisticsUpdated = true;
        }
        return (mImpl->currentTime - (RakNet::Time)(mImpl->myStatistics->connectionStartTime / 1000));
    }
    else return 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int Network::getNumClients() const
{
    if (mImpl->currentStatus == IS_CONNECTED || mImpl->isHost) {
        return (int)mImpl->clientMap.size();
    }
    else return 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int Network::getMyPing() const
{
    if (mImpl->currentStatus == IS_CONNECTED) {
        return mImpl->myPing;
    }
    else return -1;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int Network::getServerTickTimeMS() const
{
    if (mImpl->isHost) {
        return mImpl->serverConfig.tick_time_ms;
    }
    else if (isClient()) {
        return (int)mImpl->peer->GetTickTime();
    }
    else return -1;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int Network::getMaxClients() const
{
    if (mImpl->isHost) {
        return mImpl->serverConfig.max_clients;
    }
    else if (isClient()) {
        return (int)mImpl->peer->GetMaximumIncomingConnections();
    }
    else return 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float Network::getMyPacketLoss()
{
    if (mImpl->currentStatus == IS_CONNECTED) {
        if (!mImpl->statisticsUpdated) {
            if (mImpl->peer->GetStatistics(0, mImpl->myStatistics))
                mImpl->statisticsUpdated = true;
            else
                return 0;
        }
        return mImpl->myStatistics->packetlossTotal;
    }
    else return 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float Network::getMyRecentPacketLoss()
{
    if (mImpl->currentStatus == IS_CONNECTED) {
        if (!mImpl->statisticsUpdated) {
            if (mImpl->peer->GetStatistics(0, mImpl->myStatistics))
                mImpl->statisticsUpdated = true;
            else
                return 0;
        }
        return mImpl->myStatistics->packetlossLastSecond;
    }
    else return 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

uint64_t Network::getBandwidth(ConnectionMetrics metric)
{
    if (mImpl->currentStatus == IS_CONNECTED) {
        if (!mImpl->statisticsUpdated) {
            if (mImpl->peer->GetStatistics(0, mImpl->myStatistics))
                mImpl->statisticsUpdated = true;
            else
                return 0;
        }
        return mImpl->myStatistics->runningTotal[metric];
    }
    else if (mImpl->isHost) {
        if (mImpl->peer->NumberOfConnections() > 0) {
            if (!mImpl->statisticsUpdated) {
                mImpl->myStatistics = mImpl->peer->GetStatistics(RakNet::UNASSIGNED_SYSTEM_ADDRESS, mImpl->myStatistics);
                mImpl->statisticsUpdated = true;
            }
            return mImpl->myStatistics->runningTotal[metric];
        }
        else
            return 0;
    }
    else return 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

uint64_t Network::getRecentBandwidth(ConnectionMetrics metric)
{
    if (mImpl->currentStatus == IS_CONNECTED) {
        if (!mImpl->statisticsUpdated) {
            if (mImpl->peer->GetStatistics(0, mImpl->myStatistics))
                mImpl->statisticsUpdated = true;
            else
                return 0;
        }
        return mImpl->myStatistics->valueOverLastSecond[metric];
    }
    else if (mImpl->isHost) {
        if (mImpl->peer->NumberOfConnections() > 0) {
            if (!mImpl->statisticsUpdated) {
                mImpl->myStatistics = mImpl->peer->GetStatistics(RakNet::UNASSIGNED_SYSTEM_ADDRESS, mImpl->myStatistics);
                mImpl->statisticsUpdated = true;
            }
            return mImpl->myStatistics->valueOverLastSecond[metric];
        }
        else
            return 0;
    }
    else return 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::string Network::getClientNameByGUID(RakNet::RakNetGUID guid) const
{
    if (guid != RakNet::UNASSIGNED_RAKNET_GUID && mImpl->clientNameMap.size() > 0) {
        const auto& it = mImpl->clientNameMap.find(guid);
        if (it != mImpl->clientNameMap.end()) {
            return it->second;
        }
    }

    return "Unknown Client";
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Client* Network::getClientByGUID(RakNet::RakNetGUID guid) const
{
    auto it = mImpl->clientMap.find(guid);
    if (it != mImpl->clientMap.end()) {
        return &it->second;
    }

    return nullptr;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

RakNet::RakNetGUID Network::getMyGUID() const
{
    return mImpl->myGUID;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const std::string &Network::getMyClientName() const
{
    return mImpl->client_name;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

unsigned long long Network::getMyMaxOutgoingSpeedPerConnection() const
{
    return mImpl->max_outgoing_speed_per_connection;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const ServerConfig &Network::getMyServerConfig() const
{
    return mImpl->serverConfig;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Network::setMyClientName(std::string clientName)
{
    if (clientName.length() > MAX_CLIENT_NAME_LENGTH)
        clientName = clientName.substr(0, MAX_CLIENT_NAME_LENGTH);

    if (clientName != mImpl->client_name) {
        mImpl->client_name = clientName;
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Network::setTimeoutTimeMS(int time_ms)
{
    time_ms = (time_ms > MAX_TIMEOUT_TIME_MS ? MAX_TICK_TIME_MS : time_ms);
    time_ms = (time_ms < MIN_TIMEOUT_TIME_MS ? MIN_TICK_TIME_MS : time_ms);
    mImpl->serverConfig.timeout_time_ms = time_ms;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Network::setMaxClients(int num_clients)
{
    num_clients = (num_clients > MAX_CLIENTS ? MAX_CLIENTS : num_clients);
    num_clients = (num_clients < MIN_CLIENTS ? MIN_CLIENTS : num_clients);
    mImpl->serverConfig.max_clients = num_clients;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void Network::setMyMaxOutgoingSpeedPerConnection(unsigned long long bitsPerSecond)
{
    bitsPerSecond = ((bitsPerSecond > MAX_SPEED_BITS) ? MAX_SPEED_BITS : bitsPerSecond);
    bitsPerSecond = ((bitsPerSecond < MIN_SPEED_BITS) ? MIN_SPEED_BITS : bitsPerSecond);
    if (bitsPerSecond != mImpl->max_outgoing_speed_per_connection) {
        mImpl->max_outgoing_speed_per_connection = bitsPerSecond;
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const std::vector<ClientInfo>& Network::getClientInfo() const
{
    return mImpl->clientInfoList;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

unsigned short Network::getNextClientIndex(unsigned short index)
{
    if (index >= 0 && index < MAX_CLIENTS) {
        //RakNet::SystemAddress remoteSystems[MAX_CLIENTS];
        //unsigned short numberOfSystems = MAX_CLIENTS;
        //mImpl->peer->GetConnectionList(remoteSystems, &numberOfSystems);
        int nextIndex = index + 1;
        while (nextIndex != index) {
            if (nextIndex >= MAX_CLIENTS)
                nextIndex = 0;

            if (mImpl->hostClientIndexList[nextIndex] != RakNet::UNASSIGNED_RAKNET_GUID)
            {
                return nextIndex;
            }

            nextIndex++;
        }
    }
    return 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

RakNet::RakNetGUID Network::getNextClientGUID(RakNet::RakNetGUID guid)
{
    int index = mImpl->peer->GetIndexFromSystemAddress(mImpl->peer->GetSystemAddressFromGuid(guid));
    if (index >= 0 && index < MAX_CLIENTS) {
        int nextIndex = index + 1;
        while (nextIndex != index) {
            if (nextIndex >= MAX_CLIENTS)
                nextIndex = 0;
            RakNet::RakNetGUID guid = mImpl->hostClientIndexList[nextIndex];

            if (guid != RakNet::UNASSIGNED_RAKNET_GUID)
            {
                return guid;
            }

            nextIndex++;
        }
    }
    return RakNet::UNASSIGNED_RAKNET_GUID;
}



//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

} // namespace Network
