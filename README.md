# DCS-Copilot

Copyright (c) 2022, Cory Parks and contributors

[Imgur](https://i.imgur.com/BE83Dd9.png)

## Description
DCS Copilot is a standalone windows client application that utilizes the [RakNet](https://github.com/facebookarchive/RakNet) 
networking library to facilitate synchronization of aircraft state and pilot/copilot actions in DCS multi-seat aircraft.

## How Does it Work?
DCS Copilot works by communicating with other "copilot" clients over the internet and also communicating with custom DCS aircraft 
through their code (like the External Flight Model (EFM)) to inform each client's aircraft of actions committed by other copilots 
or systems information necessary for keeping everyone's aircraft in the same synchronization state.

## How to Use It?
This application is only a networking bridge between other clients over the internet and between the client application and the 
DCS aircraft. It is intended to be agnostic of specific aircraft and their necessary syncing logic. To take full advantage of this 
application, a user will need to write a RakNet connection class in their aircraft code to connect to the external application and 
the user will need to write all of the logic for integration of the syncing data into their aircraft code.

This project is in early development and is expected to change as the features mature.

See the Detailed Descriptions section for more information.

## Building the Application
Download [Qt](https://www.qt.io/download) and make sure to install QT <version> MSVC 2015 64-bit.  This program has been previously built 
successfully with Qt 5.12.3.  In the Qt Creator application, make sure the Qt <version> MSVC 2015 64bit kit is selected and the Build type is Release.
RakNet dependencies are already included in this package in the 3rdparty folder and is statically linked into the executable when built.

## Deploying the Application
Use the built-in Qt windows deployment tool windeployqt.exe on the deployment directory containing the built DCS_Copilot.exe and it will 
automatically pull all of the dependencies into the directory.

## License
See the LICENSE file.

## Detailed Descriptions

#### Seat Number
After joining the server, pilots are placed in seat number 0, which is a placeholder seat that does not exchange any aircraft
data with the host.  Use the Seat Request functionality on the main window to request a seat number (1 to (maxServerClients + 1)) 
from the host. The seat number will be automatically granted, if it is not in use by anyone else.

#### Client List
On the main window, the client list shows every connected client to the server, their current seat number, and their ping to the 
server.  This list can be sorted by the column headers in ascending or descending order by left clicking on the column headers. After 
a third click on the same column header, the sorting is removed.

#### Listener
The Listener is a local socket connection between the DCS Copilot application and your DCS World aircraft RakNet connection class.  The 
listener is the server host that the aircraft will be connecting to in whatever manner you choose.  The listener can be started or stopped
from the File menu dropdown.  There is a setting for the listener in the Edit->Settings->Listener tab to start the listener when the 
DCS Copilot application starts.  The port setting for the listener is currently not configurable.

#### Server Host
A server host can start a new server via the File menu dropdown (File->Start Server).  A new Start Server window will open that will prompt the host for
their client name once the server is connected, as well as the server port and password (optional).  Leave the password box blank if no password
is desired for clients to connect.  The port can be changed by clicking the Lock icon next to the Port box and changing the value.  The Default
Server Port can be changed in the Edit->Settings->Server tab so that this step is not required each time.  The client name can also be changed
in the Edit->Settings->Client tab.  The server can be stopped via the File menu dropdown (File->Disconnect), after which all clients will be 
informed that the server is shutting down and will be disconnected.

The server can be further configured in the Edit->Settings->Server tab:
 * Timeout Time in milliseconds (ms) - Amount of time before a client is force disconnected when it fails to communicate with the server.
 * Maximum Clients - Max number of clients that can connect to the server.  This does not include the server host.  Sets the number of seats available
   to maxServerClients + 1 so there are enough seats for every client plus the host.
 * Tick Rate in Hz - Server tick rates available are 167, 83, 56, 42, and 33 Hz, which correspond to every 6 ms interval, the update frame time of
   DCS World EFM aircraft.

#### Client Connection
A client can connect to a server via the File menu dropdown (File->Connect).  A new Connection window will open that will prompt the client for their
Client Name, as well as the server information (IP Address, Port and Password).  Tthe port can be modified by clicking the Lock icon next to the 
Port box and changing the value.

The Connection window also has a History list of recent servers where a connection was attempted and a server Favorites list in the box to the 
right. The History list is sorted from most recent to oldest.  You can fill in the server connection details with a recent server by double clicking 
on the server IP:Port info of that server in the History list.

By right clicking in the History list, you can:
 * Remove a server from the History list, if right clicked on a valid server row
 * Clear the entire server History list

The Connection window Favorites list displays all servers that have been added to the Favorites by clicking the Star icon to the left of the server 
info in the History list.  Once a server has been added to the favorites, it can be given a Favorite Name by single left clicking in the Favorite Name 
box of that server (History or Favorites list tab).  A server can be removed from the Favorites list by again clicking the star icon of that server in 
either the History list or the Favorites list tab.  You can also fill in the server connection details with a Favorite server by double clicking on the 
server IP:Port info of that server in the Favorites list.

The client connection can be further configured in the Edit->Settings->Client tab:
 * Default Client Name - Applies to hosts and clients.
 * Max Server History - Max size of the server History list on the Connection window.

#### Server Kick/Ban
The server host has the following additional functionality when right clicking on clients in the client list:
 * Kick clients out of their seat (back to seat 0)
 * Kick clients out of the server
 * Ban clients from the server

All bans are permanent until removed from master Ban List by the server host (Edit->Ban List).
The server host can edit their server's master Ban List by:
 * Removing individual IP addresses from the ban list
 * Clearing the entire Ban List
 
#### Connection Statistics
Statistics of the current connection (Client or Host) are displayed on the left of the main window:
 * Server IP - IP and port of the server you are currently connected to (Will display "localhost" when hosting)
 * Num Clients - Number of currently connected clients to the server
 * Ping - Your current ping to the server (Will display "N/A" when hosting)
 * Packet Loss - Your current percentage of packet loss with the server
 * Bandwidth Out/In - Your current bandwidth usage per second, outbound and inbound
 * Total Bandwidth Out/In - Your overall total bandwidth usage for the entire connection session, outbound and inbound
 * Connection Time - Your total time connected to the server for this connection session
 
#### Digital/Analog Command Buttons
This is a debug feature to test that data can be communicated between connected clients and the server host without requiring DCS World to be  
running with a properly connected aircraft RakNet connection.

#### Listener Status 
The status of the Listener can be seen in the bottom information bar as either "RUNNING" or "NOT RUNNING".

#### DCS Status
The status of the DCS connection can be seen in the bottom information bar as either "RUNNING" or "NOT RUNNING", where "RUNNING" indicates 
a successful connection between DCS Copilot and the aircraft's RakNet connection class.

#### Server Status
The Server status is visible in the bottom information bar as one of the follwing:
 * NOT CONNECTED - You are neither connected to a server, nor hosting a server
 * CONNECTING - You are attempting to connect to a server as a client
 * CONNECTED - You are connected to a server as a client
 * HOSTING - You are hosting a server
 
#### How It All Really Works (for real!)
Note: A full example for this type of implementation is a TODO item for this project.
This is still an area of research and investigation, so explanations may not be perfectly correct.

##### The Problem (Lack of Built-in Syncing)
DCS World built-in multicrew functionality is pretty crude, with only aircraft position, orientation, and speeds being perfectly 
synced between DCS clients in the same aircraft, originating from the pilot in command (PIC).  However, the EFM dll of every other 
client is still running and simulating as if they were the PIC, with the force and moment calculations being ignored by the sim.
Any systems logic in the EFM, as well as in the aircraft's lua systems is still being performed and can be valuable to make sure 
the visual state of the aircraft and displays is correct for all connected clients.  However, without the reliable syncing of pilot 
actions (DCS commands and events), the state of the aircraft for each connected DCS client will quickly become unsynced from the PIC.

##### The Solution (Command Syncing)
To resolve this sync issue, the goal of this project is to provide a method to sync these commands, events, and if desired, 
cockpit and external animations.  Pilot actions originate from the aircraft of the pilot that performed the action, pass through
the DCS Copilot application running on the same computer and then are either sent to all connected clients or to the DCS Copilot 
server host, depending on whether the pilot that performed the action is the DCS Copilot server host or not.  Once the action received
by another client, their aircraft will automatically perform the same action, as if they themselves peromed it, just with a small latency.
It is therefore recommended to host the DCS Copilot server if you are the PIC of the aircraft in DCS World, to eliminate latency between 
an action by a DCS client and the PIC.  If you are not the DCS Copilot host, your actions are first sent to the host and then are
rebroadcast to all other clients in a valid seat.

Axes commands, such as pitch, roll, yaw and throttles can be continuously changing, so you should make sure that only the PIC in DCS has 
direct control of these commands.  Investigation is needed to determine whether the PIC can be transferred to another client in a manner
that doesn't introduce unacceptable input lag, since the PIC in DCS would also need to change.

##### Cockpit/External Animation Syncing
It may be desired in some cases to sync cockpit or external animations directly, instead of relying on commands alone.  Some 
potential reasons for this:
 * Animation is controlled by another pilot, but is purely visual. (Ex: Head movement of other pilots in the cockpit)
 * Other DCS clients in the aircraft are using a "dead" EFM dll that only performs the sync of animations and does not include any 
   systems code to process commands and events to recreate the visual state of the aircraft.  This could be used to give rides to 
   others without sharing any sensitive binaries or lua code.

If you choose to sync animations, you should only have one client seat number in charge of any one animation or else every client's
individual animation state will conflict with the each others.

##### Communication (DCS <-> DCS Copilot)
Once a connection has been established between the DCS Copilot application and the DCS World aircraft's RakNet connection class, 
your aircraft should begin transmitting digital and analog commands performed by you, which includes clickable command actions, 
as well as relevant key binds that need to be synced.  Additionally, DCS Events performed by you should be sent in this manner 
as well. On the inbound side, actions performed by other copilots will be sent to your DCS aircraft from the DCS Copilot 
application and your aircraft code will need to perform the action seamlessly. It is recommended to implement a permission system 
at the aircraft level to intercept commands before they are performed and check whether a client in that seat (including yourself) 
should be allowed to perform that action. 

##### Communication (DCS Copilot <-> DCS Copilot)
Once a connection has been established between your DCS Copilot application and the host (or other clients) application, any actions 
sent from your DCS aircraft will be passed through the application to the DCS Copilot server host or to all other clients, if you are 
the host.  The DCS Copilot application is agnostic of the aircraft and its systems logic, so it is a straight pass-through with no 
filtering for validity.

##### Advanced Syncing Setup/Options
TODO

##### Setup Your DCS Aircraft Code
TODO