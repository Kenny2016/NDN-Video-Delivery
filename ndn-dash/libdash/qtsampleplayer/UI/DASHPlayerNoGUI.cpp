/*
 * DASHPlayerNoGUI.cpp
 *****************************************************************************
 * Copyright (C) 2012, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#include "DASHPlayerNoGUI.h"
#include <iostream>

using namespace libdash::framework::adaptation;
using namespace libdash::framework::mpd;
using namespace libdash::framework::buffer;
using namespace sampleplayer;
using namespace sampleplayer::renderer;
using namespace sampleplayer::managers;
using namespace dash::mpd;
using namespace std;

DASHPlayerNoGUI::DASHPlayerNoGUI  (int argc, char ** argv, pthread_cond_t *mainCond) :
		mainCond	(mainCond),
		isRunning	(true)
{
	InitializeCriticalSection(&this->monitorMutex);

//	this->SetSettings(0, 0, 0, 0, 0);
	this->videoElement      = NULL;
	this->audioElement      = NULL;
	this->mpd 				= NULL;
	this->url 				= NULL;
	this->adaptLogic	 	= LogicType::RateBased;
	this->isNDN 			= false;
	this->multimediaManager = new MultimediaManager(this->videoElement, this->audioElement);
	this->multimediaManager->SetFrameRate(24);
	this->multimediaManager->AttachManagerObserver(this);
	this->parseArgs(argc, argv);

	if(this->url == NULL)
	{
		this->isRunning = false;
		pthread_cond_broadcast(mainCond);
		//delete(this);
		return;
	}
	else
	{
		if(this->OnDownloadMPDPressed(string(this->url)))
		{
			this->OnStartButtonPressed(0,0,0,0,0);
		}
		else
		{
			this->isRunning = false;
			pthread_cond_broadcast(mainCond);
		}

	}
}
DASHPlayerNoGUI::~DASHPlayerNoGUI ()
{
	this->multimediaManager->Stop();
	if(this->audioElement)
		this->audioElement->StopPlayback();
	this->audioElement = NULL;

	delete(this->multimediaManager);
	delete(this->audioElement);

	DeleteCriticalSection(&this->monitorMutex);
}

void DASHPlayerNoGUI::OnStartButtonPressed               (int period, int videoAdaptationSet, int videoRepresentation, int audioAdaptationSet, int audioRepresentation)
{
	this->OnSettingsChanged(period,videoAdaptationSet,videoRepresentation, audioAdaptationSet, audioRepresentation);
	if(!((this->multimediaManager->SetVideoAdaptationLogic((LogicType)this->adaptLogic) && (this->multimediaManager->SetAudioAdaptationLogic((LogicType)this->adaptLogic)))))
	{
		printf("Error setting Video/Audio adaptation logic...\n");
		return;
	}

	L("DASH PLAYER:	STARTING VIDEO\n");
	this->multimediaManager->Start(this->isNDN);
}
void DASHPlayerNoGUI::OnStopButtonPressed                ()
{
	this->multimediaManager->Stop();
	this->isRunning = false;
	pthread_cond_broadcast(mainCond);
}
bool DASHPlayerNoGUI::IsRunning							 ()
{
	return this->isRunning;
}
void DASHPlayerNoGUI::parseArgs							 (int argc, char ** argv)
{
	if(argc == 1)
	{
		helpMessage(argv[0]);
		return;
	}
	else
	{
		int i = 0;
		while(i < argc)
		{
			if(!strcmp(argv[i],"-u"))
			{
				this->url = argv[i+1];
				i++;
				i++;
				break;
			}
			if(!strcmp(argv[i],"-n"))
			{
				this->isNDN = true;
				i++;
				break;
			}
			if(!strcmp(argv[i],"-a"))
			{
				int j =0;
				for(j = 0; j < LogicType::Count; j++)
				{
					if(LogicType_string[j] == argv[i+1])
					{
						this->adaptLogic = (LogicType)j;
						break;
					}
				}
				if(j == LogicType::Count)
				{
					printf("the different adaptation logics implemented are:\n");
					for(j = 0;j < LogicType::Count; j++)
					{
						printf("*%s\n",LogicType_string[j]);
					}
					printf("By default, the %s logic is selected.\n", LogicType_string[this->adaptLogic]);
				}
				i++;
				i++;
				break;
			}
			i++;
		}
	}
}

void DASHPlayerNoGUI::helpMessage						 (char * name)
{
	printf("Usage: %s -u url -a adaptationLogic -n\n-u:\tThe MPD's url\n-a:\tThe adaptationLogic:\n\t*AlwaysLowest\n\t*RateBased(default)\n\t*BufferBased\n-n:\tFlag to use NDN instead of TCP\n", name);
}
void DASHPlayerNoGUI::OnSettingsChanged                  (int period, int videoAdaptationSet, int videoRepresentation, int audioAdaptationSet, int audioRepresentation)
{
	if(this->multimediaManager->GetMPD() == NULL)
		return; // TODO dialog or symbol that indicates that error

	if (!this->SettingsChanged(period, videoAdaptationSet, videoRepresentation, audioAdaptationSet, audioRepresentation))
		return;

	IPeriod                         *currentPeriod      = this->multimediaManager->GetMPD()->GetPeriods().at(period);
	std::vector<IAdaptationSet *>   videoAdaptationSets = AdaptationSetHelper::GetVideoAdaptationSets(currentPeriod);
	std::vector<IAdaptationSet *>   audioAdaptationSets = AdaptationSetHelper::GetAudioAdaptationSets(currentPeriod);

	if (videoAdaptationSet >= 0 && videoRepresentation >= 0)
	{
		this->multimediaManager->SetVideoQuality(currentPeriod,
				videoAdaptationSets.at(videoAdaptationSet),
				videoAdaptationSets.at(videoAdaptationSet)->GetRepresentation().at(videoRepresentation));
	}
	else
	{
		this->multimediaManager->SetVideoQuality(currentPeriod, NULL, NULL);
	}

	if (audioAdaptationSet >= 0 && audioRepresentation >= 0)
	{
		this->multimediaManager->SetAudioQuality(currentPeriod,
				audioAdaptationSets.at(audioAdaptationSet),
				audioAdaptationSets.at(audioAdaptationSet)->GetRepresentation().at(audioRepresentation));
	}
	else
	{
		this->multimediaManager->SetAudioQuality(currentPeriod, NULL, NULL);
	}
}
void DASHPlayerNoGUI::OnEOS								()
{
	this->OnStopButtonPressed();
}
bool DASHPlayerNoGUI::OnDownloadMPDPressed               (const std::string &url)
{
	if(!this->multimediaManager->Init(url))
	{
		return false; // TODO dialog or symbol that indicates that error
	}
	return true;
}
bool DASHPlayerNoGUI::SettingsChanged                    (int period, int videoAdaptationSet, int videoRepresentation, int audioAdaptationSet, int audioRepresentation)
{
	return true;
/*	EnterCriticalSection(&this->monitorMutex);

	bool settingsChanged = false;

	if (this->currentSettings.videoRepresentation != videoRepresentation ||
			this->currentSettings.audioRepresentation != audioRepresentation ||
			this->currentSettings.videoAdaptationSet != videoAdaptationSet ||
			this->currentSettings.audioAdaptationSet != audioAdaptationSet ||
			this->currentSettings.period != period)
		settingsChanged = true;

	if (settingsChanged)
		this->SetSettings(period, videoAdaptationSet, videoRepresentation, audioAdaptationSet, audioRepresentation);

	LeaveCriticalSection(&this->monitorMutex);

	return settingsChanged;
	*/
}
void DASHPlayerNoGUI::SetSettings                        (int period, int videoAdaptationSet, int videoRepresentation, int audioAdaptationSet, int audioRepresentation)
{
	this->currentSettings.period                = period;
	this->currentSettings.videoAdaptationSet    = videoAdaptationSet;
	this->currentSettings.videoRepresentation   = videoRepresentation;
	this->currentSettings.audioAdaptationSet    = audioAdaptationSet;
	this->currentSettings.audioRepresentation   = audioRepresentation;
}

void DASHPlayerNoGUI::OnVideoBufferStateChanged          (uint32_t fillstateInPercent)
{

}
void DASHPlayerNoGUI::OnVideoSegmentBufferStateChanged   (uint32_t fillstateInPercent)
{

}
void DASHPlayerNoGUI::OnAudioBufferStateChanged          (uint32_t fillstateInPercent)
{

}
void DASHPlayerNoGUI::OnAudioSegmentBufferStateChanged   (uint32_t fillstateInPercent)
{

}
