/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Header:       NetworkLocal.h
Author:       Cory Parks
Date started: 10/2016

See LICENSE file for copyright and license information

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef NETWORKLOCAL_H
#define NETWORKLOCAL_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "NetworkTypes.h"

#include "RakPeerInterface.h"
#include "RakString.h"
#include "MessageIdentifiers.h"

#include <QObject>
#include <QString>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


class MainWindow;


namespace Network {


	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	CLASS DOCUMENTATION
	%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

	/** Local Network class wrapper using RakNet library. The local network handles
	communication with DCS running on the same computer.

	@author Cory Parks
	*/

	/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	CLASS DECLARATION
	%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

    class NetworkLocal : public QObject
	{
        Q_OBJECT

    signals:
        void localConnected();
        void receivedLocalCommand(int command);
        void receivedLocalCommandValue(int command, float value, bool reliable);

    public slots:
        void handleReceivedSeatChange(int seatNumber);
        void handleReceivedNetCommand(int command);
        void handleReceivedNetCommandValue(int command, float value, bool reliable);

	public:
        /// Constructor
        NetworkLocal(MainWindow* window_);
        /// Destructor
        virtual ~NetworkLocal();

		///Main update function to call each frame to capture all received network packets since the last call
        virtual void update();

		///Attempt to start the network server as host
		///Returns true if successful.
		bool startServer();

		///If connected to a server, attempt to disconnect.
		void disconnect();

		///Returns the network status of the peer
		ConnectionState getNetworkStatus() const;

    protected:
        RakNet::RakPeerInterface *peer = nullptr;
        RakNet::Packet *packet = nullptr;

        RakNet::SystemAddress currentConnectionAttemptAddress;
        RakNet::SystemAddress serverAddress;

        bool isAttemptingConnection = false;
        bool isHost = false;
        RakNet::RakNetGUID myGUID = RakNet::UNASSIGNED_RAKNET_GUID;

        RakNet::Time currentTime;

        ConnectionState lastStatus = IS_NOT_CONNECTED;
        ConnectionState currentStatus = IS_NOT_CONNECTED;

        MainWindow* window = nullptr;

        void writeOutput(const QString& q) const;
        void updateDCSStatus(bool running) const;

    private:

		// Make this object be noncopyable because it holds a pointer
		NetworkLocal(const NetworkLocal&);
		const NetworkLocal &operator =(const NetworkLocal &);

        void updateListenerStatus(bool running) const;
	};

    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    RakNet::RakString readBitStreamString(RakNet::Packet *packet);

    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    const char* readBitStreamCharArray(RakNet::Packet *packet);

} // namespace Network

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif // NETWORKLOCAL_H
