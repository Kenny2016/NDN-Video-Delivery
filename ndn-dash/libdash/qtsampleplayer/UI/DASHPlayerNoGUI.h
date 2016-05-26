/*
 * DASHPlayerNoGUI.h
 *****************************************************************************
 * Copyright (C) 2012, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#ifndef DASHPLAYERNOGUI_H_
#define DASHPLAYERNOGUI_H_

#include <iostream>
#include <sstream>
#include "libdash.h"
#include "IDASHPlayerNoGuiObserver.h"
#include "../Managers/IMultimediaManagerObserver.h"
#include "../Renderer/QTGLRenderer.h"
#include "../Renderer/QTAudioRenderer.h"
#include "../Managers/MultimediaManager.h"
#include "../libdashframework/Adaptation/AlwaysLowestLogic.h"
#include "../libdashframework/Adaptation/IAdaptationLogic.h"
#include "../libdashframework/Adaptation/ManualAdaptation.h"
#include "../libdashframework/Buffer/IBufferObserver.h"
#include "../libdashframework/MPD/AdaptationSetHelper.h"

namespace sampleplayer
{
    struct settings_t
    {
        int period;
        int videoAdaptationSet;
        int audioAdaptationSet;
        int videoRepresentation;
        int audioRepresentation;
    };

    class DASHPlayerNoGUI : public IDASHPlayerNoGuiObserver, public managers::IMultimediaManagerObserver

    {
        public:
            DASHPlayerNoGUI          (int argc, char** argv, pthread_cond_t *mainCond);
            virtual ~DASHPlayerNoGUI ();

            void parseArgs(int argc, char ** argv);
            void helpMessage(char *name);
            virtual void OnStartButtonPressed   (int period, int videoAdaptationSet, int videoRepresentation, int audioAdaptationSet, int audioRepresentation);
            virtual void OnStopButtonPressed    ();

            virtual void OnSettingsChanged		(int period, int videoAdaptationSet, int videoRepresentation, int audioAdaptationSet, int audioRepresentation);

            /* IMultimediaManagerObserver */
            virtual void OnVideoBufferStateChanged          (uint32_t fillstateInPercent);
            virtual void OnVideoSegmentBufferStateChanged   (uint32_t fillstateInPercent);
            virtual void OnAudioBufferStateChanged          (uint32_t fillstateInPercent);
            virtual void OnAudioSegmentBufferStateChanged   (uint32_t fillstateInPercent);
            virtual void OnEOS								();

            virtual bool OnDownloadMPDPressed   (const std::string &url);

            bool IsRunning						();

        private:
            dash::mpd::IMPD                             *mpd;
            sampleplayer::renderer::QTGLRenderer        *videoElement;
            sampleplayer::renderer::QTAudioRenderer     *audioElement;
            sampleplayer::managers::MultimediaManager   *multimediaManager;
            settings_t                                  currentSettings;
            CRITICAL_SECTION                            monitorMutex;
            char										*url;
            bool										isNDN;
            libdash::framework::adaptation::LogicType 	adaptLogic;
            pthread_cond_t								*mainCond;
            bool										isRunning;

            bool    SettingsChanged (int period, int videoAdaptationSet, int videoRepresentation, int audioAdaptationSet, int audioRepresentation);
            void    SetSettings     (int period, int videoAdaptationSet, int videoRepresentation, int audioAdaptationSet, int audioRepresentation);
    };
}
#endif /* DASHPLAYER_H_ */
