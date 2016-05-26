/*
 * RateBasedAdaptation.cpp
 *****************************************************************************
 * Copyright (C) 2016, JS
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#include "RateBasedAdaptation.h"
#include<stdio.h>


using namespace dash::mpd;
using namespace libdash::framework::adaptation;
using namespace libdash::framework::input;
using namespace libdash::framework::mpd;

RateBasedAdaptation::RateBasedAdaptation          (IMPD *mpd, IPeriod *period, IAdaptationSet *adaptationSet, bool isVid) :
                  AbstractAdaptationLogic   (mpd, period, adaptationSet, isVid)
{
	std::vector<IRepresentation* > representations = this->adaptationSet->GetRepresentation();

	this->availableBitrates.clear();
	for(size_t i = 0; i < representations.size(); i++)
	{
		this->availableBitrates.push_back((uint64_t)(representations.at(i)->GetBandwidth()));
	}
	this->currentBitrate = this->availableBitrates.at(0);
	this->representation = representations.at(0);
	this->multimediaManager = NULL;
	this->alpha = 0.85;
	L("RateBasedParams:\t%f\n",alpha);
}
RateBasedAdaptation::~RateBasedAdaptation         ()
{
}

LogicType       RateBasedAdaptation::GetType               ()
{
    return adaptation::RateBased;
}

bool			RateBasedAdaptation::IsUserDependent		()
{
	return false;
}

bool			RateBasedAdaptation::IsRateBased		()
{
	return true;
}
bool			RateBasedAdaptation::IsBufferBased		()
{
	return false;
}

void			RateBasedAdaptation::SetMultimediaManager (sampleplayer::managers::IMultimediaManagerBase *_mmManager)
{
	this->multimediaManager = _mmManager;
}

void			RateBasedAdaptation::NotifyBitrateChange	()
{
	if(this->multimediaManager->IsStarted() && !this->multimediaManager->IsStopping())
		if(this->isVideo)
			this->multimediaManager->SetVideoQuality(this->period, this->adaptationSet, this->representation);
		else
			this->multimediaManager->SetAudioQuality(this->period, this->adaptationSet, this->representation);
}

uint64_t		RateBasedAdaptation::GetBitrate				()
{
	return this->currentBitrate;
}

void 			RateBasedAdaptation::SetBitrate				(uint64_t bps)
{
	std::vector<IRepresentation *> representations;
	representations = this->adaptationSet->GetRepresentation();
	size_t i = 0;
	for(i = 0;i < representations.size();i++)
	{
		if(representations.at(i)->GetBandwidth() > this->alpha * bps)
			break;
	}
	if((size_t)i == (size_t)(representations.size()))
		i = i-1;
	L("ADAPTATION_LOGIC:\tFor %s:\tBW_estimation: %lu, choice: %lu\n", (this->isVideo ? "video" : "audio"), bps, i);
	this->representation = representations.at(i);
	this->currentBitrate = this->representation->GetBandwidth();
}

void			RateBasedAdaptation::BitrateUpdate			(uint64_t bps)
{
	Debug("Rate Based adaptation: speed received: %lu\n", bps);
	this->SetBitrate(bps);
	this->NotifyBitrateChange();
}

void 			RateBasedAdaptation::OnEOS					(bool value)
{
}

void			RateBasedAdaptation::BufferUpdate			(uint32_t bufferfill)
{
}
