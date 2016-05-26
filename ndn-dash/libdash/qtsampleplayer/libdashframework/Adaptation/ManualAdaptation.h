/*
 * ManualAdaptation.h
 *****************************************************************************
 * Copyright (C) 2013, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#ifndef LIBDASH_FRAMEWORK_ADAPTATION_MANUALADAPTATION_H_
#define LIBDASH_FRAMEWORK_ADAPTATION_MANUALADAPTATION_H_

#include "AbstractAdaptationLogic.h"
#include "../MPD/AdaptationSetStream.h"

namespace libdash
{
    namespace framework
    {
        namespace adaptation
        {
            class ManualAdaptation : public AbstractAdaptationLogic
            {
                public:
                    ManualAdaptation            (dash::mpd::IMPD *mpd, dash::mpd::IPeriod *period, dash::mpd::IAdaptationSet *adaptationSet, bool isVid);
                    virtual ~ManualAdaptation   ();

                    virtual LogicType               GetType             ();
                    virtual bool					IsUserDependent		();
                    virtual bool					IsRateBased			();
                    virtual bool					IsBufferBased		();
                    virtual void 					BitrateUpdate		(uint64_t bps);
                    virtual void 					BufferUpdate		(uint32_t bufferfill);
                    virtual void 					SetMultimediaManager(sampleplayer::managers::IMultimediaManagerBase *mmM);
                    virtual void					OnEOS				(bool value);
            };
        }
    }
}

#endif /* LIBDASH_FRAMEWORK_ADAPTATION_MANUALADAPTATION_H_ */
