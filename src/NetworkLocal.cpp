/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module:       NetworkLocal.cpp
Author:       Cory Parks
Date started: 10/2016
Purpose:      NetworkLocal Class

See LICENSE file for copyright and license information

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
NetworkLocal class handles communication between DCS Copilot and the DCS application
running on the same computer.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

//#include "stdafx.h"

#include "NetworkLocal.h"
#include <iostream>
#include <vector>

#include "RakNetTypes.h"
#include "RakNetStatistics.h"
#include "BitStream.h"
#include "GetTime.h"

#include "mainwindow.h"

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

enum CustomLocalNetworkMessages
{
    ID_LOCAL_MESSAGE_1 = ID_USER_PACKET_ENUM + 1,
    ID_LOCAL_SET_SEAT,
    ID_LOCAL_COMMAND,
    ID_LOCAL_COMMAND_VALUE,
    ID_LOCAL_UNRELIABLE_COMMAND_VALUE,
};

namespace Network {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

NetworkLocal::NetworkLocal(MainWindow* window_)
{
    window = window_;
    peer = RakNet::RakPeerInterface::GetInstance();
    //peer->SetOccasionalPing(true);
    myGUID = peer->GetMyGUID();
    currentTime = RakNet::GetTime();

    writeOutput(QString("Local IP: %1").arg(peer->GetLocalIP(0)));
    //void * data = this;// &pointers;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

NetworkLocal::~NetworkLocal()
{
    RakNet::RakPeerInterface::DestroyInstance(peer);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void NetworkLocal::writeOutput(const QString& q) const
{
    std::cout << q.toStdString().c_str() << std::endl;
    window->logMessage(q);
}

void NetworkLocal::updateListenerStatus(bool running) const
{
    window->updateListenerStatus(running);
}

void NetworkLocal::updateDCSStatus(bool running) const
{
    window->updateDCSStatus(running);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool NetworkLocal::startServer()
{
    //cancel any ongoing connections
    //peer->Shutdown(100);

    if (!isHost)
    {
        unsigned short port = 37820;
        unsigned short max_clients = 1;

        RakNet::SocketDescriptor sd(port, 0);
        RakNet::StartupResult result = peer->Startup(max_clients, 10, &sd, 1);

        if (result == RakNet::RAKNET_STARTED)
        {
            writeOutput(QString("Listener started successfully on port: %1").arg(port));
            //QLabel* label =  window->findChild<QLabel*>("Listener_status_label");
            updateListenerStatus(true);

            isHost = true;
            peer->SetMaximumIncomingConnections(max_clients);

            peer->SetTimeoutTime(1000, RakNet::UNASSIGNED_SYSTEM_ADDRESS);
            return true;
        }
        else {
            switch (result)
            {
            case RakNet::RAKNET_ALREADY_STARTED:
                writeOutput("<font color='red'>ERROR:</font> Listener already started.");
                break;
            case RakNet::INVALID_SOCKET_DESCRIPTORS:
                writeOutput("<font color='red'>ERROR:</font> Listener socket descriptors invalid.");
                break;
            case RakNet::INVALID_MAX_CONNECTIONS:
                writeOutput(QString("<font color='red'>ERROR:</font> Listener max connections invalid: %1").arg(max_clients));
                break;
            case RakNet::SOCKET_FAMILY_NOT_SUPPORTED:
                writeOutput("<font color='red'>ERROR:</font> Listener socket family not supported.");
                break;
            case RakNet::SOCKET_PORT_ALREADY_IN_USE:
                writeOutput(QString("<font color='red'>ERROR:</font> Listener socket port already in use: %1").arg(port));
                break;
            case RakNet::SOCKET_FAILED_TO_BIND:
                writeOutput("<font color='red'>ERROR:</font> Listener socket failed to bind.");
                break;
            case RakNet::SOCKET_FAILED_TEST_SEND:
                writeOutput("<font color='red'>ERROR:</font> Listener socket failed test send.");
                break;
            case RakNet::PORT_CANNOT_BE_ZERO:
                writeOutput("<font color='red'>ERROR:</font> Listener port cannot be zero.");
                break;
            case RakNet::FAILED_TO_CREATE_NETWORK_THREAD:
                writeOutput("<font color='red'>ERROR:</font> Listener failed to create network thread.");
                break;
            case RakNet::COULD_NOT_GENERATE_GUID:
                writeOutput("<font color='red'>ERROR:</font> Listener could not generate GUID.");
                break;
            case RakNet::STARTUP_OTHER_FAILURE:
                writeOutput("<font color='red'>ERROR:</font> Listener - Other failure.");
                break;
            default:
                writeOutput("<font color='red'>ERROR:</font> Listener - Unknown startup error.");
                break;
            }
            updateListenerStatus(false);
        }
    }

    return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void NetworkLocal::disconnect()
{
    if (currentStatus <= IS_CONNECTED || isHost) {
        peer->Shutdown(300);
        isHost = false;
        updateListenerStatus(false);
        updateDCSStatus(false);
        writeOutput("Shutting down the Listener ...");
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void NetworkLocal::update()
{
    //run callbacks for network status changes
    if (isHost)
        currentStatus = IS_HOSTING;
    else
        currentStatus = static_cast<ConnectionState>(peer->GetConnectionState(peer->GetSystemAddressFromIndex(0)));

    if (currentStatus != lastStatus) {
    }

    lastStatus = currentStatus;

    currentTime = RakNet::GetTime();

    //packet checking loop
    for (packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
    {
        isAttemptingConnection = false;
        switch (packet->data[0])
        {
        case ID_UNCONNECTED_PONG:
        {
            //unsigned int dataLength;
            RakNet::TimeMS time;
            RakNet::BitStream bsIn(packet->data, packet->length, false);
            bsIn.IgnoreBytes(1);
            bsIn.Read(time);
            writeOutput(QString("Ping: %1 ms").arg((unsigned int)(RakNet::GetTimeMS() - time)));

            break;
        }
        case ID_CONNECTED_PING:
            writeOutput(QString("Ping from %1").arg(packet->systemAddress.ToString(true)));
            break;
        case ID_UNCONNECTED_PING:
            writeOutput(QString("Ping from %1").arg(packet->systemAddress.ToString(true)));
            break;
        case ID_NEW_INCOMING_CONNECTION:
        {
            //received by the host only
            if (isHost) {
                int index = peer->GetIndexFromSystemAddress(packet->systemAddress);
                RakNet::RakNetGUID clientGUID = peer->GetGuidFromSystemAddress(packet->systemAddress);

                if (clientGUID != RakNet::UNASSIGNED_RAKNET_GUID) {
                    writeOutput(QString("<font color='green'>DCS Communication has started - GUID: %1</font>").arg(clientGUID.ToString()));
                }
                updateDCSStatus(true);

                emit localConnected();
            }
            break;
        }
        case ID_DISCONNECTION_NOTIFICATION:
            if (isHost) {
                int index = peer->GetIndexFromSystemAddress(packet->systemAddress);
                writeOutput(QString("DCS Communication has ended - Disconnected"));

                updateDCSStatus(false);
            }
            break;
        case ID_CONNECTION_LOST:
        {
            if (isHost) {
                int index = peer->GetIndexFromSystemAddress(packet->systemAddress);
                writeOutput(QString("<font color='red'>ERROR: DCS Communication has ended - Connection Lost</font>"));

                updateDCSStatus(false);
            }
            break;
        }

        ////////////////////////////////////////////////
        ///////////    CUSTOM MESSAGE IDS    ///////////
        ////////////////////////////////////////////////

        case ID_LOCAL_MESSAGE_1:
        {
            //writeOutput("%s", readBitStreamCharArray(packet));
            break;
        }
        case ID_LOCAL_COMMAND:
        {
            int command = -1;
            RakNet::BitStream bsIn(packet->data, packet->length, false);
            bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
            bsIn.ReadBitsFromIntegerRange(command, 0, 65535);
            emit receivedLocalCommand(command);
            writeOutput(QString("Local Command (%1)").arg(command));
            break;
        }
        case ID_LOCAL_COMMAND_VALUE:
        case ID_LOCAL_UNRELIABLE_COMMAND_VALUE:
        {
            int command = -1;
            float value = 0.f;
            RakNet::BitStream bsIn(packet->data, packet->length, false);
            bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
            bsIn.ReadBitsFromIntegerRange(command,0,65535);
            bsIn.Read(value);
            bool reliable = (packet->data[0] == ID_LOCAL_COMMAND_VALUE);
            emit receivedLocalCommandValue(command, value, reliable);
            writeOutput(QString("Local Command (%1):").arg(command)+=QString::number(value));
            break;
        }
        default:
            writeOutput(QString("Message with identifier %1 has arrived").arg(packet->data[0]));
            break;
        }
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

ConnectionState NetworkLocal::getNetworkStatus() const
{
    return currentStatus;
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

RakNet::RakString readBitStreamString(RakNet::Packet *packet)
{
    RakNet::RakString rs;
    RakNet::BitStream bsIn(packet->data, packet->length, false);
    bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
    bsIn.Read(rs);
    return rs;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const char* readBitStreamCharArray(RakNet::Packet *packet)
{
    return readBitStreamString(packet).C_String();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void NetworkLocal::handleReceivedSeatChange(int seatNumber)
{
    if (isHost && seatNumber > 0) {
        RakNet::BitStream bsOut;
        bsOut.Write((RakNet::MessageID)ID_LOCAL_SET_SEAT);
        int zeroBasedSeatNumber = seatNumber - 1;
        int maxValue = 64 - 1;
        bsOut.WriteBitsFromIntegerRange(zeroBasedSeatNumber, 0, maxValue);
        //send to dcs
        peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void NetworkLocal::handleReceivedNetCommand(int command)
{
    if (isHost) {
        RakNet::BitStream bsOut;
        bsOut.Write((RakNet::MessageID)ID_LOCAL_COMMAND);
        bsOut.WriteBitsFromIntegerRange(command, 0, 65535);
        //send to dcs
        peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void NetworkLocal::handleReceivedNetCommandValue(int command, float value, bool reliable)
{
    if (isHost) {
        RakNet::BitStream bsOut;
        if (reliable) {
            bsOut.Write((RakNet::MessageID)ID_LOCAL_COMMAND_VALUE);
        }
        else {
            bsOut.Write((RakNet::MessageID)ID_LOCAL_UNRELIABLE_COMMAND_VALUE);
        }
        bsOut.WriteBitsFromIntegerRange(command, 0, 65535);
        bsOut.Write(value);
        //send to dcs
        if (reliable) {
            peer->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
        }
        else {
            peer->Send(&bsOut, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
        }
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

} // namespace Network
