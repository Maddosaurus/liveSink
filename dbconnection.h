#ifndef DBCONNECTION_H
#define DBCONNECTION_H

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

class DBConnection
{
public:
    ~DBConnection();
    DBConnection* getInstance();

private:
    DBConnection();

private:
   static DBConnection* instance;
};

#endif // DBCONNECTION_H
