//Momentan Singleton und 1 Connection
//NICHT THREAD SAFE!! <- TODO
//Umsteigen auf ScopedDbConnection -> Pooling / Multiple Connections

#include "dbconnection.h"

#include "mongo/client/dbclient.h"
#include "mongo/client/dbclientcursor.h"

DBConnection* DBConnection::instance = NULL;

DBConnection::DBConnection()
{
    mongo::DBClientConnection c;
    c.connect("localhost");
}

DBConnection::~DBConnection()
{

}

DBConnection* DBConnection::getInstance()
{
    if(instance == NULL)
    {
        instance = new DBConnection();
    }

    return instance;
}
