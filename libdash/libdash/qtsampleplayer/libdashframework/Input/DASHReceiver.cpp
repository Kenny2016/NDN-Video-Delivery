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

using namespace libdash::framework::input;
using namespace libdash::framework::buffer;
using namespace libdash::framework::mpd;
using namespace dash::mpd;

DASHReceiver::DASHReceiver          (IMPD *mpd, IDASHReceiverObserver *obs, MediaObjectBuffer *buffer, uint32_t bufferSize) :
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
              isBuffering           (false)
{
    std::cout<<"revoke dash receiver"<<std::endl;
    this->period                = this->mpd->GetPeriods().at(0);
    this->adaptationSet         = this->period->GetAdaptationSets().at(0);
    this->representation        = this->adaptationSet->GetRepresentation().at(0);

    this->adaptationSetStream   = new AdaptationSetStream(mpd, period, adaptationSet);
    this->representationStream  = adaptationSetStream->GetRepresentationStream(this->representation);
    this->segmentOffset         = CalculateSegmentOffset();

    std::cout<<"start create ndn non parameter conn"<<std::endl;
    this->conn = new NDNConnection();
    std::cout << "start ndn connection finished" << std::endl;

    InitializeCriticalSection(&this->monitorMutex);
    std::cout << "init critical section finished" << std::endl;
}
DASHReceiver::~DASHReceiver     ()
{
    delete this->adaptationSetStream;
    DeleteCriticalSection(&this->monitorMutex);
}

bool                        DASHReceiver::Start                     ()
{
    std::cout << "dashreceiver::start" << std::endl;
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
void                        DASHReceiver::Stop                      ()
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
MediaObject*                DASHReceiver::GetNextSegment            ()
{
    std::cout << "dashreceiver::getnextsegment" << std::endl;
    ISegment *seg = NULL;

    if(this->segmentNumber >= this->representationStream->GetSize())
        return NULL;

    seg = this->representationStream->GetMediaSegment(this->segmentNumber + this->segmentOffset);

    if (seg != NULL)
    {
        MediaObject *media = new MediaObject(seg, this->representation);
        this->segmentNumber++;
        return media;
    }

    return NULL;
}
MediaObject*                DASHReceiver::GetSegment                (uint32_t segNum)
{
    std::cout << "dashreceiver::getsegment" << std::endl;
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
MediaObject*                DASHReceiver::GetInitSegment            ()
{
    std::cout << "dashreceiver::getinitsegment" << std::endl;
    ISegment *seg = NULL;

    seg = this->representationStream->GetInitializationSegment();
    std::cout<<"initializationseg "<<seg<<std::endl;

    if (seg != NULL)
    {
        std::cout << "dashreceiver::getinitseg::seg!=null...." << std::endl;
        MediaObject *media = new MediaObject(seg, this->representation);
        return media;
    }

    return NULL;
}
MediaObject*                DASHReceiver::FindInitSegment           (dash::mpd::IRepresentation *representation)
{
    std::cout << "dashreceiver::findinitsegment" << std::endl;
    if (!this->InitSegmentExists(representation))
        return NULL;

    return this->initSegments[representation];
}
uint32_t                    DASHReceiver::GetPosition               ()
{
    std::cout << "dashreceiver::getpositionr" << std::endl;
    return this->segmentNumber;
}
void                        DASHReceiver::SetPosition               (uint32_t segmentNumber)
{
    // some logic here
    std::cout << "dashreceiver::setposition" << std::endl;
    this->segmentNumber = segmentNumber;
}
void                        DASHReceiver::SetPositionInMsecs        (uint32_t milliSecs)
{
    // some logic here

    this->positionInMsecs = milliSecs;
}
void                        DASHReceiver::SetRepresentation         (IPeriod *period, IAdaptationSet *adaptationSet, IRepresentation *representation)
{
    std::cout << "dashreceiver::setrepresentation" << std::endl;
    std::cout << "representation"<<representation<<" adaptationset " <<adaptationSet<<"duration "<<period->GetDuration()<< std::endl;
    EnterCriticalSection(&this->monitorMutex);

    bool periodChanged = false;

    if (this->representation == representation)
    {
        LeaveCriticalSection(&this->monitorMutex);
        std::cout << "dashreceiver::setrepresentation::representation " << representation->GetSegmentList()<<std::endl;
        return;
    }

    this->representation = representation;

    if (this->adaptationSet != adaptationSet)
    {
        std::cout << "dashreceiver::setrepresentation::adaptationset" << std::endl;
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
dash::mpd::IRepresentation* DASHReceiver::GetRepresentation         ()
{
    std::cout << "dashreceiver::get representation" << std::endl;
    return this->representation;
}
uint32_t                    DASHReceiver::CalculateSegmentOffset    ()
{
    std::cout << "dashreceiver::calsulatesegmentoffset" << std::endl;
    if (mpd->GetType() == "static")
    //{ std::cout<<"static"<<std::endl;
    //return 0;}
        return 0;

    uint32_t firstSegNum = this->representationStream->GetFirstSegmentNumber();
    uint32_t currSegNum  = this->representationStream->GetCurrentSegmentNumber();
    uint32_t startSegNum = currSegNum - 2*bufferSize;
    std::cout<<firstSegNum<<" " <<currSegNum<<" "<<startSegNum<<std::endl;

    return (startSegNum > firstSegNum) ? startSegNum : firstSegNum;
}
void                        DASHReceiver::NotifySegmentDownloaded   ()
{
    std::cout << "dashreceiver::notifysegmentdownload" << std::endl;
    this->observer->OnSegmentDownloaded();
}
void                        DASHReceiver::DownloadInitSegment    (IRepresentation* rep)
{
    std::cout << "dashreceiver::downloadinitsegment" << std::endl;
    if (this->InitSegmentExists(rep))
        return;



    MediaObject *initSeg = NULL;
    initSeg = this->GetInitSegment();

    std::cout <<"initseg "<< initSeg<<" dashreceiver::downloadinitsegment...." << std::endl;

    if (initSeg)
    {
        std::cout << "DashReceiver::downloadinitseg::initSeg" << std::endl;
        initSeg->StartDownload(this->conn);
        this->initSegments[rep] = initSeg;
        initSeg->WaitFinished();
    }
}
bool                        DASHReceiver::InitSegmentExists      (IRepresentation* rep)
{
    std::cout << "dashreceiver::initsegmentexits" << std::endl;
    if (this->initSegments.find(rep) != this->initSegments.end())
        return true;

    return false;
}

/* Thread that does the buffering of segments */
void*                       DASHReceiver::DoBuffering               (void *receiver)
{
    std::cout << "dashreceiver::dobuffering" << std::endl;
    DASHReceiver *dashReceiver = (DASHReceiver *) receiver;

    dashReceiver->DownloadInitSegment(dashReceiver->GetRepresentation());

    MediaObject *media = dashReceiver->GetNextSegment();

    while(media != NULL && dashReceiver->isBuffering)
    {
        media->StartDownload(dashReceiver->conn);

        if (!dashReceiver->buffer->PushBack(media))
            return NULL;

        media->WaitFinished();

        dashReceiver->NotifySegmentDownloaded();

        media = dashReceiver->GetNextSegment();
    }

    dashReceiver->buffer->SetEOS(true);
    return NULL;
}
