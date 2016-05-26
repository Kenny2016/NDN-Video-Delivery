/*
 * AlwaysLowestLogic.cpp
 *****************************************************************************
 * Copyright (C) 2012, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#include "AlwaysLowestLogic.h"
#include<stdio.h>

using namespace libdash::framework::adaptation;
using namespace libdash::framework::input;
using namespace dash::mpd;

AlwaysLowestLogic::AlwaysLowestLogic        (IMPD *mpd, IPeriod *period, IAdaptationSet *adaptationSet, bool isVid) :
                   AbstractAdaptationLogic  (mpd, period, adaptationSet, isVid)
{
    this->representation = this->adaptationSet->GetRepresentation().at(0);
}
AlwaysLowestLogic::~AlwaysLowestLogic   ()
{
}

LogicType       AlwaysLowestLogic::GetType      ()
{
    return adaptation::AlwaysLowest;
}

bool			AlwaysLowestLogic::IsUserDependent()
{
	return false;
}

bool			AlwaysLowestLogic::IsRateBased()
{
	return false;
}
bool			AlwaysLowestLogic::IsBufferBased()
{
	return false;
}
void			AlwaysLowestLogic::BitrateUpdate(uint64_t bps)
{
}
void			AlwaysLowestLogic::BufferUpdate(uint32_t bufferfill)
{
}

void 			AlwaysLowestLogic::SetMultimediaManager(sampleplayer::managers::IMultimediaManagerBase *mmM)
{
}

void			AlwaysLowestLogic::OnEOS(bool value)
{
}
