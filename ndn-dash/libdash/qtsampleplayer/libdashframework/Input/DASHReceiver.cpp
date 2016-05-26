/*
 * DASHReceiver.cpp
 *****************************************************************************
 * Copyright (C) 2012, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#include "DASHReceiver.h"
#include<stdio.h>

using namespace libdash::framework::input;
using namespace libdash::framework::buffer;
using namespace libdash::framework::mpd;
using namespace dash::mpd;

DASHReceiver::DASHReceiver          (IMPD *mpd, IDASHReceiverObserver *obs, MediaObjectBuffer *buffer, uint32_t bufferSize,bool ndnEnabled) :
              mpd                   (mpd),
              period                (NULL),
              adaptationSet         (NULL),
              representation        (NULL),
              adaptationSetStream   (NULL),
              representationStream  (NULL),
              segmentNumber         (0),
              observer              (obs),
              buffer                (buffer),
              bufferSize            (bufferSize),
              isBuffering           (false),
			  withFeedBack			(false),
			  isNDN					(ndnEnabled)
{
    this->period                = this->mpd->GetPeriods().at(0);
    this->adaptationSet         = this->period->GetAdaptationSets().at(0);
    this->representation        = this->adaptationSet->GetRepresentation().at(0);

    this->adaptationSetStream   = new AdaptationSetStream(mpd, period, adaptationSet);
    this->representationStream  = adaptationSetStream->GetRepresentationStream(this->representation);
    this->segmentOffset         = CalculateSegmentOffset();

    this->conn = NULL;
    if(isNDN)
    	this->conn = new NDNConnection();

    InitializeCriticalSection(&this->monitorMutex);
}
DASHReceiver::~DASHReceiver		()
{
    delete this->adaptationSetStream;
    DeleteCriticalSection(&this->monitorMutex);
}

void			DASHReceiver::SetAdaptationLogic(adaptation::IAdaptationLogic *_adaptationLogic)
{
	this->adaptationLogic = _adaptationLogic;

	this->withFeedBack = this->adaptationLogic->IsRateBased();
}
bool			DASHReceiver::Start						()
{
    if(this->isBuffering)
        return false;

    this->isBuffering       = true;
    this->bufferingThread   = CreateThreadPortable (DoBuffering, this);

    if(this->bufferingThread == NULL)
    {
        this->isBuffering = false;
        return false;
    }

    return true;
}
void			DASHReceiver::Stop						()
{
    if(!this->isBuffering)
        return;

    this->isBuffering = false;
    this->buffer->SetEOS(true);

    if(this->bufferingThread != NULL)
    {
        JoinThread(this->bufferingThread);
        DestroyThreadPortable(this->bufferingThread);
    }
}
MediaObject*	DASHReceiver::GetNextSegment	()
{
    ISegment *seg = NULL;

    if(this->segmentNumber >= this->representationStream->GetSize())
        return NULL;

    seg = this->representationStream->GetMediaSegment(this->segmentNumber + this->segmentOffset);

    if (seg != NULL)
    {
    	std::vector<IRepresentation *> rep = this->adaptationSet->GetRepresentation();
    	int quality = 0;
    	while(quality != rep.size() - 1)
    	{
    		if(rep.at(quality) == this->representation)
    			break;
    		quality++;
    	}
    	Debug("DASH receiver: Next segment is: %s / %s\n",((dash::network::IChunk*)seg)->Host().c_str(),((dash::network::IChunk*)seg)->Path().c_str());
    	L("DASH_Receiver:\t%s\t%d\n", ((dash::network::IChunk*)seg)->Path().c_str() ,quality);
    												//this->representation->GetBandwidth());
        MediaObject *media = new MediaObject(seg, this->representation,this->withFeedBack);
        this->segmentNumber++;
        return media;
    }

    return NULL;
}
MediaObject*	DASHReceiver::GetSegment		(uint32_t segNum)
{
    ISegment *seg = NULL;

    if(segNum >= this->representationStream->GetSize())
        return NULL;

    seg = this->representationStream->GetMediaSegment(segNum + segmentOffset);

    if (seg != NULL)
    {
        MediaObject *media = new MediaObject(seg, this->representation);
        return media;
    }

    return NULL;
}
MediaObject*	DASHReceiver::GetInitSegment	()
{
    ISegment *seg = NULL;

    seg = this->representationStream->GetInitializationSegment();

    if (seg != NULL)
    {
        MediaObject *media = new MediaObject(seg, this->representation);
        return media;
    }

    return NULL;
}
MediaObject*	DASHReceiver::FindInitSegment	(dash::mpd::IRepresentation *representation)
{
    if (!this->InitSegmentExists(representation))
        return NULL;

    return this->initSegments[representation];
}
uint32_t                    DASHReceiver::GetPosition               ()
{
    return this->segmentNumber;
}
void                        DASHReceiver::SetPosition               (uint32_t segmentNumber)
{
    // some logic here

    this->segmentNumber = segmentNumber;
}
void                        DASHReceiver::SetPositionInMsecs        (uint32_t milliSecs)
{
    // some logic here

    this->positionInMsecs = milliSecs;
}
void                        DASHReceiver::SetRepresentation         (IPeriod *period, IAdaptationSet *adaptationSet, IRepresentation *representation)
{
    EnterCriticalSection(&this->monitorMutex);

    bool periodChanged = false;

    if (this->representation == representation)
    {
        LeaveCriticalSection(&this->monitorMutex);
        return;
    }

    this->representation = representation;

    if (this->adaptationSet != adaptationSet)
    {
        this->adaptationSet = adaptationSet;

        if (this->period != period)
        {
            this->period = period;
            periodChanged = true;
        }

        delete this->adaptationSetStream;
        this->adaptationSetStream = NULL;

        this->adaptationSetStream = new AdaptationSetStream(this->mpd, this->period, this->adaptationSet);
    }

    this->representationStream  = this->adaptationSetStream->GetRepresentationStream(this->representation);
    this->DownloadInitSegment(this->representation);

    if (periodChanged)
    {
        this->segmentNumber = 0;
        this->CalculateSegmentOffset();
    }

    LeaveCriticalSection(&this->monitorMutex);
}

libdash::framework::adaptation::IAdaptationLogic* DASHReceiver::GetAdaptationLogic	()
{
	return this->adaptationLogic;
}
dash::mpd::IRepresentation* DASHReceiver::GetRepresentation         ()
{
    return this->representation;
}
uint32_t                    DASHReceiver::CalculateSegmentOffset    ()
{
    if (mpd->GetType() == "static")
        return 0;

    uint32_t firstSegNum = this->representationStream->GetFirstSegmentNumber();
    uint32_t currSegNum  = this->representationStream->GetCurrentSegmentNumber();
    uint32_t startSegNum = currSegNum - 2*bufferSize;

    return (startSegNum > firstSegNum) ? startSegNum : firstSegNum;
}
void                        DASHReceiver::NotifySegmentDownloaded   ()
{
    this->observer->OnSegmentDownloaded();
}
void						DASHReceiver::NotifyBitrateChange(dash::mpd::IRepresentation *representation)
{
	if(this->representation != representation)
	{
		this->representation = representation;
		this->SetRepresentation(this->period,this->adaptationSet,this->representation);
	}
}
void                        DASHReceiver::DownloadInitSegment    (IRepresentation* rep)
{
    if (this->InitSegmentExists(rep))
        return;

    MediaObject *initSeg = NULL;
    initSeg = this->GetInitSegment();

    if (initSeg)
    {
        initSeg->StartDownload(this->conn);
        this->initSegments[rep] = initSeg;
        initSeg->WaitFinished();
    }
}
bool                        DASHReceiver::InitSegmentExists      (IRepresentation* rep)
{
    if (this->initSegments.find(rep) != this->initSegments.end())
        return true;

    return false;
}

void					DASHReceiver::Notifybps					(uint64_t bps)
{
	if(this)
	{
		if(this->adaptationLogic)
		{
			if(this->withFeedBack)
			{
				this->adaptationLogic->BitrateUpdate(bps);
			}
		}
	}
}
//Is only called when this->adaptationLogic->IsBufferBased
void 					DASHReceiver::OnSegmentBufferStateChanged(uint32_t fillstateInPercent)
{
	this->adaptationLogic->BufferUpdate(fillstateInPercent);
}
void					DASHReceiver::OnEOS(bool value)
{
	this->adaptationLogic->OnEOS(value);
}
/* Thread that does the buffering of segments */
void*                       DASHReceiver::DoBuffering               (void *receiver)
{
    DASHReceiver *dashReceiver = (DASHReceiver *) receiver;

    dashReceiver->DownloadInitSegment(dashReceiver->GetRepresentation());

    MediaObject *media = dashReceiver->GetNextSegment();
    media->SetDASHReceiver(dashReceiver);
    while(media != NULL && dashReceiver->isBuffering)
    {
        media->StartDownload(dashReceiver->conn);

        if (!dashReceiver->buffer->PushBack(media))
            return NULL;

        media->WaitFinished();
        dashReceiver->NotifySegmentDownloaded();
        media = dashReceiver->GetNextSegment();
        if(media)
        	media->SetDASHReceiver(dashReceiver);
    }

    dashReceiver->buffer->SetEOS(true);
    return NULL;
}

void					DASHReceiver::ShouldAbort				()
{
	this->segmentNumber--;
	this->buffer->ShouldAbort();
	printf("Aborted\n");
}
