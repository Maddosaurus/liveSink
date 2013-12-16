//Anpassungen von Matthias Meidinger,
//Originaldatei: <LIVE555 v2013.09.08>/testProgs/testRTSPClient.cpp

//für die Kamera diesen rtsp-Path nutzen:
// rtsp://192.168.1.200:554/rtspjpeg480p

/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2013, Live Networks, Inc.  All rights reserved
// A demo application, showing how to create and run a RTSP client (that can potentially receive multiple streams concurrently).
//
// NOTE: This code - although it builds a running application - is intended only to illustrate how to develop your own RTSP
// client application.  For a full-featured RTSP client application - with much more functionality, and many options - see
// "openRTSP": http://www.live555.com/openRTSP/

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include "extendedfilesink.h"
#include "iostream"
#include "RTSPClient.hh"
#include <stdio.h>
#include <sstream>

unsigned int camCount = 0;



// Forward function definitions:

// RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);

// Other event handler functions:
void subsessionAfterPlaying(void* clientData); // called when a stream's subsession (e.g., audio or video substream) ends
void subsessionByeHandler(void* clientData); // called when a RTCP "BYE" is received for a subsession
void streamTimerHandler(void* clientData);
// called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")

// The main streaming routine (for each "rtsp://" URL):
void openURL(/*UsageEnvironment& env,*/ char const* progName, char const* rtspURL);

// Used to iterate through each stream's 'subsessions', setting up each one:
void setupNextSubsession(RTSPClient* rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):
void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);

// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient) {
    return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) {
    return env << subsession.mediumName() << "/" << subsession.codecName();
}

void usage(UsageEnvironment& env, char const* progName) {
    env << "Usage: " << progName << "-n <camname> <rtsp-url>\n";
    env << "\t(where <camname> is without spaces \n";
    env << "\tand <rtsp-url> is a \"rtsp://\" URL)\n";

}

char eventLoopWatchVariable = 0;

TaskScheduler* scheduler;
UsageEnvironment* env;
std::string camno;

/*!
 * \brief initializes the TaskScheduler and the UsageEnvironment,
 * \param -n
 * \param rtsp-URL URL of the rtsp-Stream used as MotionJPEG input stream
 * \return Never returns except for exit (Event driven infinite loop).
 */
int main(int argc, char** argv) {
    // Begin by setting up our usage environment:
    scheduler = BasicTaskScheduler::createNew();
    env= BasicUsageEnvironment::createNew(*scheduler);
    camno = "";

    // We need at least one "rtsp://" URL argument:
    if (argc != 4) {
        usage(*env, argv[0]);
        return 1;
    }

    //Parse the -n param
    char* const opt = argv[1];

    //check if it is -n
    if(opt[0] != '-') {
        usage(*env, argv[0]);
        return 1;
    } else if(opt[1] != 'n') {
        usage(*env, argv[0]);
        return 1;
    } else {
        camno = argv[2];
    }
    //open URL, which is first param after progName
    openURL(argv[0], argv[3]);

    // All subsequent activity takes place within the event loop:
    env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
    // This function call does not return, unless, at some point in time, "eventLoopWatchVariable" gets set to something non-zero.

    return 0;
}

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:
//! A class that is holding the stream state through a streams' lifetime
class StreamClientState {
public:
    StreamClientState();
    virtual ~StreamClientState();

public:
    MediaSubsessionIterator* iter;
    MediaSession* session;
    MediaSubsession* subsession;
    TaskToken streamTimerTask;
    double duration;
};



StreamClientState scs;

RTSPClient *rtspClient;

/// If set to 1, print verbose output from each "RTSPClient"
#define RTSP_CLIENT_VERBOSITY_LEVEL 1

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
/// Default streaming tactic is RTD/UDP. For RTP-over-TCP change to true
#define REQUEST_STREAMING_OVER_TCP False

/// Count, how many RTSPClient's are used by the server
static unsigned rtspClientCount = 0; // Counts how many streams (i.e., "RTSPClient"s) are currently in use.

/*!
 * \brief opens the given rtsp-URL and calls the rtsp DESCRIBE-command (with a callback-method)
 * \param progName  name of the calling program (i.e. argv[0])
 * \param rtspURL   rtsp-URL to be opnened (i.e. rtsp://10.10.10.10:554/rtspjpeg480p)
 */
void openURL(/*UsageEnvironment& env,*/ char const* progName, char const* rtspURL) {
    // Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
    // to receive (even if more than stream uses the same "rtsp://" URL).
    rtspClient = RTSPClient::createNew(*env, rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, progName);
    if (rtspClient == NULL) {
        *env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << *env->getResultMsg() << "\n";
        return;
    }

    ++rtspClientCount;

    // Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
    // Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
    // Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
    rtspClient->sendDescribeCommand(continueAfterDESCRIBE);
}


// Implementation of the RTSP 'response handlers':
/*!
 * \brief Fuction to be called to parse results of the rtsp DESCRIBE command
 *
 * It creates a MediaSession object for each given Session. This object is stored in the StreamClientState-Object (scs.session).
 * Finally it calls setupNextSubsession to add some detail info to the session.
 * \param rtspClient            calling client
 * \param resultCode            result of the DESCRIBE action
 * \param resultString          result of the DESCRIBE action
 */
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
    do {
        //UsageEnvironment& env = rtspClient->envir(); // alias
        //StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

        if (resultCode != 0) {
            *env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
            delete[] resultString;
            break;
        }

        char* const sdpDescription = resultString;
        *env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

        // Create a media session object from this SDP description:
        scs.session = MediaSession::createNew(*env, sdpDescription);
        delete[] sdpDescription; // because we don't need it anymore
        if (scs.session == NULL) {
            *env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << *env->getResultMsg() << "\n";
            break;
        } else if (!scs.session->hasSubsessions()) {
            *env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
            break;
        }

        // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
        // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
        // (Each 'subsession' will have its own data source.)
        scs.iter = new MediaSubsessionIterator(*scs.session);
        setupNextSubsession(rtspClient);
        return;
    } while (0);

    // An unrecoverable error occurred with this stream.
    shutdownStream(rtspClient);
}



/*!
 * \brief Sets up the next subsession which is stored in the scs.subsession object.
 *
 * This function sends the rtsp SETUP command to the source and sets up the callback function (continueAfterSETUP)
 * Changes by the author: If the subsession isn't a MotionJPEG, dismiss it.
 * \param rtspClient Environment vars
 */
void setupNextSubsession(RTSPClient* rtspClient) {
    scs.subsession = scs.iter->next();
    std::string result = scs.subsession->mediumName();        //Get the medium name
    std::string encoding = scs.subsession->codecName();
    if ((scs.subsession != NULL) && (result != "audio") && (encoding == "JPEG")) {     //We don't want audio. We want JPEG video for now, so ignore the rest
        if (!scs.subsession->initiate()) {
            *env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << *env->getResultMsg() << "\n";
            setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
        } else {
            *env << *rtspClient << "Initiated the \"" << *scs.subsession
                 << "\" subsession (client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1 << ")\n";
            // Continue setting up this subsession, by sending a RTSP "SETUP" command:
            rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
            //  }
        }
        return;
    }

    // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
    if (scs.session->absStartTime() != NULL) {
        // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
    } else {
        scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
    }
}

/*!
 * \brief Sets up the filesink for a rtsp subsession
 *
 * This function sets up the filesink for a given rtsp subsession, sets up the callback handler continueAfterPLAY() and sends the PLAY-command to the source (requesting it to start sending data, that is).
 * As it is only called after filtering all subsessions, it will be only called, if the given subsession is JPEG-encoded.
 * This ensures that the DBSink inserts only valid data into the DB.
 * > Changes by the author: Included the build process of a variable camname for saving it in the DB.
 * > Scheme: cam<camno>.jpeg
 *
 * \param rtspClient            Environment vars
 * \param resultCode            Result of the rtsp SETUP command
 * \param resultString          Result of the rtsp SETUP command
 */
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
    do {

        if (resultCode != 0) {
            *env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
            break;
        }

        *env << *rtspClient << "Set up the \"" << *scs.subsession
             << "\" subsession (client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1 << ")\n";

        // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
        // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
        // after we've sent a RTSP "PLAY" command.)


        //Neue Elemente von mir:
        //Wir bauen uns den Camnamen

        std::string camprefix = "cam";
        std::string camsuffix = ".jpeg";
        if(camno == "")
            camno = "xx";

        std::stringstream sstr;
        sstr << camprefix << camno << camsuffix;
        scs.subsession->sink = ExtendedFileSink::createNew(*env, sstr.str() , 60000);

        // Ende Neue Elemente von mir
        if (scs.subsession->sink == NULL) {
            *env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
                 << "\" subsession: " << *env->getResultMsg() << "\n";
            break;
        }
        else
            *env << "Stream creation succeeded! \n";

        *env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession" << "ID: " << scs.subsession->sink->name() << "\n";
        scs.subsession->miscPtr = rtspClient; // a hack to let subsession handle functions get the "RTSPClient" from the subsession
        scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
                                           subsessionAfterPlaying, scs.subsession);
        // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
        if (scs.subsession->rtcpInstance() != NULL) {
            scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
        }
    } while (0);
    delete[] resultString;

    // Set up the next subsession, if any:
    setupNextSubsession(rtspClient);
}

/*!
 * \brief Called as soon as a source stops streaming or the client requests a stop; Cleanup after streaming.
 * \param rtspClient            Environment vars
 * \param resultCode            Result of the rtsp PLAY command
 * \param resultString          Result of the rtsp PLAY command
 */
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
    Boolean success = False;

    do {

        if (resultCode != 0) {
            *env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
            break;
        }

        // Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
        // using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
        // 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
        // (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
        if (scs.duration > 0) {
            unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
            scs.duration += delaySlop;
            unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
            scs.streamTimerTask = env->taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
        }

        *env << *rtspClient << "Started playing session";
        if (scs.duration > 0) {
            *env << " (for up to " << scs.duration << " seconds)";
        }
        *env << "...\n";

        success = True;
    } while (0);
    delete[] resultString;

    if (!success) {
        // An unrecoverable error occurred with this stream.
        shutdownStream(rtspClient);
    }
}


// Implementation of the other event handlers:

void subsessionAfterPlaying(void* clientData) {
    MediaSubsession* subsession = (MediaSubsession*)clientData;
    RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

    // Begin by closing this subsession's stream:
    Medium::close(subsession->sink);
    subsession->sink = NULL;

    // Next, check whether *all* subsessions' streams have now been closed:
    MediaSession& session = subsession->parentSession();
    MediaSubsessionIterator iter(session);
    while ((subsession = iter.next()) != NULL) {
        if (subsession->sink != NULL) return; // this subsession is still active
    }

    // All subsessions' streams have now been closed, so shutdown the client:
    shutdownStream(rtspClient);
}

void subsessionByeHandler(void* clientData) {
    MediaSubsession* subsession = (MediaSubsession*)clientData;
    RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
    UsageEnvironment& env = rtspClient->envir(); // alias

    env << *rtspClient << "Received RTCP \"BYE\" on \"" << *subsession << "\" subsession\n";

    // Now act as if the subsession had closed:
    subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) {
    RTSPClient* rtspClient = (RTSPClient*)clientData;
    //StreamClientState& scs = rtspClient->scs; // alias

    scs.streamTimerTask = NULL;

    // Shut down the stream:
    shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient* rtspClient, int exitCode) {
    // First, check whether any subsessions have still to be closed:
    if (scs.session != NULL) {
        Boolean someSubsessionsWereActive = False;
        MediaSubsessionIterator iter(*scs.session);
        MediaSubsession* subsession;

        while ((subsession = iter.next()) != NULL) {
            if (subsession->sink != NULL) {
                Medium::close(subsession->sink);
                subsession->sink = NULL;

                if (subsession->rtcpInstance() != NULL) {
                    subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
                }

                someSubsessionsWereActive = True;
            }
        }

        if (someSubsessionsWereActive) {
            // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
            // Don't bother handling the response to the "TEARDOWN".
            rtspClient->sendTeardownCommand(*scs.session, NULL);
        }
    }

    *env << *rtspClient << "Closing the stream.\n";
    Medium::close(rtspClient);
    // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

    if (--rtspClientCount == 0) {
        // The final stream has ended, so exit the application now.
        // (Of course, if you're embedding this code into your own application, you might want to comment this out,
        // and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
        exit(exitCode);
    }
}


// Implementation of "StreamClientState":

StreamClientState::StreamClientState()
    : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) {
}

StreamClientState::~StreamClientState() {
    delete iter;
    if (session != NULL) {
        // We also need to delete "session", and unschedule "streamTimerTask" (if set)
        UsageEnvironment& env = session->envir(); // alias

        env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
        Medium::close(session);
    }
}



