#ifndef EXTENDEDFILESINK_H
#define EXTENDEDFILESINK_H

#include "MediaSink.hh"
#include "GroupsockHelper.hh"
#include "OutputFile.hh"

#include "mongo/client/dbclient.h"
#include "mongo/client/dbclientcursor.h"

/*!
 * \brief The ExtendedFileSink class saves JPEGs to a MongoDB
 */
class ExtendedFileSink: public MediaSink {
public:
    static ExtendedFileSink* createNew(UsageEnvironment& env, std::string tmpCamNameSuffix, unsigned bufferSize = 20000);
    void addData(unsigned char const* data, unsigned dataSize, struct timeval presentationTime);

protected:
    ExtendedFileSink(UsageEnvironment& env, unsigned bufferSize, std::string tmpCamNameSuffix);
      // called only by createNew()
    virtual ~ExtendedFileSink();

protected: // redefined virtual functions:
    virtual Boolean continuePlaying();


protected:
    static void afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds);
    virtual void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime);

    /// Buffer for receiving a JPEG
    unsigned char* fBuffer;
    /// Size of fBuffer
    unsigned fBufferSize;
    /// Name under which the file will be saved
    std::string stringfNameSuffix;
};

#endif // EXTENDEDFILESINK_H
