/*
 * CCNConnection.cpp
 *****************************************************************************
 * Copyright (C) 2012, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "NDNConnection.h"
#include <inttypes.h>
#include <time.h>
#include <limits.h>
#include <poll.h>
#include <errno.h>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/command-interest-generator.hpp>

#include <boost/exception/diagnostic_information.hpp>

//logging purpose
#include <chrono>
#include <stdarg.h>

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/

using namespace libdash::framework::input;

using namespace dash;
using namespace dash::network;
using namespace dash::metrics;

using std::bind;
using std::placeholders::_1;
using std::placeholders::_2;

using duration_in_seconds = std::chrono::duration<double, std::ratio<1,1> >;

//#define LOGBUFSIZE 1000
//char logbuf[LOGBUFSIZE];

#define NOW() do { double now = std::chrono::duration_cast<duration_in_seconds>(std::chrono::system_clock::now() - m_start_time).count(); } while(0)

/*****************************************************************************
 * Module local data structure
 *****************************************************************************/

// Isolate the C++ plugin data in a separate class

NDNConnection::NDNConnection()
  : m_recv_name(std::string())
  , m_first(1)
  , m_live(false)
  , m_cid_timing_out(0)	//chunk id
  , m_timeout_counter(0),

  peekBufferLen   (0),
  //contentLength   (0),
  isInit          (false),
  isScheduled     (false),
  isConn          (false),
  //i_name              (NULL),
  //p_name              (NULL),
  //interest_template   (NULL),
  //ccn_get_timeout     (CCN_DEFAULT_TIMEOUT),
  b_last              (false)
  //ndnConn             (NULL)
//

{
  //std::cout<<"here"<<std::endl;
  i_chunksize       = -1;
  i_missed_co       = 0;
  //std::cout<<"here"<<std::endl;
  i_lifetime = DEFAULT_LIFETIME;
  i_retrytimeout = RETRY_TIMEOUT;
  //std::cout<<"here"<<std::endl;
//
  this->peekBuffer = new uint8_t[PEEKBUFFER];
  //std::cout<<"here"<<std::endl;
  std::cout << "NDNConnection::create NDN connection"<<std::endl;
//
}


NDNConnection::~NDNConnection()
{
    delete[] this->peekBuffer;
    //if(this->i_name != NULL)
        //ccn_charbuf_destroy(&this->i_name);

    //if(this->p_name != NULL)
        //ccn_charbuf_destroy(&this->p_name);

    //if(this->interest_template != NULL)
        //ccn_charbuf_destroy(&this->interest_template);

    //if(this->cf != NULL)
        //ccn_fetch_destroy(cf);

    //if(ccnConn != NULL)
    //{
        //ccn_disconnect(ccnConn);
        //ccn_destroy(&ccnConn);
    //}
}

/*
void NDNConnection::L(std::string event, uint64_t id)
{
 double now = std::chrono::duration_cast<duration_in_seconds>(std::chrono::system_clock::now() - m_start_time).count();
  snprintf(logbuf, LOGBUFSIZE, "NDNLOG %f %s %" PRIu64, now, event.c_str(), id);
  m_data->p_log(m_data, (char*)logbuf);
}
*/

bool NDNConnection::Init            (IChunk *chunk)
//
{
    m_first = 1;
    m_timeout_counter = 0;
    m_cid_timing_out = 0;
    m_buffer.size = 0;
    //m_data = malloc(sizeof(ndn_cxx_libdash_data_t));
    std::cout << "init connection" << std::endl;
    //std::cout<<" host:"<<chunk->Host()<<" type:" <<chunk->GetType()<<" startbyte:" <<chunk->StartByte()<<" Range:" <<chunk->Range()<<std::endl;

    //std::cout<<chunk->Host()<<std::endl;
    //std::cout<<chunk->Path()<<std::endl;
    //std::string name = std::string("ndn:/" + chunk->Host() + chunk->Path());
    //std::cout<<name.c_str();
    m_name = "ndn:/" + chunk->Host() + chunk->Path();
    //std::cout<<"m_name "<<m_name<<std::endl;
    //m_data->chunk = chunk;
    //std::cout<<"m_data.chunk "<<m_data->chunk<<std::endl;
    m_data.eof = 0;
    //std::cout<<"m_name.eof "<<m_data.eof<<std::endl;
    m_data.i_pos = 0;
    //std::cout<<"m_name.i_pos "<<m_data.i_pos<<std::endl;
    //m_data.i_size = 0;
    //std::cout<<"m_name.i_size "<<m_data.i_size<<std::endl;

    b_last = false;

    if( this->isConn ){
        std::cout << "already connected" << std::endl;
           return true;
    }
/*
    if( (this->ccnConn = ccn_create()) == NULL )
    {
        std::cout <<"could not get ccn handle with ccn_create()" << std::endl;
        return false;
    }
    if( ccn_connect(this->ccnConn, NULL) == -1)
    {
        std::cout << "could not get connection to local ccn with ccn_connect()" << std::endl;
        return false;
    }
    if( (this->cf = ccn_fetch_new(this->ccnConn)) == NULL){
        std::cout << "fetch new error" << std::endl;
        return false;
    }
*/
    this->isInit = true;
    this->isConn = true;
    return true;
}


bool    NDNConnection::Schedule    (dash::network::IChunk *chunk)
{
    //std::cout <<"Start Schedule, check conn" << std::endl;
    if(!this->isInit)
        return false;

    std::cout <<"Start Schedule, check schedule" << std::endl;
    if(this->isScheduled)
        return false;

    //std::cout <<"Start Expression of Interest: " << m_name << std::endl;

    this->isScheduled = true;

    doFetch();

    try
    {
        m_face.processEvents();
    }
    catch (...)
    {
        std::cerr << "Unexpected exception during the fetching, diagnostic informations:\n" <<
        boost::current_exception_diagnostic_information();

        return false;
        //return NULL;
    }
    //std::cout<<"finish dofetch"<<std::endl;
    return true;
}


int             NDNConnection::Peek            (uint8_t *data, size_t len, IChunk *chunk)
{
    return -1;
}


void NDNConnection::doFetch()
{
  double now = std::chrono::duration_cast<duration_in_seconds>(std::chrono::system_clock::now() - m_start_time).count();

  ndn::Name name(m_name);
    //std::cout<<name<<std::endl;
  if(m_live && m_timeout_counter > i_retrytimeout)
  {
    m_first = 1;
    m_cid_timing_out = 0;
    m_timeout_counter = 0;
  }

  if (m_first == 0) {
    name.append(m_recv_name[-2]).appendSegment(m_data.i_pos / i_chunksize);
      //std::cout<<"fetch interest: "<<m_data.i_pos / i_chunksize<<std::endl;
    //L("fetch_interest", m_data->i_pos / i_chunksize);
  } else {
      //std::cout<<"fetch first interest: "<<m_data.i_pos / i_chunksize<<std::endl;
    //L("fetch_interest", m_data->i_pos / i_chunksize);
  }
    //std::cout<<"Interest "<<name<<std::endl;
  ndn::Interest interest(name, ndn::time::milliseconds(i_lifetime));

  if (m_live && (m_first == 1)) {
    interest.setMustBeFresh(true);
#ifdef FRESH
  } else {
    interest.setMustBeFresh(true);
#endif // FRESH
  }


//#ifdef ANDROID
//  m_data->p_log_64(m_data, (char*)"F: asking for", m_data->i_pos / i_chunksize);
//#else
  //printf("F: asking_for: %" PRIu64 "\n", m_data->i_pos / i_chunksize);
//#endif // ANDROID
  m_last_fetch = std::chrono::system_clock::now();
    //std::cout<<"Interest "<<interest<<std::endl;
    //std::cout<<"Interest getname "<<interest.getName()<<std::endl;
  m_face.expressInterest(interest, bind(&NDNConnection::onData, this, _1, _2), bind(&NDNConnection::onTimeout, this, _1));

}


/*****************************************************************************
 * CALLBACKS
 *****************************************************************************/

void NDNConnection::onData(const ndn::Interest& interest, const ndn::Data& data)
{
    //std::cout<<"onData...... "<<std::endl;
  if (m_data.eof == 1)
    return;

  //L("fetch_data", data.getName()[-1].toSegment());

  double now = std::chrono::duration_cast<duration_in_seconds>(std::chrono::system_clock::now() - m_start_time).count();

  if (m_first == 1) {
      //std::cout<<"first segment "<<std::endl;
      m_recv_name = data.getName();
      //std::cout<<"m_recv_name "<<m_recv_name<<std::endl;
      i_chunksize = data.getContent().value_size();
      //std::cout<<"i_chunksize "<<i_chunksize<<std::endl;

      //For live video purpose ISSUE WITH FRESHNESS FOR VOD IN THE CASE OF MULTICAST

      m_data.i_pos = data.getName()[-1].toSegment() * i_chunksize;

      //std::cout<<"m_data.i_pos "<<m_data.i_pos<<std::endl;
      m_live = bool(m_data.i_pos > 0);

      m_first = 0;
      m_cid_timing_out = 0;
      m_timeout_counter = 0;

      //Way around if the problem is not fixed?
      if(data.getName().toUri().find("live") != std::string::npos)
      {
        m_data.i_pos = data.getName()[-1].toSegment() * i_chunksize;
        m_live = true;
          //doFetch();
      }
      else
      { std::cout<<"not live "<<std::endl;
 	    m_data.i_pos = 0;
        m_live = 0;
          //doFetch();
        return;
      }


//#ifndef ANDROID
//      printf("LIVE OFFSET=%" PRIu64 "\n", m_data->i_pos);
//#endif
    //L("live_offset", m_data->i_pos);

  }

  //m_data->p_log_64(m_data, "F! data_received", data.getName()[-1].toSegment());

    //std::cout<<"second segment "<<std::endl;
    m_recv_name = data.getName();
    //std::cout<<"m_recv_name "<<m_recv_name<<std::endl;
  const ndn::Block& content = data.getContent();
    //std::cout<<"data content: "<<std::endl<<content.value()<<std::endl;

  memcpy(m_buffer.data, reinterpret_cast<const char*>(content.value()), content.value_size());
  m_buffer.size = content.value_size();
    //std::cout<<"buffer content: "<<std::endl<<m_buffer.data<<std::endl;
  m_buffer.start_offset = 0;
    m_data.i_pos += (m_buffer.size - m_buffer.start_offset);
    if(m_buffer.size < i_chunksize) {
        m_data.eof = 1;
        b_last = true;
        std::cout<<"LAST"<<std::endl;
        return;
    }
    else {
        //doFetch();
    }
    //std::cout<<"memcpy exit"<<std::endl;
    //exit(1);
}


  void NDNConnection::onTimeout(const ndn::Interest& interest)
{
    //std::cout<<"onTimeout of Interest: "<< interest << std::endl;
  double now = std::chrono::duration_cast<duration_in_seconds>(std::chrono::system_clock::now() - m_start_time).count();
//#ifdef ANDROID
//  m_data->p_log_64(m_data, (char*)"FETCH TIMEOUT", interest.getName()[-1].toSegment());
//#else
//  printf("F: FETCH TIMEOUT %" PRIu64 "\n", interest.getName()[-1].toSegment());
//#endif // ANDROID
  //L("fetch_timeout", (m_first) ? 0 : interest.getName()[-1].toSegment());
  m_buffer.size = 0;
  m_buffer.start_offset = 0;

  if(!m_first && interest.getName()[-1].toSegment() == m_cid_timing_out) {
      //std::cout << "Segment number: " << interest.getName()[-1]<< " cid: " << m_cid_timing_out << std::endl;
      //std::cout<<"if timeout"<<std::endl;
      //m_timeout_counter = (m_timeout_counter + 1) % (i_retrytimeout + 2);
      m_timeout_counter += 1;
      if(m_timeout_counter > NDN_MAX_TRYS)
      {
          b_last = true;
      }
      //std::cout << "timeout_counter: " << m_timeout_counter<<std::endl;
  }
  else
  {
      //std::cout<<"else timeout"<<std::endl;
      m_cid_timing_out = m_first ? 0 : interest.getName()[-1].toSegment();
      m_timeout_counter = 1;
      if(m_timeout_counter > NDN_MAX_TRYS)
      {
          b_last = true;
      }
      //std::cout << "timeout_counter: " << m_timeout_counter<<std::endl;
  }
}


/*****************************************************************************/
/*
namespace {
  std::function<void(int)> callback;
  extern "C" void wrapper(int i) {
    callback(i);
  }
}
*/


/*****************************************************************************
 * Read NDNBlock:
 *****************************************************************************/
/*
int NDNConnection::Read(uint8_t *data, size_t len, IChunk *chunk)
{
  //const unsigned char *data = NULL;
  uint64_t start_offset = 0;
  //uint64_t _i_size;
  //int i_ret;
  bool b_last = false;
  double delay;

  data = m_buffer.data;/////////

  doFetch();

  try
  {
    m_face.processEvents();
  }
  catch (...)
  {
    std::cerr << "Unexpected exception during the fetching, diagnostic informations:\n" <<
                  boost::current_exception_diagnostic_information();

    return -1;
    //return NULL;
  }

  // This will set both buf and data_size
  if (m_buffer.size <= 0) {
    // TODO document what VLC is doing after a missing chunk
#ifdef ANDROID
    //LOG("NDNBlock unable to retrieve requested content: retrying");
#else
    //printf("F: received with delay 1\n");
#endif // ANDROID
    i_missed_co++;
    return -1;
    //return NULL;
  }

  delay = std::chrono::duration_cast<duration_in_seconds>(std::chrono::system_clock::now() - m_last_fetch).count();

  bool isLast = false; // hardcoded
  if (isLast || m_buffer.size < i_chunksize)
    b_last = true;

  // Determine what offset is needed by VLC, necesarily aligned on chunk
  // size
  start_offset = m_data.i_pos % i_chunksize;

  if (start_offset > m_buffer.size) {
    //LOG("NDNBlock start_offset > data_size");
    //LOG("NDNBlock start_offset %" PRId64 " > data_size %zu", start_offset, m_buffer.size);
    return -1;
    //return NULL;
  } else {
    m_buffer.start_offset = start_offset;
  }
  m_data.i_pos += (m_buffer.size - start_offset);

  if (b_last) {
    m_data.i_size = m_data.i_pos;
    //LOG("LAST\n");
    //printf("LAST chunk\n");
    m_data.eof = 1;
    //LOG("m_data.eof=1");
  }

  // return a pointer towards the buffer
  //return &m_buffer;
  return m_buffer.size;
}
*/

int NDNConnection::Read(uint8_t *data, size_t len, IChunk *chunk)
{
    //std::cout<<"invoke read"<<std::endl;
    uint64_t start_offset = 0;
    double delay;

    while(true)
    {
        if (b_last)
            return 0;

        doFetch();

        try
        {
            m_face.processEvents();
        }
        catch (...)
        {
            std::cerr << "Unexpected exception during the fetching, diagnostic informations:\n" <<
            boost::current_exception_diagnostic_information();

            return 0;
            //return NULL;
        }

        // This will set both buf and data_size
        //std::cout<<m_buffer.size<<std::endl;
        if (m_buffer.size <= 0) {
            // TODO document what libdash is doing after a missing chunk
#ifdef ANDROID
            //LOG("NDNBlock unable to retrieve requested content: retrying");
#else
            //printf("F: received with delay 1\n");
#endif // ANDROID
            i_missed_co++;
            continue;
            //return 0;
            //return NULL;
        }

        delay = std::chrono::duration_cast<duration_in_seconds>(std::chrono::system_clock::now() - m_last_fetch).count();

        //rcfr = ccn_fetch_read(fs, data, len/*CCN_CHUNK_SIZE*/);
        memcpy(data, m_buffer.data, m_buffer.size);

        //std::cout<<"read content: "<<std::endl<<data<<std::endl;

        if(m_buffer.size > 0)
        {
            return m_buffer.size;
        }
    }
    //return rcfr;
}


/*
extern "C" {
  buffer_t* ndn_cxx_libdash_block(ndn_cxx_vlc_data_t * data)
  {
    NDNConnection * conn = (NDNConnection*)(data->conn);

    return conn->NDNBlock();
  }
}
*/

const std::vector<ITCPConnection *>&        NDNConnection::GetTCPConnectionList    () const
{
    return tcpConnections;
}


const std::vector<IHTTPTransaction *>&      NDNConnection::GetHTTPTransactionList  () const
{
    return httpTransactions;
}
