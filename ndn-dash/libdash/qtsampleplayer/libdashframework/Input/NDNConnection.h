/*
 * NDNConnection.h
 *****************************************************************************
 * Header file for NDNx input module for libdash
 *
 * Author: Qian Li <qian.li@irt-systemx.fr>
 *
 * Copyright (C) 2016 IRT SystemX
 *
 * Based on NDNx input module for VLC by Jordan Augé
 * Portions Copyright (C) 2015 Cisco Systems
 * Based on CCNx input module for libdash by
 * Copyright (C) 2012, bitmovin Softwareentwicklung OG, All Rights Reserved
 * Based on the NDNx input module by UCLA
 * Portions Copyright (C) 2013 Regents of the University of California.
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 * This work is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation.
 *****************************************************************************/

#ifndef NDNCONNECTION_H_
#define NDNCONNECTION_H_

#include "../libdash/source/portable/Networking.h"
#include "IConnection.h"
#include "../../debug.h"
#include "log/log.h"

#include <string>
#include <stdint.h>
#include <iostream>
#include <sstream>
#include <chrono>
#include <inttypes.h>
#include <stdlib.h>
#include <stdarg.h>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/command-interest-generator.hpp>

#define DEFAULT_LIFETIME	   250
#define RETRY_TIMEOUT 		   25


typedef struct {
    volatile uint64_t i_pos;
//    volatile int eof;
} ndn_cxx_libdash_data_t;

typedef struct {
    uint8_t data[10000];
    uint64_t start_offset;
    uint64_t size;
} buffer_t;

namespace libdash {
    namespace framework {
        namespace input {
            class NDNConnection : public dash::network::IConnection {
            public:
                NDNConnection();

                ~NDNConnection();

                virtual void Init(dash::network::IChunk *chunk);

                virtual int Read(uint8_t *data, size_t len, dash::network::IChunk *chunk);

                virtual int Peek(uint8_t *data, size_t len, dash::network::IChunk *chunk);

                virtual double GetAverageDownloadingSpeed();

                virtual void doFetch();

                virtual void onData(const ndn::Interest &interest, const ndn::Data &data);

                virtual void onTimeout(const ndn::Interest &interest);

                /*
                 *  IDASHMetrics
                 *  Can't be deleted...
                 */
                const std::vector<dash::metrics::ITCPConnection *> &GetTCPConnectionList() const;

                const std::vector<dash::metrics::IHTTPTransaction *> &GetHTTPTransactionList() const;

            private:
                uint64_t i_chunksize;
                int i_lifetime;
                int i_missed_co;
                /**< number of content objects we missed in NDNBlock */
                buffer_t m_buffer;

                std::string m_name;
                ndn::Name m_recv_name;
                ndn::Face m_face;
                int m_first;
                bool m_isFinished;
                uint64_t m_nextSeg;
  //              int i_retrytimeout;
  //              int m_cid_timing_out;
  //              int m_timeout_counter;

                double  speed; // in bps
                uint64_t sizeDownloaded;
                std::chrono::time_point<std::chrono::system_clock> m_start_time;

                std::vector<dash::metrics::ITCPConnection *> tcpConnections;
                std::vector<dash::metrics::IHTTPTransaction *> httpTransactions;
                uint64_t cumulativeBytesReceived;
            };
        }
    }
}

#endif /* NDNCONNECTION_H_ */
