#include "extendedfilesink.h"
#include <sstream>

/*!
 * \brief Protected constructor of the class.
 * \param env                   UsageEnvironment for usage in MediaSink constructor
 * \param bufferSize            Unsigned number which defines the buffersize for receiving data
 * \param tmpCamNameSuffix      Name of the file to save it in the database (on receiving data)
 */
ExtendedFileSink::ExtendedFileSink(UsageEnvironment& env, unsigned bufferSize, std::string tmpCamNameSuffix) : MediaSink(env), fBufferSize(bufferSize) {
    fBuffer = new unsigned char[bufferSize];
    this->stringfNameSuffix=tmpCamNameSuffix;
}

/*!
 * \brief Destructor - frees up the space used by the buffer.
 */
ExtendedFileSink::~ExtendedFileSink() {
    delete[] fBuffer;
}

/*!
 * \brief Factory pattern for creating a new instance of this class.
 * \param env                   UsageEnvironment for usage in MediaSink constructor
 * \param tmpCamNameSuffix      Name of the file to save it in the database (on receiving data)
 * \param bufferSize            Unsigned number which defines the buffersize for receiving data
 * \return                      new Instance of ExtendedFileSink
 */
ExtendedFileSink* ExtendedFileSink::createNew(UsageEnvironment& env, std::string tmpCamNameSuffix, unsigned bufferSize) {
    return new ExtendedFileSink(env, bufferSize, tmpCamNameSuffix);
}

/*!
 * \brief Helper function for determining if there is something to play
 * \return true, if something is there. false, if there is nothing.
 */
Boolean ExtendedFileSink::continuePlaying() {
    if (fSource == NULL) return False;

    fSource->getNextFrame(fBuffer, fBufferSize, afterGettingFrame, this, onSourceClosure, this);

    return True;
}

/*!
 * \brief Callback funtion for handling an incoming frame.
 * \param clientData            ExtendedFileSink-Object in which to write
 * \param frameSize             unsigned number representing the size of the received frame
 * \param numTruncatedBytes     unsigned number representing the number of truncated bytes (cut off, because frameSize > bufferSize)
 * \param presentationTime      timeval - since when is the stream running
 */
void ExtendedFileSink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {
    ExtendedFileSink* sink = (ExtendedFileSink*)clientData;
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime);
}

/*!
 * \brief Method for writing given data to the database
 *
 * most important / most called function of this class. It uses a mongo::ScopedDbConnection for creating a connection pool.
 * After creating the pool, it uses mongo::ScopedDbConnection.conn() (a single mongo connection) to create a mongo::GridFS Object.
 * This is used for writing files to the database grDB.
 * \param data              The data that has to be written. Usually, it is a single JPEG.
 * \param dataSize          Size of the JPEG
 * \param presentationTime  Stream is running for <presentationTime>
 */
void ExtendedFileSink::addData(unsigned char const* data, unsigned dataSize, struct timeval presentationTime) {
    if (data != NULL) {
        mongo::ScopedDbConnection c("localhost");
        mongo::GridFS gfs = mongo::GridFS(c.conn(), "grDB");

        std::stringstream ss;
        ss << presentationTime.tv_usec << this->stringfNameSuffix;
        std::cout <<"addData:" << this->stringfNameSuffix<< std::endl;

        gfs.storeFile(reinterpret_cast<const char*>(data), dataSize, this->stringfNameSuffix);

        c.done();
    }
}

/*!
 * \brief Callback function which is called, as soon as a frame is received
 * \param frameSize             The data that has to be written. Usually, it is a single JPEG.
 * \param numTruncatedBytes     Size of the JPEG
 * \param presentationTime      Stream is running for <presentationTime>
 */
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
