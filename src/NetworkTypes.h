/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Header:       NetworkTypes.h
Author:       Cory Parks
Date started: 10/2016

See LICENSE file for copyright and license information

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef NETWORKTYPES_H
#define NETWORKTYPES_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/



/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace Network {

	enum ConnectionState
	{
		/// Connect() was called, but the process hasn't started yet
		IS_PENDING,
		/// Processing the connection attempt
		IS_CONNECTING,
		/// Is connected and able to communicate
		IS_CONNECTED,
		/// Was connected, but will disconnect as soon as the remaining messages are delivered
		IS_DISCONNECTING,
		/// A connection attempt failed and will be aborted
		IS_SILENTLY_DISCONNECTING,
		/// No longer connected
		IS_DISCONNECTED,
		/// Was never connected, or else was disconnected long enough ago that the entry has been discarded
		IS_NOT_CONNECTED,
		/// Client is hosting
		IS_HOSTING
	};

    enum ServerStatus
    {
        SS_IS_CONNECTING = -1,
        SS_NOT_CONNECTED = 0,
        SS_CONNECTED,
        SS_HOSTING
    };

	enum ConnectionMetrics
	{
		/// How many bytes per pushed via a call to RakPeerInterface::Send()
		USER_MESSAGE_BYTES_PUSHED,

		/// How many user message bytes were sent via a call to RakPeerInterface::Send(). This is less than or equal to USER_MESSAGE_BYTES_PUSHED.
		/// A message would be pushed, but not yet sent, due to congestion control
		USER_MESSAGE_BYTES_SENT,

		/// How many user message bytes were resent. A message is resent if it is marked as reliable, and either the message didn't arrive or the message ack didn't arrive.
		USER_MESSAGE_BYTES_RESENT,

		/// How many user message bytes were received, and returned to the user successfully.
		USER_MESSAGE_BYTES_RECEIVED_PROCESSED,

		/// How many user message bytes were received, but ignored due to data format errors. This will usually be 0.
		USER_MESSAGE_BYTES_RECEIVED_IGNORED,

		/// How many actual bytes were sent, including per-message and per-datagram overhead, and reliable message acks
		ACTUAL_BYTES_SENT,

		/// How many actual bytes were received, including overead and acks.
		ACTUAL_BYTES_RECEIVED,
    };
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif // NETWORKTYPES_H
