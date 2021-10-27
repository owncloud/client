#include "resourceloader.h"

#include <QtGlobal>

// Q_INIT_RESOURCE doesn't like namespaces
void init()
{
    Q_INIT_RESOURCE(client);
    Q_INIT_RESOURCE(owncloudResources_translations);
    Q_INIT_RESOURCE(theme);
}

void OCC::Resources::init()
{
    ::init();
}
