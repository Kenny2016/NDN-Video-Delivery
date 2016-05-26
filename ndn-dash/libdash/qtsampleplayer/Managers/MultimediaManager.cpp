/*
 * MultimediaManager.cpp
 *****************************************************************************
 * Copyright (C) 2013, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#include "MultimediaManager.h"

using namespace libdash::framework::adaptation;
using namespace libdash::framework::buffer;
using namespace sampleplayer::managers;
using namespace sampleplayer::renderer;
using namespace dash::mpd;

#define SEGMENTBUFFER_SIZE 5

MultimediaManager::MultimediaManager            (QTGLRenderer *videoElement, QTAudioRenderer *audioElement) :
                   videoElement                 (videoElement),
                   audioElement                 (audioElement),
                   mpd                          (NULL),
                   period                       (NULL),
                   videoAdaptationSet           (NULL),
                   videoRepresentation          (NULL),
                   videoLogic                   (NULL),
                   videoStream                  (NULL),
                   audioAdaptationSet           (NULL),
                   audioRepresentation          (NULL),
                   audioLogic                   (NULL),
				   videoRendererHandle			(NULL),
				   audioRendererHandle			(NULL),
				   audioStream                  (NULL),
                   isStarted                    (false),
				   isStopping					(false),
                   framesDisplayed              (0),
                   segmentsDownloaded           (0),
                   isVideoRendering             (false),
                   isAudioRendering             (false),
				   eos							(false)
{
    InitializeCriticalSection (&this->monitorMutex);

    this->manager = CreateDashManager();
   // av_register_all();
}
MultimediaManager::~MultimediaManager           ()
{
    this->Stop();
    this->manager->Delete();

    DeleteCriticalSection (&this->monitorMutex);
}

IMPD*   MultimediaManager::GetMPD                           ()
{
    return this->mpd;
}
bool    MultimediaManager::Init                             (const std::string& url)
{
    EnterCriticalSection(&this->monitorMutex);

    this->mpd = this->manager->Open((char *)url.c_str());

    if(this->mpd == NULL)
    {
        LeaveCriticalSection(&this->monitorMutex);
        return false;
    }

    LeaveCriticalSection(&this->monitorMutex);
    return true;
}
bool	MultimediaManager::IsStarted						()
{
	return this->isStarted;
}
bool 	MultimediaManager::IsStopping						()
{
	return this->isStopping;
}
bool	MultimediaManager::IsNDN							()
{
	return this->isNDN;
}
void    MultimediaManager::Start                            (bool ndnEnabled)
{
	this->isNDN = ndnEnabled;
    /* Global Start button for start must be added to interface*/
    if (this->isStarted)
        this->Stop();

    L("Starting MultimediaManager...\n");
    L("Is_NDN:\t%s\n", isNDN ? "true" : "false");
    L("Segment_Buffer_size:\t%d\n", SEGMENTBUFFER_SIZE);
    L("Adaptation_rate:\t%s\n", this->logicName);
    EnterCriticalSection(&this->monitorMutex);

    if (this->videoAdaptationSet && this->videoRepresentation)
    {
        this->InitVideoRendering(0);
        this->videoStream->SetAdaptationLogic(this->videoLogic);
        this->videoLogic->SetMultimediaManager(this);
        this->videoStream->Start();
        this->StartVideoRenderingThread();
    }

    if (this->audioAdaptationSet && this->audioRepresentation)
    {
        this->InitAudioPlayback(0);
        if(this->audioElement)
        	this->audioElement->StartPlayback();
        this->audioStream->SetAdaptationLogic(this->audioLogic);
        this->audioLogic->SetMultimediaManager(this);
        this->audioStream->Start();
        this->StartAudioRenderingThread();
    }

    this->isStarted = true;

    LeaveCriticalSection(&this->monitorMutex);
}
void    MultimediaManager::Stop                             ()
{
    if (!this->isStarted)
        return;

    this->isStopping = true;
    EnterCriticalSection(&this->monitorMutex);

    this->StopVideo();
    this->StopAudio();

    this->isStopping = false;
    this->isStarted = false;

    LeaveCriticalSection(&this->monitorMutex);
    L("VIDEO STOPPED\n");
    FlushLog();
}
void    MultimediaManager::StopVideo                        ()
{
    if(this->isStarted && this->videoStream)
    {
        this->videoStream->Stop();
        this->StopVideoRenderingThread();

        delete this->videoStream;
        delete this->videoLogic;

        this->videoStream = NULL;
        this->videoLogic = NULL;
    }
}
void    MultimediaManager::StopAudio                        ()
{
    if (this->isStarted && this->audioStream)
    {
        this->audioStream->Stop();
        this->StopAudioRenderingThread();

        if(this->audioElement)
        	this->audioElement->StopPlayback();

        delete this->audioStream;
        delete this->audioLogic;

        this->audioStream = NULL;
        this->audioLogic = NULL;
    }
}
bool    MultimediaManager::SetVideoQuality                  (IPeriod* period, IAdaptationSet *adaptationSet, IRepresentation *representation)
{
    EnterCriticalSection(&this->monitorMutex);

    this->period                = period;
    this->videoAdaptationSet    = adaptationSet;
    this->videoRepresentation   = representation;

    if (this->videoStream)
        this->videoStream->SetRepresentation(this->period, this->videoAdaptationSet, this->videoRepresentation);

    LeaveCriticalSection(&this->monitorMutex);
    return true;
}
bool    MultimediaManager::SetAudioQuality                  (IPeriod* period, IAdaptationSet *adaptationSet, IRepresentation *representation)
{
    EnterCriticalSection(&this->monitorMutex);

    this->period                = period;
    this->audioAdaptationSet    = adaptationSet;
    this->audioRepresentation   = representation;

    if (this->audioStream)
        this->audioStream->SetRepresentation(this->period, this->audioAdaptationSet, this->audioRepresentation);

    LeaveCriticalSection(&this->monitorMutex);
    return true;
}
bool	MultimediaManager::IsUserDependent					()
{
	if(this->videoLogic)
		return this->videoLogic->IsUserDependent();
	else
		return true;
}
bool    MultimediaManager::SetVideoAdaptationLogic          (libdash::framework::adaptation::LogicType type)
{
	if(this->videoAdaptationSet)
	{
		this->videoLogic = AdaptationLogicFactory::Create(type, this->mpd, this->period, this->videoAdaptationSet, 1);
		this->logicName = LogicType_string[type];
	}
	else
	{
		this->videoLogic = NULL;
		return true;
	}
	if(this->videoLogic)
		return true;
	else
		return false;
}
void 	MultimediaManager::ShouldAbort							(bool isVideo)
{
	if(isVideo)
	{
		this->videoStream->ShouldAbort();
		return;
	}
	else
	{
		this->audioStream->ShouldAbort();
	}
}
bool    MultimediaManager::SetAudioAdaptationLogic          (libdash::framework::adaptation::LogicType type)
{
	if(this->audioAdaptationSet)
	{
		this->audioLogic = AdaptationLogicFactory::Create(type, this->mpd, this->period, this->audioAdaptationSet, 0);
		this->logicName = LogicType_string[type];
	}
	else
	{
		this->audioLogic = NULL;
		return true;
	}
	if(this->audioLogic)
		return true;
	else
		return false;
}
void    MultimediaManager::AttachManagerObserver            (IMultimediaManagerObserver *observer)
{
    this->managerObservers.push_back(observer);
}
void    MultimediaManager::NotifyVideoBufferObservers       (uint32_t fillstateInPercent)
{
    for (size_t i = 0; i < this->managerObservers.size(); i++)
        this->managerObservers.at(i)->OnVideoBufferStateChanged(fillstateInPercent);
}
void    MultimediaManager::NotifyVideoSegmentBufferObservers(uint32_t fillstateInPercent)
{
    for (size_t i = 0; i < this->managerObservers.size(); i++)
        this->managerObservers.at(i)->OnVideoSegmentBufferStateChanged(fillstateInPercent);
}
void    MultimediaManager::NotifyAudioSegmentBufferObservers(uint32_t fillstateInPercent)
{
    for (size_t i = 0; i < this->managerObservers.size(); i++)
        this->managerObservers.at(i)->OnAudioSegmentBufferStateChanged(fillstateInPercent);
}
void    MultimediaManager::NotifyAudioBufferObservers       (uint32_t fillstateInPercent)
{
    for (size_t i = 0; i < this->managerObservers.size(); i++)
        this->managerObservers.at(i)->OnAudioBufferStateChanged(fillstateInPercent);
}
void    MultimediaManager::InitVideoRendering               (uint32_t offset)
{
//    this->videoLogic = AdaptationLogicFactory::Create(libdash::framework::adaptation::Manual, this->mpd, this->period, this->videoAdaptationSet);

    this->videoStream = new MultimediaStream(sampleplayer::managers::VIDEO, this->mpd, SEGMENTBUFFER_SIZE, 2, 0, this->IsNDN());
    this->videoStream->AttachStreamObserver(this);
    this->videoStream->SetRepresentation(this->period, this->videoAdaptationSet, this->videoRepresentation);
    this->videoStream->SetPosition(offset);
}
void    MultimediaManager::InitAudioPlayback                (uint32_t offset)
{
 //   this->audioLogic = AdaptationLogicFactory::Create(libdash::framework::adaptation::Manual, this->mpd, this->period, this->audioAdaptationSet);

    this->audioStream = new MultimediaStream(sampleplayer::managers::AUDIO, this->mpd, SEGMENTBUFFER_SIZE, 0, 10, this->IsNDN());
    this->audioStream->AttachStreamObserver(this);
    this->audioStream->SetRepresentation(this->period, this->audioAdaptationSet, this->audioRepresentation);
    this->audioStream->SetPosition(offset);
}
void    MultimediaManager::OnSegmentDownloaded              ()
{
    this->segmentsDownloaded++;
}
void    MultimediaManager::OnSegmentBufferStateChanged    (StreamType type, uint32_t fillstateInPercent)
{
    switch (type)
    {
        case AUDIO:
            this->NotifyAudioSegmentBufferObservers(fillstateInPercent);
            break;
        case VIDEO:
            this->NotifyVideoSegmentBufferObservers(fillstateInPercent);
            break;
        default:
            break;
    }
}
void    MultimediaManager::OnVideoBufferStateChanged      (uint32_t fillstateInPercent)
{
    this->NotifyVideoBufferObservers(fillstateInPercent);
}
void    MultimediaManager::OnAudioBufferStateChanged      (uint32_t fillstateInPercent)
{
    this->NotifyAudioBufferObservers(fillstateInPercent);
}
void    MultimediaManager::SetFrameRate                     (double framerate)
{
    this->frameRate = framerate;
}

void 	MultimediaManager::SetEOS							(bool value)
{
	this->eos = value;
	if(value) //ie: End of Stream so the rendering thread(s) will finish
	{
		this->isStopping = true;
		if(this->videoRendererHandle != NULL)
		{
			JoinThread(this->videoRendererHandle);
			DestroyThreadPortable(this->videoRendererHandle);
			this->videoRendererHandle = NULL;
		}
		if(this->audioRendererHandle != NULL)
		{
			JoinThread(this->audioRendererHandle);
			DestroyThreadPortable(this->audioRendererHandle);
			this->audioRendererHandle = NULL;
		}
		this->isStopping = false;
		for(size_t i = 0; i < this->managerObservers.size(); i++)
			this->managerObservers.at(0)->OnEOS();
	}
}

bool    MultimediaManager::StartVideoRenderingThread        ()
{
    this->isVideoRendering = true;

    this->videoRendererHandle = CreateThreadPortable (RenderVideo, this);

    if(this->videoRendererHandle == NULL)
        return false;

    return true;
}
void    MultimediaManager::StopVideoRenderingThread         ()
{
    this->isVideoRendering = false;

    if (this->videoRendererHandle != NULL)
    {
        JoinThread(this->videoRendererHandle);
        DestroyThreadPortable(this->videoRendererHandle);
    }
}
bool    MultimediaManager::StartAudioRenderingThread        ()
{
    this->isAudioRendering = true;

    this->audioRendererHandle = CreateThreadPortable (RenderAudio, this);

    if(this->audioRendererHandle == NULL)
        return false;

    return true;
}
void    MultimediaManager::StopAudioRenderingThread         ()
{
    this->isAudioRendering = false;

    if (this->audioRendererHandle != NULL)
    {
        JoinThread(this->audioRendererHandle);
        DestroyThreadPortable(this->audioRendererHandle);
    }
}
void*   MultimediaManager::RenderVideo        (void *data)
{
    MultimediaManager *manager = (MultimediaManager*) data;
    QImage *frame = manager->videoStream->GetFrame();

    while(manager->isVideoRendering)
    {
        if (frame)
        {
        	if(manager->videoElement)
        	{
        		manager->videoElement->SetImage(frame);
        		manager->videoElement->update();
        	}
            manager->framesDisplayed++;

            PortableSleep(1 / manager->frameRate);
            delete(frame);
        }
        else
        {
        	if(manager->eos)
        		break;
        }
        frame = manager->videoStream->GetFrame();
    }

    return NULL;
}
void*   MultimediaManager::RenderAudio        (void *data)
{
	MultimediaManager *manager = (MultimediaManager*) data;

    AudioChunk *samples = manager->audioStream->GetSamples();

    while(manager->isAudioRendering)
    {
        if (samples)
        {
        	if(manager->audioElement)
        	{
        		manager->audioElement->WriteToBuffer(samples->Data(), samples->Length());
        	}

            PortableSleep(1 / (double)48000);
            delete samples;
        }
        else
        	if(manager->eos)
        		break;

        samples = manager->audioStream->GetSamples();
    }

    return NULL;
}
