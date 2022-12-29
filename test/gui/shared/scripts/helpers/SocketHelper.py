import os
import sys

# the script needs to use the system wide python
# to switch from the built-in interpreter see https://kb.froglogic.com/squish/howto/using-external-python-interpreter-squish-6-6/
# if the IDE fails to reference the script, add the folder in Edit->Preferences->PyDev->Interpreters->Libraries
sys.path.append(os.path.realpath('../../../shell_integration/nautilus/'))
from syncstate import SocketConnect

from helpers.SyncHelper import (
    filterSyncMessages,
    filterMessagesForItem,
)


# socket messages
socket_messages = []
socketConnect = None


def getSocketConnection():
    global socketConnect
    if not socketConnect or not socketConnect.connected:
        socketConnect = SocketConnect()
    return socketConnect


def readSocketMessages():
    socket_messages = []
    socketConnect = getSocketConnection()
    socketConnect.read_socket_data_with_timeout(0.1)
    for line in socketConnect.get_available_responses():
        socket_messages.append(line)
    return socket_messages


def readAndUpdateSocketMessages():
    messages = readSocketMessages()
    return updateSocketMessages(messages)


def updateSocketMessages(messages):
    global socket_messages
    socket_messages.extend(filterSyncMessages(messages))
    return socket_messages


def clearSocketMessages(resource=''):
    global socket_messages
    if resource:
        resource_messages = set(filterMessagesForItem(socket_messages, resource))
        socket_messages = [
            msg for msg in socket_messages if msg not in resource_messages
        ]
    else:
        socket_messages.clear()


def closeSocketConnection():
    global socketConnect, socket_messages
    socket_messages.clear()
    if socketConnect:
        socketConnect.connected = False
        socketConnect._sock.close()
