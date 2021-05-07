from urllib.parse import urlparse

def substituteInLineCodes(context, value):
    value = value.replace('%local_server%', context.userData['localBackendUrl'])
    value = value.replace('%client_sync_path%', context.userData['clientSyncPath'])
    value = value.replace('%local_server_hostname%', urlparse(context.userData['localBackendUrl']).netloc)

    return value
