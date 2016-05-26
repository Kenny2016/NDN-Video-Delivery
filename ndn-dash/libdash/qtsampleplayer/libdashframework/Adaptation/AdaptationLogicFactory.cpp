/*
 * AdaptationLogicFactory.cpp
 *****************************************************************************
 * Copyright (C) 2013, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#include "AdaptationLogicFactory.h"
#include<stdio.h>

using namespace libdash::framework::adaptation;
using namespace dash::mpd;

IAdaptationLogic* AdaptationLogicFactory::Create(LogicType logic, IMPD *mpd, IPeriod *period, IAdaptationSet *adaptationSet,bool isVid)
{
	Debug("Adaptation Logic for %s: ", isVid ? "video" : "audio");
    switch(logic)
    {
        case Manual:
        	{
        		Debug("Manual\n");
        		return new ManualAdaptation     (mpd, period, adaptationSet, isVid);
        	}
        case AlwaysLowest:
        	{
        		Debug("Always lowest\n");
        		return new AlwaysLowestLogic    (mpd, period, adaptationSet, isVid);
        	}
        case RateBased:
        	{
        		Debug("Rate based\n");
        		return new RateBasedAdaptation	(mpd,period,adaptationSet, isVid);
        	}
        case BufferBased:
        	{
        		Debug("Buffer based\n");
        		return new BufferBasedAdaptation (mpd,period,adaptationSet, isVid);
        	}
        default:
        	{
        		Debug("default => return Manual\n");
        		return new ManualAdaptation  (mpd, period, adaptationSet, isVid);
        	}
    }
}
