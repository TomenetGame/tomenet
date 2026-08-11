#!/bin/bash

# Helper script for tomenet.server to send a player to a remote server
# as 'guest', temporarily (SERVER_PORTALS feature) -  C. Blue

# Arguments format:
# (1) Remote server, (2) account name (modified, temp), (3) account password (new, temp), (4) character name (modified, temp), (5) savegame filename

# This source server's (arbitrary) name (consistent with entries in lib/config/server_portals.cfg)
SOURCESERVER=xxx

# -----------------------------------------------------------------------------

# Cut away the path to lib/save/ and just keep the actual savegame's filename
LOCALFILE=$(basename ${5})

SAVEFILE=`echo "SAVE_"$SOURCESERVER"_"$LOCALFILE`
INFOFILE=`echo "INFO_"$SOURCESERVER"_"$LOCALFILE`

echo "`date +%H:%M:%S` : <$1> <$2> <$3> <$4> <$SAVEFILE>" >> server_portals.log
echo "         : '$4' ('$2') -> $1" >> server_portals.log

echo $2 > server_portals.tmp
echo $3 >> server_portals.tmp
echo $4 >> server_portals.tmp

# Configure all known remote servers (ie their own, local, arbitrary SOURCESERVER names)
# with their specific game paths and IP addresses (or hostnames)
case "$1" in
yyy)
    REMOTEHOST=some.host.name
    REMOTEPATH=/home/tomenet
    REMOTESAVEPATH=lib/save/server_portals
    ;;
case "$1" in
zzz)
    REMOTEHOST=or.some.ip.address
    REMOTEPATH=/home/tomenet
    REMOTESAVEPATH=lib/save/server_portals
    ;;
esac

REMOTESAVEPATH=$REMOTEPATH/lib/save/server_portals

# Send the savegame file
scp -q $5 tomenet@$REMOTEHOST:$REMOTEPATH/$REMOTESAVEPATH/$SAVEFILE
# Send info about temporary account name, temporary password, temporary character name
scp -q server_portals.tmp tomenet@$REMOTEHOST:$REMOTEPATH/$REMOTESAVEPATH/$INFOFILE

rm server_portals.tmp
