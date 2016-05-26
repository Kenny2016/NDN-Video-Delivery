/*
 * RateBasedAdaptation.h
 *****************************************************************************
 * Copyright (C) 2016, JS
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#ifndef LIBDASH_FRAMEWORK_ADAPTATION_RATEBASEDADAPTATION_H_
#define LIBDASH_FRAMEWORK_ADAPTATION_RATEBASEDADAPTATION_H_

#include "AbstractAdaptationLogic.h"
#include "../MPD/AdaptationSetStream.h"
#include "../Input/IDASHReceiverObserver.h"

namespace libdash
{
    namespace framework
    {
        namespace adaptation
        {
            class RateBasedAdaptation : public AbstractAdaptationLogic
            {
                public:
                    RateBasedAdaptation            (dash::mpd::IMPD *mpd, dash::mpd::IPeriod *period, dash::mpd::IAdaptationSet *adaptationSet, bool isVid);
                    virtual ~RateBasedAdaptation   ();

                    virtual LogicType               GetType             ();
                    virtual bool					IsUserDependent		();
                    virtual bool 					IsRateBased			();
                    virtual bool 					IsBufferBased		();
                    virtual void 					BitrateUpdate		(uint64_t bps);
                    virtual void 					BufferUpdate		(uint32_t bufferfill);
                    void 							SetBitrate			(uint64_t bps);
                    uint64_t						GetBitrate			();
                    virtual void 					SetMultimediaManager	(sampleplayer::managers::IMultimediaManagerBase *_mmManager);
                    void							NotifyBitrateChange	();
                    void							OnEOS				(bool value);
                private:
                    uint64_t						currentBitrate;
                    std::vector<uint64_t>			availableBitrates;
                    sampleplayer::managers::IMultimediaManagerBase	*multimediaManager;
                    dash::mpd::IRepresentation		*representation;
                    double							alpha;
            };
        }
    }
}

#endif /* LIBDASH_FRAMEWORK_ADAPTATION_RATEBASEDADAPTATION_H_ */
