class SetupClientHelper():
    def __int__(self):
        pass
    
    def substituteInLineCodes(self, context, value):
        from urllib.parse import urlparse
        value = value.replace('%local_server%', context.userData['localBackendUrl'])
        value = value.replace('%client_sync_path%', context.userData['clientSyncPath'])
        value = value.replace('%local_server_hostname%', urlparse(context.userData['localBackendUrl']).netloc)

        return value
