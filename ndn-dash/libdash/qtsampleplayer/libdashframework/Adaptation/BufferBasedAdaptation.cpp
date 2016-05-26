/*
 * BufferBasedAdaptation.cpp
 *****************************************************************************
 * Copyright (C) 2016, JS
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#include "BufferBasedAdaptation.h"
#include<stdio.h>


using namespace dash::mpd;
using namespace libdash::framework::adaptation;
using namespace libdash::framework::input;
using namespace libdash::framework::mpd;

BufferBasedAdaptation::BufferBasedAdaptation          (IMPD *mpd, IPeriod *period, IAdaptationSet *adaptationSet, bool isVid) :
                  AbstractAdaptationLogic   (mpd, period, adaptationSet, isVid)
{
	std::vector<IRepresentation* > representations = this->adaptationSet->GetRepresentation();
	this->reservoirThreshold = 25;
	this->maxThreshold = 80;
	this->representation = this->adaptationSet->GetRepresentation().at(0);
	this->multimediaManager = NULL;
	this->lastBufferFill = 0;
	this->bufferEOS = false;
	L("BufferBasedParams:\t%f\t%f\n", (double)reservoirThreshold/100, (double)maxThreshold/100);
	Debug("Buffer Adaptation:	STARTED\n");
}
BufferBasedAdaptation::~BufferBasedAdaptation         ()
{
}

LogicType       BufferBasedAdaptation::GetType               ()
{
    return adaptation::BufferBased;
}

bool			BufferBasedAdaptation::IsUserDependent		()
{
	return false;
}

bool			BufferBasedAdaptation::IsRateBased		()
{
	return false;
}
bool			BufferBasedAdaptation::IsBufferBased		()
{
	return true;
}

void			BufferBasedAdaptation::SetMultimediaManager (sampleplayer::managers::IMultimediaManagerBase *_mmManager)
{
	this->multimediaManager = _mmManager;
}

void			BufferBasedAdaptation::NotifyBitrateChange	()
{
	if(this->multimediaManager)
		if(this->multimediaManager->IsStarted() && !this->multimediaManager->IsStopping())
			if(this->isVideo)
				this->multimediaManager->SetVideoQuality(this->period, this->adaptationSet, this->representation);
			else
				this->multimediaManager->SetAudioQuality(this->period, this->adaptationSet, this->representation);
}

uint64_t		BufferBasedAdaptation::GetBitrate				()
{
	return this->currentBitrate;
}

void 			BufferBasedAdaptation::SetBitrate				(uint32_t bufferFill)
{
	std::vector<IRepresentation *> representations;
	representations = this->adaptationSet->GetRepresentation();
	size_t i = 0;

	while(bufferFill > this->reservoirThreshold + i * (this->maxThreshold - this->reservoirThreshold)/representations.size())
	{
		i++;
	}
	if((size_t)i >= (size_t)(representations.size()))
		i = representations.size() - 1;
//	if(!this->bufferEOS && this->lastBufferFill > this->reservoirThreshold && bufferFill <= this->reservoirThreshold)
//	{
//		this->multimediaManager->ShouldAbort(this->isVideo);
//	}
	L("ADAPTATION_LOGIC:\tFor %s:\tbuffer_level: %f, choice: %lu\n",isVideo ? "video" : "audio", (double)bufferFill/100, i);
	this->representation = representations.at(i);
	this->lastBufferFill = bufferFill;
}

void			BufferBasedAdaptation::BitrateUpdate		(uint64_t bps)
{
}

void			BufferBasedAdaptation::OnEOS				(bool value)
{
//	this->bufferEOS = value;
}
void			BufferBasedAdaptation::BufferUpdate			(uint32_t bufferFill)
{
	this->SetBitrate(bufferFill);
	this->NotifyBitrateChange();
}
