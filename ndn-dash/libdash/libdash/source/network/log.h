#ifndef LOGDEFINITIONFILE__H
#define LOGDEFINITIONFILE__H

//#define LOG_BUILD

#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <chrono>
#include <ratio>

#ifdef LOG_BUILD

//Logging time is done here
//format of the logging would be: "[timestamp_since_start(in seconds)] logging message"

using duration_in_seconds = std::chrono::duration<double, std::ratio<1,1> >;

namespace sampleplayer
{
	namespace log
	{
		extern std::chrono::time_point<std::chrono::system_clock> m_start_time;
		extern int flushThreshold;
		extern char *loggingBuffer;
		extern int loggingPosition;
		extern pthread_mutex_t logMutex;
		extern pthread_cond_t loggingCond;
		extern pthread_t * logTh;
		extern const char* logFile;
		extern void Init();
		extern void Stop();
		extern void* LogStart(void * data);
		extern void Start(const char* data);
	}
}
	//We need this to flush the log after the video ends (or when the user stops the video)
	#define	FlushLog() do { sampleplayer::log::Stop();			\
							pthread_join(*(sampleplayer::log::logTh), NULL);		\
							sampleplayer::log::Start(sampleplayer::log::logFile);	\
						  } while(0)

	#define L(...) do { double now = std::chrono::duration_cast<duration_in_seconds>(std::chrono::system_clock::now() - sampleplayer::log::m_start_time).count();\
						pthread_mutex_lock(&(sampleplayer::log::logMutex)); \
						sampleplayer::log::loggingPosition += sprintf(sampleplayer::log::loggingBuffer + sampleplayer::log::loggingPosition, "[%f]", now); \
						sampleplayer::log::loggingPosition += sprintf(sampleplayer::log::loggingBuffer + sampleplayer::log::loggingPosition, __VA_ARGS__); \
						pthread_cond_broadcast(&(sampleplayer::log::loggingCond)); \
						pthread_mutex_unlock(&(sampleplayer::log::logMutex));  \
					} while(0)

#else
	#define FlushLog() do {} while(0)
	#define L(...) do {} while(0)

#endif //LOG_BUILD

#endif //LOGDEFINITIONFILE__H
