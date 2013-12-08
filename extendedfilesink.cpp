#include "extendedfilesink.h"
#include <sstream>

ExtendedFileSink::ExtendedFileSink(UsageEnvironment& env, FILE* fid, unsigned bufferSize, std::string tmpCamNameSuffix) : MediaSink(env), fOutFid(fid), fBufferSize(bufferSize) {
    fBuffer = new unsigned char[bufferSize];
    this->stringfNameSuffix=tmpCamNameSuffix;
    std::cout <<"EFS: local ---" << this->stringfNameSuffix<< std::endl;
    std::cout <<"EFS: global ---" << ExtendedFileSink::stringfNameSuffix<< std::endl;
}

ExtendedFileSink::~ExtendedFileSink() {
    delete[] fBuffer;
    //delete[] ExtendedFileSink::fNameSuffix;
}

ExtendedFileSink* ExtendedFileSink::createNew(UsageEnvironment& env, std::string tmpCamNameSuffix, unsigned bufferSize) {
    return new ExtendedFileSink(env, /*fid*/NULL, bufferSize, tmpCamNameSuffix);
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
        ss << presentationTime.tv_usec << this->stringfNameSuffix;
        std::cout <<"addData:" << this->stringfNameSuffix<< std::endl;

        gfs.storeFile(reinterpret_cast<const char*>(data), dataSize, this->stringfNameSuffix);//ss.str());

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

    ExtendedFileSink::addData(fBuffer, frameSize, presentationTime);

    // TODO: Pruefen, ob das schreiben geklappt hat!

    // Then try getting the next frame:
    continuePlaying();
}
