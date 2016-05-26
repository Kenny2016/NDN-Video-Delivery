#ifndef LOGGINGTHREAD__H
#define LOGGINGTHREAD__H

#include "log.h"

#ifdef LOG_BUILD
namespace sampleplayer
{
	namespace log
	{
		std::chrono::time_point<std::chrono::system_clock> m_start_time;
		int flushThreshold;
		char *loggingBuffer;
		int loggingPosition;
		pthread_mutex_t logMutex;
		pthread_cond_t loggingCond;
		static volatile bool isRunning;
		pthread_t * logTh;
		const char* logFile;

		void Init()
		{
			loggingBuffer =  (char *)calloc(4096,sizeof(char));
			flushThreshold = 3500;
			loggingPosition = 0;
			pthread_mutex_init(&(logMutex),NULL);
			pthread_cond_init(&(loggingCond), NULL);
			isRunning = true;
			m_start_time = std::chrono::system_clock::now();
		}

		void Start(const char* data)
		{
			Init();
			logTh = (pthread_t*)malloc(sizeof(pthread_t));
			if (!logTh)
			{
				std::cerr << "Error allocating thread." << std::endl;
				logTh = NULL;
			}
			if(int err = pthread_create(logTh, NULL, sampleplayer::log::LogStart, (void *)data))
			{
				std::cerr << "Error creating the thread" << std::endl;
				logTh = NULL;
			}
		}
		void* LogStart(void * data)
		{
			logFile = (data == NULL) ? "log" : (char*) data;
			while(isRunning)
			{
				pthread_mutex_lock(&(logMutex));
				while(isRunning && loggingPosition < flushThreshold)
				{
					pthread_cond_wait(&(loggingCond), &(logMutex));
				}
				FILE *fp;
				fp = fopen(logFile, "a");
				fprintf(fp,"%s", loggingBuffer);
				fclose(fp);
				loggingPosition = 0;
				pthread_mutex_unlock(&(logMutex));
			}
		}

		void Stop()
		{
			isRunning = false;
			pthread_cond_signal(&(loggingCond));
		}

	}
}
#endif //LOG_BUILD

#endif //LOGGINGTHREAD__H
