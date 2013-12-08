#ifndef EXTENDEDFILESINK_H
#define EXTENDEDFILESINK_H

#include "MediaSink.hh"
#include "GroupsockHelper.hh"
#include "OutputFile.hh"

#include "mongo/client/dbclient.h"
#include "mongo/client/dbclientcursor.h"

class ExtendedFileSink: public MediaSink {
public:
    static ExtendedFileSink* createNew(UsageEnvironment& env, std::string tmpCamNameSuffix, unsigned bufferSize = 20000);
    void addData(unsigned char const* data, unsigned dataSize, struct timeval presentationTime);

protected:
    ExtendedFileSink(UsageEnvironment& env, FILE* fid, unsigned bufferSize, std::string tmpCamNameSuffix);
      // called only by createNew()
    virtual ~ExtendedFileSink();

protected: // redefined virtual functions:
    virtual Boolean continuePlaying();


protected:
    static void afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds);
    virtual void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime);

    FILE* fOutFid;
    unsigned char* fBuffer;
    unsigned fBufferSize;
    std::string stringfNameSuffix;
};

#endif // EXTENDEDFILESINK_H
