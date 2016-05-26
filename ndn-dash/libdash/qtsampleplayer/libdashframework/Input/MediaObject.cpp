/*
 * MediaObject.cpp
 *****************************************************************************
 * Copyright (C) 2012, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#include "MediaObject.h"
#include <inttypes.h>
#include<stdio.h>


using namespace libdash::framework::input;

using namespace dash::mpd;
using namespace dash::network;
using namespace dash::metrics;

MediaObject::MediaObject    (ISegment *segment, IRepresentation *rep, bool withFeedBack) :
             segment        (segment),
             rep            (rep),
			 withFeedBack	(withFeedBack)
{
    InitializeConditionVariable (&this->stateChanged);
    InitializeCriticalSection   (&this->stateLock);
}
MediaObject::~MediaObject   ()
{
    if(this->state == IN_PROGRESS)
    {
        this->segment->AbortDownload();
        this->OnDownloadStateChanged(ABORTED);
    }
    this->segment->DetachDownloadObserver(this);
    this->WaitFinished();

    DeleteConditionVariable (&this->stateChanged);
    DeleteCriticalSection   (&this->stateLock);
}

void 				MediaObject::SetFeedBack(bool flag)
{
	this->withFeedBack = flag;
}
bool                MediaObject::StartDownload          ()
{
  /*  NDNConnection* conn = new NDNConnection();
    if(!conn->Init(this->segment))
            std::cout <<"INIT ERROR\n";

    if(!conn->Schedule(this->segment))
        std::cout <<"Schedule\n";

    this->segment->AttachDownloadObserver(this);
    return this->segment->StartDownload(conn); */
	this->segment->AttachDownloadObserver(this);
	return this->segment->StartDownload();
}
bool                MediaObject::StartDownload          (NDNConnection* conn)
{
	L("Media_object:\tSTARTING DOWNLOAD %s\n", ((IChunk*)this->segment)->Path().c_str());
	if(conn == NULL)
		return this->StartDownload();

	conn->Init(this->segment);

    this->segment->AttachDownloadObserver(this);
    return this->segment->StartDownload(conn);
}
void                MediaObject::AbortDownload          ()
{
    this->segment->AbortDownload();
    L("Media_object:\tABORTED DOWNLOAD %s\n", ((IChunk*)this->segment)->Path().c_str());
    this->OnDownloadStateChanged(ABORTED);
}
void                MediaObject::WaitFinished           ()
{
    EnterCriticalSection(&this->stateLock);

    while(this->state != COMPLETED && this->state != ABORTED){
        SleepConditionVariableCS(&this->stateChanged, &this->stateLock, INFINITE);
    }

    LeaveCriticalSection(&this->stateLock);
    if(this->state != ABORTED)
    {
    	if(this->withFeedBack)
    	{
    		this->dashReceiver->Notifybps(this->bps);
    	}
    	L("Media_object:\tFINISHED DOWNLOAD %s\n", ((IChunk*)this->segment)->Path().c_str());
    }
}
int                 MediaObject::Read                   (uint8_t *data, size_t len)
{
    return this->segment->Read(data, len);
}
int                 MediaObject::Peek                   (uint8_t *data, size_t len)
{
    return this->segment->Peek(data, len);
}
int                 MediaObject::Peek                   (uint8_t *data, size_t len, size_t offset)
{
    return this->segment->Peek(data, len, offset);
}
IRepresentation*    MediaObject::GetRepresentation      ()
{
    return this->rep;
}
void                MediaObject::OnDownloadStateChanged (DownloadState state)
{
    EnterCriticalSection(&this->stateLock);

    this->state = state;

    WakeAllConditionVariable(&this->stateChanged);
    LeaveCriticalSection(&this->stateLock);
}
void                MediaObject::OnDownloadRateChanged  (uint64_t bitsPerSecond)
{
	this->bps = bitsPerSecond;
}
void				MediaObject::SetDASHReceiver(input::DASHReceiver *_dashReceiver)
{
	this->dashReceiver = _dashReceiver;
}
void				MediaObject::SetAdaptationLogic(adaptation::IAdaptationLogic *_adaptationLogic)
{
	this->adaptationLogic = _adaptationLogic;
}
const std::vector<ITCPConnection *>&    MediaObject::GetTCPConnectionList   () const
{
    return this->segment->GetTCPConnectionList();
}
const std::vector<IHTTPTransaction *>&  MediaObject::GetHTTPTransactionList () const
{
    return this->segment->GetHTTPTransactionList();
}
