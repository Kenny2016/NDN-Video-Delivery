#ifndef QTSAMPLEPLAYER_MANAGERS_IMULTIMEDIAMANAGERBASE_H_
#define QTSAMPLEPLAYER_MANAGERS_IMULTIMEDIAMANAGERBASE_H_

#include "IMPD.h"

namespace sampleplayer
{
    namespace managers
    {
    	class IMultimediaManagerBase
    	{
    		public:
    		virtual bool SetVideoQuality      (dash::mpd::IPeriod* period, dash::mpd::IAdaptationSet *adaptationSet, dash::mpd::IRepresentation *representation) = 0;
    		virtual bool SetAudioQuality      (dash::mpd::IPeriod* period, dash::mpd::IAdaptationSet *adaptationSet, dash::mpd::IRepresentation *representation) = 0;
    		virtual bool IsStarted			  ()=0;
    		virtual bool IsStopping			  ()=0;
    		virtual void ShouldAbort		  (bool isVideo)=0;
    	};
    }
}

#endif //QTSAMPLEPLAYER_MANAGERS_IMULTIMEDIAMANAGERBASE_H_
