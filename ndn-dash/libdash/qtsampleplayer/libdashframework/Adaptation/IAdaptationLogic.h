/*
 * IAdaptationLogic.h
 *****************************************************************************
 * Copyright (C) 2013, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#ifndef LIBDASH_FRAMEWORK_ADAPTATION_IADAPTATIONLOGIC_H_
#define LIBDASH_FRAMEWORK_ADAPTATION_IADAPTATIONLOGIC_H_


#include "../Input/MediaObject.h"
#include "../Input/DASHReceiver.h"
#include "IRepresentation.h"
#include "../../Managers/IMultimediaManagerBase.h"
#include "log/log.h"

namespace libdash
{
    namespace framework
    {
    	namespace input
		{
    		class DASHReceiver;
		}
        namespace adaptation
        {
//#define START __LINE__
			#define FOREACH_ADAPTATIONLOGIC(ADAPTATIONLOGIC) \
			ADAPTATIONLOGIC(Manual)   \
			ADAPTATIONLOGIC(AlwaysLowest)  \
			ADAPTATIONLOGIC(RateBased)  \
			ADAPTATIONLOGIC(BufferBased)  \
			ADAPTATIONLOGIC(Count)  \

//			#define NUMBER_OF_LOGICS __LINE__-START-1

			#define GENERATE_ENUM(ENUM) ENUM,
			#define GENERATE_STRING(STRING) #STRING,

        	enum LogicType {
        		FOREACH_ADAPTATIONLOGIC(GENERATE_ENUM)
        	};

        	static const char *LogicType_string[] = {
        		FOREACH_ADAPTATIONLOGIC(GENERATE_STRING)
        	};
       //    enum LogicType
        //    {
        //        Manual,
         //       AlwaysLowest
        //    };

            class IAdaptationLogic
            {
                public:
                    virtual ~IAdaptationLogic () {}

                    virtual uint32_t                    GetPosition         ()                                              = 0;
                    virtual void                        SetPosition         (uint32_t segmentNumber)                        = 0;
                    virtual dash::mpd::IRepresentation* GetRepresentation   ()                                              = 0;
                    virtual void                        SetRepresentation   (dash::mpd::IPeriod *period,
                                                                             dash::mpd::IAdaptationSet *adaptationSet,
                                                                             dash::mpd::IRepresentation *representation)    = 0;
                    virtual LogicType                   GetType             ()                                              = 0;
                    virtual bool						IsUserDependent		()												= 0;
                    virtual void						BitrateUpdate		(uint64_t bps)									= 0;
                    virtual void						BufferUpdate		(uint32_t bufferfillstate)									= 0;
                    virtual bool 						IsRateBased			()												= 0;
                    virtual bool 						IsBufferBased		()												= 0;
                    virtual void						SetMultimediaManager		(sampleplayer::managers::IMultimediaManagerBase *mmManager)				= 0;
                    virtual void						OnEOS				(bool value) 									= 0;
            };
        }
    }
}
#endif /* LIBDASH_FRAMEWORK_ADAPTATION_IADAPTATIONLOGIC_H_ */
