/*  Sirikata
 *  asyncConnectionGet.cpp
 *
 *  Copyright (c) 2010, Behram Mistree
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "asyncConnectionGet.hpp"
#include <iostream>
#include <map>
#include <utility>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>
#include <sirikata/core/network/Asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/regex.hpp>

namespace Sirikata
{
//constructor
  AsyncConnectionGet::AsyncConnectionGet(SpaceContext* con, Network::IOStrand* str, Network::IOStrand* error_strand, Network::IOStrand* result_strand, AsyncCraqScheduler* master, CraqObjectSegmentation* oseg, const std::tr1::function<void()>&readyStateChangedCallback)
  : ctx(con),
    mStrand(str),
    mPostErrorsStrand(error_strand),
    mResultStrand(result_strand),
    mSchedulerMaster (master),
    mOSeg(oseg),
    mReceivedStopRequest(false),
    mReadyStateChangedCallback(readyStateChangedCallback)
{
  mReady = NEED_NEW_SOCKET; //starts in the state that it's requesting a new socket.  Presumably asyncCraq reads that we need a new socket, and directly calls "initialize" on this class

  getTime = 0;
  numGets = 0;
}


  void AsyncConnectionGet::stop()
  {
    mStrand->post(std::tr1::bind(&AsyncConnectionGet::clear_all_deadline_timers,this));

    mReceivedStopRequest = true;
    if (mSocket != NULL)
      mSocket->cancel();

  }


  void AsyncConnectionGet::clear_all_deadline_timers()
  {
    MultiOutstandingQueries::iterator it;

    for(it = allOutstandingQueries.begin(); it != allOutstandingQueries.end(); ++it)
    {
      if(it->second->deadline_timer != NULL)
      {
        it->second->deadline_timer->cancel();
        delete it->second->deadline_timer;
        it->second->deadline_timer = NULL;

        if (it->second->traceToken != NULL)
        {
          delete it->second->traceToken;
          it->second->traceToken = NULL;
        }

      }
    }
  }



  int AsyncConnectionGet::numStillProcessing()
  {
    return (int) (allOutstandingQueries.size());
  }


  void AsyncConnectionGet::outputLargeOutstanding()
  {
      //don't do anything in this function any longer
      return;

    MultiOutstandingQueries::iterator it;
    Duration dur = Time::local() - Time::epoch();
    uint64 currentTime = dur.toMilliseconds();


    for (it = allOutstandingQueries.begin(); it != allOutstandingQueries.end(); ++it)
    {
      double timeInQueue = ((double) currentTime) - ((double) it->second->time_admitted);

      if (timeInQueue > 50)
      {
        std::cout<<"\nConnection Get Long Time Waiting:   "<<timeInQueue<<"    "<<it->second->currentlySearchingFor << "\n";
        if (it->second->gs == IndividualQueryData::GET)
          std::cout<<"GET operation\n";
        else
          std::cout<<"SET operation\n";

        if (it->second->is_tracking)
          std::cout<<"TRACKING\n";
        else
          std::cout<<"NOT tracking  \n";

        std::cout<<"Time admitted:    "<< it->second->time_admitted <<"\n";
        std::cout<<"Tracking number:  "<<it->second->tracking_number<<"\n\n\n";

      }
    }
  }


//This gets called with a pointer to query data as its argument.  The query data inside of this has waited too long to be processed.
//Therefore, we must return an error as our operation result and remove the query from our list of outstanding queries.
void AsyncConnectionGet::queryTimedOutCallbackGet(const boost::system::error_code& e, const std::string&currentlySearchingFor)
{
  if ( mReceivedStopRequest)
    return;

  if (e == boost::asio::error::operation_aborted)
    return;

  bool foundInIterator = false;

  //look through multimap to find
  std::pair <MultiOutstandingQueries::iterator, MultiOutstandingQueries::iterator> eqRange =  allOutstandingQueries.equal_range(currentlySearchingFor);

  //  std::cout<<"\n\nQuery timeout callback\n";

  MultiOutstandingQueries::iterator outQueriesIter;
  outQueriesIter = eqRange.first;

  while(outQueriesIter != eqRange.second)
  {
    if (outQueriesIter->second->gs == IndividualQueryData::GET )
    {
      outQueriesIter->second->traceToken->deadlineExpired = true;

      //says that this is a get.
      CraqOperationResult* cor  = new CraqOperationResult(CraqEntry(NullServerID,0),
                                                          outQueriesIter->second->currentlySearchingFor,
                                                          outQueriesIter->second->tracking_number,
                                                          false,
                                                          CraqOperationResult::GET,
                                                          false,
                                                          outQueriesIter->second->traceToken);

      cor->objID[CRAQ_DATA_KEY_SIZE -1] = '\0';

      mPostErrorsStrand->post(std::tr1::bind(&AsyncCraqScheduler::erroredGetValue, mSchedulerMaster, cor));


      if (outQueriesIter->second->deadline_timer != NULL)
      {
        outQueriesIter->second->deadline_timer->cancel();
        delete outQueriesIter->second->deadline_timer;
      }

      foundInIterator = true;

      delete outQueriesIter->second;  //delete this from a memory perspective
      allOutstandingQueries.erase(outQueriesIter++); //
    }
    else
    {
      outQueriesIter++;
    }
  }

  if (! foundInIterator)
    std::cout<<"\n\nAsyncConnectionGet CALLBACK.  Not found in iterator.\n\n";

}

//this function gets called whenever we haven't received a response to our query.  (Essentially, we just re-issue the query.)
//it should be called wrapped inside of osegStrand because interfacing with outstandingqueries.
void AsyncConnectionGet::queryTimedOutCallbackGetPrint(const boost::system::error_code& e, const std::string&)
{
  if ( mReceivedStopRequest)
    return;

  if (e == boost::asio::error::operation_aborted)
    return;

  std::cout<<"\n\nAsyncConnectionGet CALLBACK\n\n";

}




  AsyncConnectionGet::~AsyncConnectionGet()
  {
    outputLargeOutstanding();

    //delete all outstanding queries
    for (MultiOutstandingQueries::iterator outQuerIt = allOutstandingQueries.begin(); outQuerIt != allOutstandingQueries.end(); ++outQuerIt)
    {
      if (outQuerIt->second->deadline_timer != NULL)
      {
        outQuerIt->second->deadline_timer->cancel();
        delete   outQuerIt->second->deadline_timer;
      }

      if (outQuerIt->second->traceToken != NULL)
        delete outQuerIt->second->traceToken;

      delete outQuerIt->second;
    }
    allOutstandingQueries.clear();

    if (! NEED_NEW_SOCKET)
    {
      mSocket->cancel();
      mSocket->close();
      delete mSocket;
      mSocket = NULL;
    }
  }


  AsyncConnectionGet::ConnectionState AsyncConnectionGet::ready()
  {
    return mReady;
  }



  //gives us a socket to connect to
  void AsyncConnectionGet::initialize( Sirikata::Network::TCPSocket* socket,    boost::asio::ip::tcp::resolver::iterator it)
  {
    mSocket = socket;
    mReady = PROCESSING;   //need to run connection routine.  so until we receive an ack that conn has finished, we stay in processing state.

    mHandlerState = false;
    mSocket->async_connect(*it,mStrand->wrap(std::tr1::bind(&AsyncConnectionGet::connect_handler,this,_1)));  //using that tcp socket for an asynchronous connection.

    mPrevReadFrag = "";

    mTimesBetweenResults = 0;
    mAllResponseCount = 0;
  }


//connection handler.
void AsyncConnectionGet::connect_handler(const boost::system::error_code& error)
{
  if ( mReceivedStopRequest)
    return;

  if (error)
  {
    mSocket->cancel();
    mSocket->close();
    delete mSocket;
    mSocket = NULL;
    mReady = NEED_NEW_SOCKET;
    mPostErrorsStrand->post(mReadyStateChangedCallback);

    std::cout<<"\n\nError in connection\n\n";
    return;
  }

#ifdef ASYNC_CONNECTION_DEBUG
  std::cout<<"\n\nbftm debug: asyncConnection: connected\n\n";
#endif

  mReady = READY;
  mPostErrorsStrand->post(mReadyStateChangedCallback);

  set_generic_stored_not_found_error_handler();

  //run any outstanding get queries.
  runReQuery();
}


  int AsyncConnectionGet::runReQuery()
  {
    if ( mReceivedStopRequest)
      return 0;

    MultiOutstandingQueries::iterator it;

    int returner = 0;
    for (it = allOutstandingQueries.begin(); it != allOutstandingQueries.end(); ++it)
    {
      getQuery(it->second->currentlySearchingFor);
      ++ returner;
    }

    if (returner != 0)
      std::cout<<"\n\nThis is numToRequery: "<<returner<<"\n\n";

    if (! mHandlerState)
    {
      std::cout<<"\n\n***********************************HANDLER ISSUE*******************\n\n";
    }

    return returner;
  }


  void AsyncConnectionGet::printStatisticsTimesTaken()
  {
    double total = 0;

    for (int s=0; s< (int) mTimesTaken.size(); ++s)
      total += mTimesTaken[s];


    std::cout<<"\n\n\nTHIS IS TOTAL:   "<<total<<"\n\n";
    std::cout<<"\n\n\nTHIS IS AVG:     "<<total/((double) mTimesTaken.size())<<"\n\n";

  }


//dummy handler for writing the set instruction.  (Essentially, if we run into an error from doing the write operation of a set, we know what to do.)
void AsyncConnectionGet::write_some_handler_set(  const boost::system::error_code& error, std::size_t bytes_transferred)
{
  if ( mReceivedStopRequest)
    return;

  if (error)
  {
    printf("\n\nin write_some_handler_set\n\n");
    fflush(stdout);
    assert(false);
    killSequence();
  }
}



//datakey should have a null termination character.
//public interface for the get command
//called from inside of oseg_strand.
void AsyncConnectionGet::get(const CraqDataKey& dataToGet, OSegLookupTraceToken* traceToken)
{
  if( mReceivedStopRequest)
  {
    if (traceToken!= NULL)
      delete traceToken;
    return;
  }

  //Duration beginningDur = Time::local() - Time::epoch();
  //traceToken->getConnectionNetworkGetBegin = beginningDur.toMicroseconds();
  traceToken->stamp(OSegLookupTraceToken::OSEG_TRACE_GET_CONNECTION_NETWORK_GET_BEGIN);


  assert(mReady==PROCESSING);

  IndividualQueryData* iqd = new IndividualQueryData;
  iqd->is_tracking = false;
  iqd->tracking_number = 0;
  std::string tmpString = dataToGet;
  tmpString += STREAM_DATA_KEY_SUFFIX;
  strncpy(iqd->currentlySearchingFor,tmpString.c_str(),tmpString.size() + 1);
  iqd->gs = IndividualQueryData::GET;


  iqd->time_admitted = 0;


  //need to add the individual query data to allOutstandingQueries.
  allOutstandingQueries.insert(std::pair<std::string, IndividualQueryData*> (tmpString, iqd));


  iqd->deadline_timer  = new Sirikata::Network::DeadlineTimer(*ctx->ioService);
  iqd->deadline_timer->expires_from_now(boost::posix_time::milliseconds(STREAM_ASYNC_GET_TIMEOUT_MILLISECONDS));
  std::string bind_this_currently_searching_for(iqd->currentlySearchingFor);
  iqd->deadline_timer->async_wait(mStrand->wrap(std::tr1::bind(&AsyncConnectionGet::queryTimedOutCallbackGet, this, _1, bind_this_currently_searching_for)));

  iqd->traceToken = traceToken;


  getQuery(dataToGet);


  traceToken->stamp(OSegLookupTraceToken::OSEG_TRACE_GET_CONNECTION_NETWORK_GET_END);

}


  void AsyncConnectionGet::getBound(const CraqObjectID& obj_dataToGet, OSegLookupTraceToken* traceToken)
  {
    get(obj_dataToGet.cdk, traceToken);
  }


void AsyncConnectionGet::setProcessing()
{
    mReady=PROCESSING;
}

bool AsyncConnectionGet::getQuery(const CraqDataKey& dataToGet)
{
  if ( mReceivedStopRequest)
    return true;
  assert(mReady == PROCESSING);
  //crafts query
  std::string query;
  query.append(CRAQ_DATA_KEY_QUERY_PREFIX);
  query.append(dataToGet); //this is the re
  query += STREAM_DATA_KEY_SUFFIX; //bftm changed here.
  query.append(CRAQ_DATA_KEY_QUERY_SUFFIX);

  //sets write handler
  async_write((*mSocket),
              boost::asio::buffer(query),
              std::tr1::bind(&AsyncConnectionGet::write_some_handler_get,this,_1,_2));

  return true;
}


void AsyncConnectionGet::write_some_handler_get(  const boost::system::error_code& error, std::size_t bytes_transferred)
{
  if ( mReceivedStopRequest)
    return;
  mReady=READY;
  mPostErrorsStrand->post(mReadyStateChangedCallback);
  if (error)
  {
    printf("\n\nin write_some_handler_get\n\n");
    fflush(stdout);
    assert(false);
    killSequence();
  }
}


//This sequence needs to load all of its outstanding queries into the error results vector.
//
void AsyncConnectionGet::killSequence()
{
  if ( mReceivedStopRequest)
    return;

  mReady = NEED_NEW_SOCKET;
  mPostErrorsStrand->post(mReadyStateChangedCallback);
  mSocket->cancel();
  mSocket->close();
  delete mSocket;
  mSocket = NULL;


  printf("\n\n HIT KILL SEQUENCE \n\n");
  //  printf("\n\nHere is allResps:  %s\n\n ", mAllResps.c_str());

  fflush(stdout);
  assert(false);

  //  still need to load all the outstanding stuff into error vector
}



void AsyncConnectionGet::printOutstanding()
{
  MultiOutstandingQueries::iterator multiIter;

  std::cout<<"\n\n\n\n**************PRINTING OUTSTANDING*************************\n\n";

  for (multiIter = allOutstandingQueries.begin(); multiIter != allOutstandingQueries.end(); ++multiIter)
  {
    std::cout<<"\t"<<multiIter->first<<"      "<<multiIter->second<<"\n";
  }

  std::cout<<"\n\n";

}



//looks through the entire response string and processes out relevant information:
//  "ABCDSTORED000000000011000000000000000000000ZVALUE000000000000000000000000000000000Z120000000000YYSTORED000000000022000000000000000000000ZSTORED000000000000003300000000000000000ZNOT_FOUND000000000011000000000000000000000ZERROR000000000011000000000000000000000Z"
// returns true if anything matches the basic template.  false otherwise
bool AsyncConnectionGet::processEntireResponse(std::string response)
{
  if ( mReceivedStopRequest)
    return true;

  //index from stored
  //not_found
  //value
  //error
  bool returner = false;
  bool firstTime = true;

  bool keepChecking = true;
  bool secondChecking;

  response = mPrevReadFrag + response;  //see explanation at end when re-setting mPrevReadFrag
  mPrevReadFrag = "";

  int numChecking = 0;

  while(keepChecking)
  {
    keepChecking   =                            false;

    //checks to see if there are any responses to get queries with data (also processes);
    secondChecking =             checkValue(response);
    keepChecking   =   keepChecking || secondChecking;

    //checks to see if there are any responses to set responses that worked (stored) (also processes)
    secondChecking =            checkStored(response);
    keepChecking   =   keepChecking || secondChecking;

    //checks to see if there are any responses to get queries that were not found  (also processes)
    secondChecking =          checkNotFound(response);
    keepChecking   =   keepChecking || secondChecking;

    //checks to see if there are any error responses.  (also processes)
    secondChecking =             checkError(response);
    keepChecking   =   keepChecking || secondChecking;

    if (firstTime)
    {
      returner  = keepChecking;  //states whether or not there were any full-formed expressions in this read
      firstTime =        false;
    }
  }

  mPrevReadFrag = response;  //apparently I've been running into the problem of what happens when data gets interrupted mid-stream
                             //The solution is to save the end bit of data that couldn't be parsed correctly (now in "response" variable and save it for appending to the next read.

  if ((int)mPrevReadFrag.size() > MAX_GET_PREV_READ_FRAG_SIZE)
    mPrevReadFrag.substr(((int)mPrevReadFrag.size()) - CUT_GET_PREV_READ_FRAG);

  return returner;
}




/*
get abc
NOT_FOUND abc
set abc 3
def
STORED abc
get abc
VALUE abc 3
def
*/

// This function takes in a mutable string
// If there is a *full* not found message in response:
//  1) Removes it from response;
//  2) Add it to operation result queue
//  3) returns true
//
//Otherwise, returns false.
//
bool AsyncConnectionGet::checkNotFound(std::string& response)
{
  bool returner = false;
  size_t notFoundIndex = response.find(CRAQ_NOT_FOUND_RESP);

  std::string prefixed = "";
  std::string suffixed = "";


  if (notFoundIndex != std::string::npos)
  {
    prefixed = response.substr(0,notFoundIndex); //prefixed will be everything before the first STORED tag


    suffixed = response.substr(notFoundIndex); //first index should start with STORED_______

    size_t suffixNotFoundIndex = suffixed.find(CRAQ_NOT_FOUND_RESP);

    std::vector<size_t> tmpSizeVec;
    tmpSizeVec.push_back(suffixed.find(CRAQ_NOT_FOUND_RESP,STREAM_CRAQ_NOT_FOUND_RESP_SIZE ));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_ERROR_RESP));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_STORED_RESP));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_VALUE_RESP));

    //Looking until we get to Z
    size_t endValueIndex = suffixed.find("YZ");
    if (endValueIndex != std::string::npos)
    {
      tmpSizeVec.push_back(suffixed.find("YZ") + 2);
    }



    size_t smallestNext = smallestIndex(tmpSizeVec);
    std::string notFoundPhrase;
    if (smallestNext != std::string::npos)
    {
      //means that the smallest next
      notFoundPhrase = suffixed.substr(suffixNotFoundIndex, smallestNext);

      suffixed = suffixed.substr(smallestNext);


      std::string dataKey;
      if ( parseValueNotFound(notFoundPhrase,dataKey))
      {
        //checks to make sure that the notFoundPhrase is fully and correctly formed
        processValueNotFound(dataKey);
        response = prefixed +suffixed;
        returner = true;
      }
      else
      {
        //the notfound phrase was incomplete.  we can't process it.  we return the response to its original state and return false immediately.
        response = prefixed + notFoundPhrase + suffixed;
        return false;
      }

    }
    else
    {
      //means that the stored value is the last
      notFoundPhrase = suffixed.substr(suffixNotFoundIndex);
      response = prefixed;


      std::string dataKey;
      if ( parseValueNotFound(notFoundPhrase,dataKey))
      {
        //checks to make sure that the notFoundPhrase is fully and correctly formed
        processValueNotFound(dataKey);
        returner = true;
      }
      else
      {
        //the notfound phrase was incomplete.  we can't process it.  we return the response to its original state and return false immediately.
        response = response + notFoundPhrase;
        return false;
      }

    }
    //the above should have grabbed a phrase starting with "NOT_FOUND" from a sequence of characters
    //notFoundPhrase should store everything from NOT_FOUND up until either the end of response, or until a phrase keyed from a new keyword begins.  (eg. another "NOT_FOUND" or "STORED" or "VALUE" or "ERROR".)


  }
  return returner;
}

//checks a string to see if it's a correctly formatted not_found message.  If it is, grab data key from it, and return it in dataKey tab.
//if it is not formatted correctly, returns false
bool AsyncConnectionGet::parseValueNotFound(std::string response, std::string& dataKey)
{
  size_t notFoundIndex = response.find(CRAQ_NOT_FOUND_RESP);

  if (notFoundIndex == std::string::npos)
    return false;//means that there isn't actually a not found tag in this

  if (notFoundIndex != 0)
    return false;//means that not found was in the wrong place.  return false so that can initiate kill sequence.

  //the not_found value was upfront.
  dataKey = response.substr(STREAM_CRAQ_NOT_FOUND_RESP_SIZE, CRAQ_DATA_KEY_SIZE);

  if ((int)dataKey.size() != CRAQ_DATA_KEY_SIZE)
    return false;  //didn't read enough of the key header

  return true;
}



//takes the data key associated with a not found message, and loads it into operation result vector.
void AsyncConnectionGet::processValueNotFound(std::string dataKey)
{
  if ( mReceivedStopRequest)
    return;

  //look through multimap to find
  std::pair <MultiOutstandingQueries::iterator, MultiOutstandingQueries::iterator> eqRange =  allOutstandingQueries.equal_range(dataKey);

  MultiOutstandingQueries::iterator outQueriesIter;
  outQueriesIter = eqRange.first;

  while(outQueriesIter != eqRange.second)
  {
    if (outQueriesIter->second->gs == IndividualQueryData::GET )
    {
      outQueriesIter->second->traceToken->notFound = true;

      //ugly: this should not be here.
      ++mOSeg->mOSegQueueLen;
      outQueriesIter->second->traceToken->osegQLenPostReturn = mOSeg->mOSegQueueLen;


      //says that this is a get.
      CraqOperationResult* cor  = new CraqOperationResult (CraqEntry(NullServerID,0),
                                                           outQueriesIter->second->currentlySearchingFor,
                                                           outQueriesIter->second->tracking_number,
                                                           true,
                                                           CraqOperationResult::GET,
                                                           false,
                                                           outQueriesIter->second->traceToken); //this is a not_found, means that we add 0 for the id found

      cor->objID[CRAQ_DATA_KEY_SIZE -1] = '\0';


      mResultStrand->post(std::tr1::bind(&CraqObjectSegmentation::craqGetResult, mOSeg, cor));


      if (outQueriesIter->second->deadline_timer != NULL)
      {
        outQueriesIter->second->deadline_timer->cancel();
        delete outQueriesIter->second->deadline_timer;
      }


      delete outQueriesIter->second;  //delete this from a memory perspective
      allOutstandingQueries.erase(outQueriesIter++); //
    }
    else
    {
      outQueriesIter++;
    }
  }
}



//just see comments for not found stuff
//looks to see if we received a value in the response.
//eg: VALUE000000000000000000000000000000000Z120000000000YY
bool AsyncConnectionGet::checkValue(std::string& response)
{
//   static int debugCounter = 0;

//   if (response.size() > 8000)
//   {
//     ++debugCounter;
//     std::cout<<"\n\nRESPONSE GOT TOO BIG: \n\n";
//     std::cout<<response<<"\n";
//     std::cout.flush();

//     if (debugCounter > 5)
//       assert(false);
//   }


  bool returner = false;
  size_t valueIndex = response.find(STREAM_CRAQ_VALUE_RESP);

  std::string prefixed = "";
  std::string suffixed = "";

  if (valueIndex != std::string::npos)
  {
    prefixed = response.substr(0,valueIndex); //prefixed will be everything before the first STORED tag

    suffixed = response.substr(valueIndex); //first index should start with STORED_______

    size_t suffixValueIndex = suffixed.find(STREAM_CRAQ_VALUE_RESP);

    std::vector<size_t> tmpSizeVec;
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_VALUE_RESP,STREAM_CRAQ_VALUE_RESP_SIZE ));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_ERROR_RESP));
    tmpSizeVec.push_back(suffixed.find(CRAQ_NOT_FOUND_RESP));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_STORED_RESP));
    //Looking until we get to Z
    size_t endValueIndex = suffixed.find("YZ");
    if (endValueIndex != std::string::npos)
    {
      tmpSizeVec.push_back(suffixed.find("YZ") + 2);
    }

    size_t smallestNext = smallestIndex(tmpSizeVec);

    std::string valuePhrase;
    if (smallestNext != std::string::npos)
    {
      //means that the smallest next
      valuePhrase = suffixed.substr(suffixValueIndex, smallestNext);
      suffixed = suffixed.substr(smallestNext);
      //      response = prefixed +suffixed;

      std::string dataKey;
      CraqEntry sID(NullServerID,0);
      if (parseValueValue(valuePhrase,dataKey,sID)) //sends it
      {
        processValueFound(dataKey,sID);
        response = prefixed + suffixed;
        returner = true;  //set returner here because we know we got a correct response.
      }
      else
      {
        response = prefixed + valuePhrase + suffixed;
        //        response = response + valuePhrase + suffixed;
        return false;
      }
    }
    else
    {
      //means that the stored value is the last
      valuePhrase = suffixed.substr(suffixValueIndex);
      response = prefixed;

      std::string dataKey;
      CraqEntry sID(NullServerID,0);
      if (parseValueValue(valuePhrase,dataKey,sID)) //sends it
      {
        processValueFound(dataKey,sID);
        returner = true;  //set returner here because we know we got a correct response.
      }
      else
      {
        response = response + valuePhrase;
        //        printf("\n\nOffending response: %s\n ",response.c_str() );
        return false;
      }
    }
  }
  return returner;
}


//takes the string associated with the datakey of a value found message and inserts it into operation value found
void AsyncConnectionGet::processValueFound(std::string dataKey, const CraqEntry& sID)
{
  if ( mReceivedStopRequest)
    return;

  //look through multimap to find
  std::pair  <MultiOutstandingQueries::iterator, MultiOutstandingQueries::iterator> eqRange = allOutstandingQueries.equal_range(dataKey);

  MultiOutstandingQueries::iterator outQueriesIter;
  outQueriesIter = eqRange.first;
  while(outQueriesIter != eqRange.second)
  {
    if (outQueriesIter->second->gs == IndividualQueryData::GET) //we only need to
    {
        //Duration gotItDur = Time::local() - Time::epoch();
        //outQueriesIter->second->traceToken->getConnectionNetworkReceived = gotItDur.toMicroseconds();
        outQueriesIter->second->traceToken->stamp(OSegLookupTraceToken::OSEG_TRACE_GET_CONNECTION_NETWORK_RECEIVED);

      //ugly: this should not be here.
      ++mOSeg->mOSegQueueLen;
      outQueriesIter->second->traceToken->osegQLenPostReturn = mOSeg->mOSegQueueLen;


      CraqOperationResult* cor  = new CraqOperationResult (sID,
                                                           outQueriesIter->second->currentlySearchingFor,
                                                           outQueriesIter->second->tracking_number,
                                                           true,
                                                           CraqOperationResult::GET,
                                                           false,
                                                           outQueriesIter->second->traceToken);

      cor->objID[CRAQ_DATA_KEY_SIZE -1] = '\0';



      mResultStrand->post(std::tr1::bind(&CraqObjectSegmentation::craqGetResult, mOSeg, cor));



      if (outQueriesIter->second->deadline_timer != NULL)
      {
        outQueriesIter->second->deadline_timer->cancel();
        delete outQueriesIter->second->deadline_timer;
      }

      delete outQueriesIter->second;  //delete this from a memory perspective
      allOutstandingQueries.erase(outQueriesIter++); //
    }
    else
    {
      outQueriesIter++;
    }
  }
}


//VALUE|CRAQ KEY|SIZE|VALUE
//returns the datakey and id associated with a value found response.
bool AsyncConnectionGet::parseValueValue(std::string response, std::string& dataKey,CraqEntry& sID)
{
  size_t valueIndex = response.find(STREAM_CRAQ_VALUE_RESP);


  if (valueIndex == std::string::npos)
    return false;//means that value isn't actually in the response

  if (valueIndex != 0)
    return false;  //means that the value is in the wrong place.  return false so that can initiate kill sequence.


  //********Parse data key

  dataKey = response.substr(STREAM_CRAQ_VALUE_RESP_SIZE, CRAQ_DATA_KEY_SIZE);

  if ((int)dataKey.size() != CRAQ_DATA_KEY_SIZE)
    return false;  //didn't read enough of the key header

  //*******Parse server id

  if (STREAM_CRAQ_VALUE_RESP_SIZE + CRAQ_DATA_KEY_SIZE + STREAM_SIZE_SIZE_TAG_GET_RESPONSE > (int)response.size())
  {
    //    printf("\n\nThis is the response we're trying to substring ERROR:  %s\n\n", response.c_str());
    //    fflush(stdout);
    return false;
  }

  std::string tmpSID = response.substr(STREAM_CRAQ_VALUE_RESP_SIZE + CRAQ_DATA_KEY_SIZE + STREAM_SIZE_SIZE_TAG_GET_RESPONSE, CRAQ_SERVER_SIZE); // the +2 is from



//   if ((int)tmpSID.size() != CRAQ_SERVER_SIZE)
//     return false; //didn't read enough of the key header to find server id




  //parse tmpSID to int
  sID=CraqEntry((unsigned char*)tmpSID.c_str());

  return true;
}



void AsyncConnectionGet::processStoredValue(std::string dataKey)
{
  if ( mReceivedStopRequest)
    return;

  //look through multimap to find
  std::pair  <MultiOutstandingQueries::iterator, MultiOutstandingQueries::iterator> eqRange = allOutstandingQueries.equal_range(dataKey);

  MultiOutstandingQueries::iterator outQueriesIter;
  outQueriesIter = eqRange.first;

  while(outQueriesIter != eqRange.second)
  {
    if (outQueriesIter->second->gs == IndividualQueryData::SET) //we only need to
    {
      CraqOperationResult* cor  = new CraqOperationResult (outQueriesIter->second->currentlySettingTo,
                                                           outQueriesIter->second->currentlySearchingFor,
                                                           outQueriesIter->second->tracking_number,
                                                           true,
                                                           CraqOperationResult::SET,
                                                           outQueriesIter->second->is_tracking); //this is a not_found, means that we add 0 for the id found

      cor->objID[CRAQ_DATA_KEY_SIZE -1] = '\0';

      mResultStrand->post(std::tr1::bind(&CraqObjectSegmentation::craqSetResult, mOSeg, cor));


      if (outQueriesIter->second->deadline_timer != NULL)
      {
        outQueriesIter->second->deadline_timer->cancel();
        delete outQueriesIter->second->deadline_timer;
      }


      delete outQueriesIter->second;  //delete this from a memory perspective
      allOutstandingQueries.erase(outQueriesIter++); //
    }
    else
    {
      outQueriesIter++;
    }
  }
}


bool AsyncConnectionGet::parseStoredValue(const std::string& response, std::string& dataKey)
{

  size_t storedIndex = response.find(STREAM_CRAQ_STORED_RESP);

  if (storedIndex == std::string::npos)
    return false;//means that there isn't actually a not found tag in this

  if (storedIndex != 0)
    return false;//means that not found was in the wrong place.  return false so that can initiate kill sequence.

  //the not_found value was upfront.
  dataKey = response.substr(STREAM_CRAQ_STORED_RESP_SIZE, CRAQ_DATA_KEY_SIZE);

  if ((int)dataKey.size() != CRAQ_DATA_KEY_SIZE)
    return false;  //didn't read enough of the key header

  return true;

}




void AsyncConnectionGet::set_generic_stored_not_found_error_handler()
{
  if ( mReceivedStopRequest)
    return;

  boost::asio::streambuf * sBuff = new boost::asio::streambuf;
  //  const boost::regex reg ("(ND\r\n|ERROR\r\n)");  //read until error or get a response back.  (Note: ND is the suffix attached to set values so that we know how long to read.
  //  const boost::regex reg ("Z\r\n");  //read until error or get a response back.  (Note: ND is the suffix attached to set values so that we know how long to read.

  const boost::regex reg ("(Z\r\n|ERROR\r\n)");  //read until error or get a response back.  (Note: ND is the suffix attached to set values so that we know how long to read.

  mHandlerState = true;

  //sets read handler
  boost::asio::async_read_until((*mSocket),
                                (*sBuff),
                                reg,
                                mStrand->wrap(std::tr1::bind(&AsyncConnectionGet::generic_read_stored_not_found_error_handler,this,_1,_2,sBuff)));

}




void AsyncConnectionGet::generic_read_stored_not_found_error_handler ( const boost::system::error_code& error, std::size_t bytes_transferred, boost::asio::streambuf* sBuff)
{
  if( mReceivedStopRequest)
  {
    delete sBuff;
    return;
  }



  mHandlerState = false;

  if (error)
  {

    if (mSocket == NULL)
      return;

    std::cout<<"\n\nTHIS UPPER ERROR\n\n";

    killSequence();
    return;
  }

  std::istream is(sBuff);
  std::string response = "";
  std::string tmpLine;

  is >> tmpLine;


  while (tmpLine.size() != 0)
  {
    response.append(tmpLine);
    tmpLine = "";
    is >> tmpLine;
  }


  bool anything = processEntireResponse(response); //this will go through everything that we read out.  And sort it by errors, storeds, not_founds, and values.

  delete sBuff;
  set_generic_stored_not_found_error_handler();
}


int AsyncConnectionGet::getRespCount()
{
  return mAllResponseCount;
}


// Looks for and removes all instances of complete stored messages
// Processes them as well
//
bool AsyncConnectionGet::checkStored(std::string& response)
{
  bool returner = false;
  size_t storedIndex = response.find(STREAM_CRAQ_STORED_RESP);

  std::string prefixed = "";
  std::string suffixed = "";


  if (storedIndex != std::string::npos)
  {
    prefixed = response.substr(0,storedIndex); //prefixed will be everything before the first STORED tag


    suffixed = response.substr(storedIndex); //first index should start with STORED_______

    size_t suffixStoredIndex = suffixed.find(STREAM_CRAQ_STORED_RESP);

    std::vector<size_t> tmpSizeVec;
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_STORED_RESP,STREAM_CRAQ_STORED_RESP_SIZE ));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_ERROR_RESP));
    tmpSizeVec.push_back(suffixed.find(CRAQ_NOT_FOUND_RESP));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_VALUE_RESP));

    size_t smallestNext = smallestIndex(tmpSizeVec);
    std::string storedPhrase;
    if (smallestNext != std::string::npos)
    {
      //means that the smallest next
      storedPhrase = suffixed.substr(suffixStoredIndex, smallestNext);
      suffixed = suffixed.substr(smallestNext);
      response = prefixed +suffixed;
    }
    else
    {
      //means that the stored value is the last
      storedPhrase = suffixed.substr(suffixStoredIndex);

      response = prefixed;
    }

    std::string dataKey;
    if (parseStoredValue(storedPhrase, dataKey))
    {
      returner = true;
      processStoredValue(dataKey);
    }
    else
    {
      response = prefixed + storedPhrase;
      return false;
    }
  }
  return returner;
}




//returns the smallest entry in the entered vector.
size_t AsyncConnectionGet::smallestIndex(std::vector<size_t> sizeVec)
{
  std::sort(sizeVec.begin(), sizeVec.end());

  return sizeVec[0];
}



bool AsyncConnectionGet::checkError(std::string& response)
{
  bool returner = false;
  size_t errorIndex = response.find(STREAM_CRAQ_ERROR_RESP);

  std::string prefixed = "";
  std::string suffixed = "";


  if (errorIndex != std::string::npos)
  {
    prefixed = response.substr(0,errorIndex); //prefixed will be everything before the first STORED tag


    suffixed = response.substr(errorIndex); //first index should start with STORED_______

    size_t suffixErrorIndex = suffixed.find(STREAM_CRAQ_ERROR_RESP);

    std::vector<size_t> tmpSizeVec;
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_ERROR_RESP,STREAM_CRAQ_ERROR_RESP_SIZE ));
    tmpSizeVec.push_back(suffixed.find(CRAQ_NOT_FOUND_RESP));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_STORED_RESP));
    tmpSizeVec.push_back(suffixed.find(STREAM_CRAQ_VALUE_RESP));


    size_t endValueIndex = suffixed.find("YZ");
    if (endValueIndex != std::string::npos)
    {
      tmpSizeVec.push_back(suffixed.find("YZ") + 2);
    }


    size_t smallestNext = smallestIndex(tmpSizeVec);
    std::string errorPhrase;
    if (smallestNext != std::string::npos)
    {
      //means that the smallest next
      errorPhrase = suffixed.substr(suffixErrorIndex, smallestNext);
      suffixed = suffixed.substr(smallestNext);
      returner = true;

      response = prefixed +suffixed;
    }
    else
    {
      //means that the stored value is the last
      errorPhrase = suffixed.substr(suffixErrorIndex);
      returner = true;
      response = prefixed;
    }

  }
  return returner;
}

}
