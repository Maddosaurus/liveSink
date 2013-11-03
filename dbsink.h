#ifndef DBSINK_H
#define DBSINK_H

#ifndef _MEDIA_SINK_HH
#include "MediaSink.hh"
#endif

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"


class DBSink: public MediaSink
{
public:
    DBSink* createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId);
    static void afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds);
    void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds);
protected:
    DBSink(UsageEnvironment &env, MediaSubsession &subsession, char const* streamId);
    virtual ~DBSink();
private:
    virtual Boolean continuePlaying();

private:
    u_int8_t* fReceiveBuffer;
    MediaSubsession& fSubsession;
    char* fStreamId;
};

#endif // DBSINK_H


/*
class DummySink: public MediaSink {
public:
  static DummySink* createNew(UsageEnvironment& env,
                  MediaSubsession& subsession, // identifies the kind of data that's being received
                  char const* streamId = NULL); // identifies the stream itself (optional)

private:
  DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId);
    // called only by "createNew()"
  virtual ~DummySink();

  static void afterGettingFrame(void* clientData, unsigned frameSize,
                                unsigned numTruncatedBytes,
                struct timeval presentationTime,
                                unsigned durationInMicroseconds);
  void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
             struct timeval presentationTime, unsigned durationInMicroseconds);

private:
  // redefined virtual functions:
  virtual Boolean continuePlaying();

private:
  u_int8_t* fReceiveBuffer;
  MediaSubsession& fSubsession;
  char* fStreamId;
};

*/
