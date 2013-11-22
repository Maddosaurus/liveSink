#include "extendedfilesink.h"
#include <sstream>

ExtendedFileSink::ExtendedFileSink(UsageEnvironment& env, FILE* fid, unsigned bufferSize, char const* perFrameFileNamePrefix, char const* fNameSuffix) : MediaSink(env), fOutFid(fid), fBufferSize(bufferSize) {
    fBuffer = new unsigned char[bufferSize];
    if (perFrameFileNamePrefix != NULL) {
        fPerFrameFileNamePrefix = strDup(perFrameFileNamePrefix);
        fPerFrameFileNameBuffer = new char[strlen(perFrameFileNamePrefix) + 100];
    } else {
        fPerFrameFileNamePrefix = NULL;
        fPerFrameFileNameBuffer = NULL;
    }

    ExtendedFileSink::fNameSuffix=fNameSuffix;
}

ExtendedFileSink::~ExtendedFileSink() {
    delete[] fPerFrameFileNameBuffer;
    delete[] fPerFrameFileNamePrefix;
    delete[] fBuffer;
    if (fOutFid != NULL) fclose(fOutFid);
}

ExtendedFileSink* ExtendedFileSink::createNew(UsageEnvironment& env, char const* fNameSuffix, unsigned bufferSize, Boolean oneFilePerFrame) {
    do {

        FILE* fid;
        char const* perFrameFileNamePrefix;
        if (oneFilePerFrame) {
            // Create the fid for each frame
            fid = NULL;
            perFrameFileNamePrefix = fNameSuffix;
        } else {
            // Normal case: create the fid once
            fid = OpenOutputFile(env, fNameSuffix);
            if (fid == NULL) break;
            perFrameFileNamePrefix = NULL;
        }

        return new ExtendedFileSink(env, fid, bufferSize, perFrameFileNamePrefix, fNameSuffix);
    } while (0);

    return NULL;
}

Boolean ExtendedFileSink::continuePlaying() {
    if (fSource == NULL) return False;

    fSource->getNextFrame(fBuffer, fBufferSize, afterGettingFrame, this, onSourceClosure, this);

    return True;
}

void ExtendedFileSink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {
    ExtendedFileSink* sink = (ExtendedFileSink*)clientData;
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime);
}

void ExtendedFileSink::addData(unsigned char const* data, unsigned dataSize, struct timeval presentationTime) {
    if (data != NULL) {
        mongo::ScopedDbConnection c("localhost");
        mongo::GridFS gfs = mongo::GridFS(c.conn(), "grDB");

        std::stringstream ss;
        ss << presentationTime.tv_usec << fNameSuffix;

        gfs.storeFile(reinterpret_cast<const char*>(data), dataSize, ss.str());

        c.done();
    }
}

void ExtendedFileSink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime) {
    if (numTruncatedBytes > 0) {
        envir() << "ExtendedFileSink::afterGettingFrame(): The input frame data was too large for our buffer size ("
                << fBufferSize << ").  "
                << numTruncatedBytes << " bytes of trailing data was dropped!  Correct this by increasing the \"bufferSize\" parameter in the \"createNew()\" call to at least "
                << fBufferSize + numTruncatedBytes << "\n";
    }
    addData(fBuffer, frameSize, presentationTime);

    // TODO: Pruefen, ob das schreiben geklappt hat!

    // Then try getting the next frame:
    continuePlaying();
}
