/*
 * ManualAdaptation.cpp
 *****************************************************************************
 * Copyright (C) 2013, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#include "ManualAdaptation.h"

using namespace dash::mpd;
using namespace libdash::framework::adaptation;
using namespace libdash::framework::input;
using namespace libdash::framework::mpd;

ManualAdaptation::ManualAdaptation          (IMPD *mpd, IPeriod *period, IAdaptationSet *adaptationSet, bool isVid) :
                  AbstractAdaptationLogic   (mpd, period, adaptationSet, isVid)
{
}
ManualAdaptation::~ManualAdaptation         ()
{
}

LogicType       ManualAdaptation::GetType               ()
{
    return adaptation::Manual;
}

bool			ManualAdaptation::IsUserDependent		()
{
	return true;
}

bool			ManualAdaptation::IsRateBased		()
{
	return false;
}
bool			ManualAdaptation::IsBufferBased		()
{
	return false;
}

void 			ManualAdaptation::BitrateUpdate			(uint64_t bps)
{
}
void 			ManualAdaptation::BufferUpdate			(uint32_t bufferfill)
{
}

void			ManualAdaptation::OnEOS					(bool value)
{
}

void 			ManualAdaptation::SetMultimediaManager	(sampleplayer::managers::IMultimediaManagerBase *mmM)
{
}
