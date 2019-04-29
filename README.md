# DCS-Copilot

Copyright (c) 2019, Cory Parks and contributors

## Description
DCS Copilot is a standalone windows client application that utilizes the [RakNet](https://github.com/facebookarchive/RakNet) 
networking protocol to facilitate synchronization of aircraft state and copilot actions in DCS multi-seat aircraft.

## How Does it Work?
DCS Copilot works by communicating with other "copilot" clients over the internet and also communicating with custom DCS aircraft 
through their code (like the External Flight Model) to inform each copilot aircraft of actions committed by other copilots 
or systems information necessary for keeping everyone's aircraft in the same synchronization state.

## How to User It?
This application is only a networking bridge between other clients over the internet and between the client application and the 
DCS aircraft. It is intended to be agnostic of specific aircraft and their necessary syncing logic. To take full advantage of this 
application, a user will need to write a RakNet connection class in their aircraft code to connect to the external application and 
the user will need to write all of the logic for integration of the syncing data into their aircraft code. 
