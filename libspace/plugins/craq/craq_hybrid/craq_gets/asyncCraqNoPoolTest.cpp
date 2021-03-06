/*  Sirikata
 *  asyncCraqNoPoolTest.cpp
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

#include <iostream>
#include <iomanip>

#include "asyncCraqGet.hpp"
#include <boost/asio.hpp>
#include <string.h>
#include <vector>

#include "../asyncCraqUtil.hpp"
#include "asyncConnectionGet.hpp"

#include "../../craq_oseg/asyncUtil.hpp"

#include <sys/time.h>

//#define WHICH_SERVER  "localhost"
//#define WHICH_SERVER       "meru22"

//#define WHICH_SERVER        "127.0.0.1"
//#define WHICH_PORT          "10499"

//#define WHICH_SERVER        "meru21"
#define WHICH_SERVER        "localhost"
#define WHICH_PORT          "10499"

namespace Sirikata
{


void printResponse(CraqOperationResult* res);

void basicWait(AsyncCraqGet* myReader,std::vector<CraqOperationResult*> &allGetResults,std::vector<CraqOperationResult*>&allTrackedResults);

void iteratedWait(int numWaits,AsyncCraqGet* myReader,std::vector<CraqOperationResult*> &allGetResults,std::vector<CraqOperationResult*>&allTrackedResults);


void craftQueries(std::vector<CraqDataSetGet*>& vect);
void runMultiUnLoadSpeed(int numTimesLoad);




void basicWaitMulti(AsyncCraqGet* myReader,std::vector<CraqOperationResult*> &allGetResults,std::vector<CraqOperationResult*>&allTrackedResults);
void iteratedWaitMulti(int numWaits,AsyncCraqGet* myReader,std::vector<CraqOperationResult*> &allGetResults,std::vector<CraqOperationResult*>&allTrackedResults);
void runMultiUnLoadSpeedMulti(int numTimesLoad);





//static const int numPuts = 400;
  static const int numPuts = 400;
static const int numInit = 10;

void runLoad();
void runUnLoad();
void runMultiUnLoad(int numTimesLoad);
}



int main (int argc, char** argv)
{
  //  runLoad();
  //  runMultiUnLoad(200);

  std::cout<<"\n\nAbout to run\n\n";
  std::cout.flush();
  //CBR::runMultiUnLoadSpeed(400);
    CBR::runMultiUnLoadSpeedMulti(400);
    //  CBR::runMultiUnLoadSpeedMulti(400);
  std::cout<<"\n\nDONE\n\n";


  //  runMultiUnLoad(40);
  //  runUnLoad();

  return 0;
}


namespace Sirikata
{

void runLoad()
{
  AsyncCraqGet myReader1(NULL,NULL);

  //declare craq args
  std::vector<CraqInitializeArgs> craqArgs1;
  CraqInitializeArgs cInitArgs;
  cInitArgs.ipAdd   =     WHICH_SERVER;
  cInitArgs.port    =       WHICH_PORT;

  craqArgs1.push_back(cInitArgs);

  std::vector<CraqOperationResult*> allGets;
  std::vector<CraqOperationResult*> allTrackedSets;


  std::cout<<"\n\nAbout to perform reader initialization.\n\n";
  std::cout.flush();
  myReader1.initialize(craqArgs1);

  for(int s=0; s < 10000; ++s)
  {
    iteratedWait(1,&myReader1,allGets, allTrackedSets);
    //    iteratedWait(1,&myReader2,allGets, allTrackedSets);
  }

  std::cout<<"\n\nAbout to perform first set of sets\n\n";
  std::cout.flush();
  //performing first sets
  CraqDataKey cDK;
  std::string query;

  struct timeval first_time, finished_time;
  gettimeofday(&first_time,NULL);

  for (int s=numInit; s < numInit+ numPuts; ++s)
  {
    query = "";
    std::stringstream ss;
    ss << s;
    std::string tmper = ss.str();

    for (int t=0; t< CRAQ_DATA_KEY_SIZE - (((int)tmper.size()) + 1); ++t)
    {
      query.append("0");
    }
    query.append(tmper);


    CraqDataSetGet cdSetGet (query,s,false,CraqDataSetGet::SET);
    myReader1.set(cdSetGet);

    iteratedWait(1,&myReader1,allGets,allTrackedSets);

    std::cout<<"\n\nLoading: "<<s;
    std::cout.flush();

  }

  for(int s=0; s < 100000; ++s)
  {
    iteratedWait(1,&myReader1,allGets, allTrackedSets);
    iteratedWait(1,&myReader1,allGets, allTrackedSets);
  }

  //about to perform gets

  //  iteratedWait(100000,&myReader,allGets, allTrackedSets);

  for(int s=0; s < 100000; ++s)
  {
    iteratedWait(1,&myReader1,allGets, allTrackedSets);
    iteratedWait(1,&myReader1,allGets, allTrackedSets);
  }

  for(int s=0; s < 100000; ++s)
  {
    iteratedWait(100,&myReader1,allGets, allTrackedSets);
    iteratedWait(100,&myReader1,allGets, allTrackedSets);
  }

  int numIter = 0;

  while ((numIter < 100) && (myReader1.numStillProcessing() != 0))
  {
    std::cout<<"\n\nThis is numIter:  "<<numIter<<"\n\n";
    std::cout<<"\n\nThis is numStillProcessing:   "<<myReader1.numStillProcessing()<<"\n\n";
    std::cout<<"\n\n This is num gets:   "<<allGets.size()<<"\n\n";
    std::cout.flush();

    for(int s=0; s < 100000; ++s)
    {
      iteratedWait(20,&myReader1,allGets, allTrackedSets);
      iteratedWait(20,&myReader1,allGets, allTrackedSets);
    }
    ++numIter;
  }




  int allGetsSize = (int) allGets.size();
  std::cout<<"\n\nThis is size of allGets:   "<<allGets.size()<<"\n\n";
  std::cout.flush();

  for (int s=0; s < (int)allGets.size(); ++ s)
  {
    printResponse(allGets[s]);
  }

  gettimeofday(&finished_time,NULL);
  double secondsElapsed  = finished_time.tv_sec - first_time.tv_sec;
  double uSecondsElapsed = finished_time.tv_usec - first_time.tv_usec;
  double realTimeTakenSeconds = secondsElapsed + uSecondsElapsed/(1000000);
  std::cout<<"\n\nThis is timer final:  "<<realTimeTakenSeconds<<"\n\n";
  std::cout<<"\n\nThis is size of allGets:   "<<allGetsSize<<"\n\n";
  std::cout.flush();
}





void runUnLoad()
{
  //declare reader
  AsyncCraqGet myReader1(NULL,NULL);


  //declare craq args
  std::vector<CraqInitializeArgs> craqArgs1;
  CraqInitializeArgs cInitArgs;
  cInitArgs.ipAdd   =     WHICH_SERVER;
  cInitArgs.port    =       WHICH_PORT;



  craqArgs1.push_back(cInitArgs);

  std::vector<CraqOperationResult*> allGets;
  std::vector<CraqOperationResult*> allTrackedSets;


  std::cout<<"\n\nAbout to perform reader initialization.\n\n";
  myReader1.initialize(craqArgs1);


  for(int s=0; s < 10000; ++s)
  {
    iteratedWait(1,&myReader1,allGets, allTrackedSets);
    //    iteratedWait(1,&myReader2,allGets, allTrackedSets);
  }


  std::cout<<"\n\nAbout to perform first set of sets\n\n";
  //performing first sets
  CraqDataKey cDK;
  std::string query;



  struct timeval first_time, finished_time;
  gettimeofday(&first_time,NULL);



  for(int s=0; s < 100000; ++s)
  {
    iteratedWait(1,&myReader1,allGets, allTrackedSets);
    iteratedWait(1,&myReader1,allGets, allTrackedSets);
  }

  //about to perform gets
  std::cout<<"\n\nAbout to perform a bunch of gets\n\n";

  //  for (int s=0; s < numPuts; ++s)
  for (int s=numInit; s < numInit+ numPuts; ++s)
  {
    query = "";
    std::stringstream ss;
    ss << s;
    std::string tmper = ss.str();

    for (int t=0; t< CRAQ_DATA_KEY_SIZE - (((int)tmper.size()) + 1); ++t)
    {
      query.append("0");
    }
    query.append(tmper);


    CraqDataSetGet cdSetGet2 (query,s,false,CraqDataSetGet::GET);
    myReader1.get(cdSetGet2);

    iteratedWait(1,&myReader1,allGets,allTrackedSets);

  }

  //  iteratedWait(100000,&myReader,allGets, allTrackedSets);

  for(int s=0; s < 100000; ++s)
  {
    iteratedWait(1,&myReader1,allGets, allTrackedSets);
    iteratedWait(1,&myReader1,allGets, allTrackedSets);
  }

  for(int s=0; s < 100000; ++s)
  {
    iteratedWait(5,&myReader1,allGets, allTrackedSets);
    iteratedWait(5,&myReader1,allGets, allTrackedSets);
  }


  int allGetsSize = (int) allGets.size();
  std::cout<<"\n\nThis is size of allGets:   "<<allGets.size()<<"\n\n";


  for (int s=0; s < (int)allGets.size(); ++ s)
  {
    printResponse(allGets[s]);
  }

  gettimeofday(&finished_time,NULL);
  double secondsElapsed  = finished_time.tv_sec - first_time.tv_sec;
  double uSecondsElapsed = finished_time.tv_usec - first_time.tv_usec;
  double realTimeTakenSeconds = secondsElapsed + uSecondsElapsed/(1000000);
  std::cout<<"\n\nThis is timer final:  "<<realTimeTakenSeconds<<"\n\n";
  std::cout<<"\n\nThis is size of allGets:   "<<allGetsSize<<"\n\n";
}




void runMultiUnLoad(int numTimesLoad)
{
  //declare reader
  AsyncCraqGet myReader1(NULL,NULL);


  //declare craq args
  std::vector<CraqInitializeArgs> craqArgs1;
  CraqInitializeArgs cInitArgs;
  cInitArgs.ipAdd   =     WHICH_SERVER;
  cInitArgs.port    =       WHICH_PORT;



  craqArgs1.push_back(cInitArgs);

  std::vector<CraqOperationResult*> allGets;
  std::vector<CraqOperationResult*> allTrackedSets;


  std::cout<<"\n\nAbout to perform reader initialization.\n\n";
  myReader1.initialize(craqArgs1);


  for(int s=0; s < 10000; ++s)
  {
    iteratedWait(1,&myReader1,allGets, allTrackedSets);
    //    iteratedWait(1,&myReader2,allGets, allTrackedSets);
  }


  std::cout<<"\n\nAbout to perform first set of sets\n\n";
  //performing first sets
  CraqDataKey cDK;
  std::string query;


  iteratedWait(200000,&myReader1,allGets, allTrackedSets);



  struct timeval first_time, finished_time;
  gettimeofday(&first_time,NULL);


  //about to perform gets
  std::cout<<"\n\nAbout to perform a bunch of gets\n\n";

  int numgetting  = 0;

  for (int a = 0; a < numTimesLoad; ++a)
  {
    for (int s=numInit; s < numInit+ numPuts; ++s)
    {
      query = "";
      std::stringstream ss;
      ss << s;
      std::string tmper = ss.str();

      for (int t=0; t< CRAQ_DATA_KEY_SIZE - (((int)tmper.size()) + 1); ++t)
      {
        query.append("0");
      }
      query.append(tmper);


      CraqDataSetGet cdSetGet2 (query,s,false,CraqDataSetGet::GET);
      myReader1.get(cdSetGet2);

      //      std::cout<<a*numPuts + s<<"\n";

      ++numgetting;
      if (numgetting %50 == 0)
        iteratedWait(1,&myReader1,allGets,allTrackedSets);

      //      else
      //        usleep(1);

    }
  }



  iteratedWait(10000,&myReader1,allGets, allTrackedSets);



  int numIter = 0;

  while ((numIter < 300) && (myReader1.numStillProcessing() != 0))
  {
    std::cout<<"\n\nThis is numIter:  "<<numIter<<"\n";
    std::cout<<"This is numStillProcessing:   "<<myReader1.numStillProcessing()<<"\n";
    std::cout<<"This is the size of allGets:  "<<allGets.size()<<"\n\n\n";
    std::cout.flush();


    iteratedWait(30000,&myReader1,allGets, allTrackedSets);

    ++numIter;

    if (numIter %100 == 0)
    {
      //      std::cout<<"\n\n**********************RE-RUNNING*********\n\n";
      //      int value = myReader1.runReQuery();
      //      std::cout<<"\t\tNum re-queried:  "<<value<<"\n\n";
    }
  }

  int allGetsSize = (int) allGets.size();
  std::cout<<"\n\nThis is size of allGets:   "<<allGets.size()<<"\n\n";


  for (int s=0; s < (int)allGets.size(); ++ s)
  {
    printResponse(allGets[s]);
  }

  gettimeofday(&finished_time,NULL);
  double secondsElapsed  = finished_time.tv_sec - first_time.tv_sec;
  double uSecondsElapsed = finished_time.tv_usec - first_time.tv_usec;
  double realTimeTakenSeconds = secondsElapsed + uSecondsElapsed/(1000000);
  std::cout<<"\n\nThis is timer final:  "<<realTimeTakenSeconds<<"\n\n";
  std::cout<<"\n\nThis is size of allGets:   "<<allGetsSize<<"\n\n";

  std::cout<<"\n\nThis is response count:  ";
  int counter = myReader1.getRespCount();
  std::cout<<counter<<"\n\n\n\n";

}




void runMultiUnLoadSpeedMulti(int numTimesLoad)
{

  //declare reader
  AsyncCraqGet myReader1(NULL,NULL);


  //declare craq args
  std::vector<CraqInitializeArgs> craqArgs1;
  CraqInitializeArgs cInitArgs;
  cInitArgs.ipAdd   =     WHICH_SERVER;
  cInitArgs.port    =       WHICH_PORT;



  craqArgs1.push_back(cInitArgs);

  std::vector<CraqOperationResult*> allGets;
  std::vector<CraqOperationResult*> allTrackedSets;


  std::cout<<"\n\nAbout to perform reader initialization.\n\n";
  myReader1.initialize(craqArgs1);

  iteratedWait(10000,&myReader1,allGets, allTrackedSets);

  std::cout<<"\n\nAbout to perform first set of sets\n\n";
  //performing first sets

  iteratedWait(200000,&myReader1,allGets, allTrackedSets);

  //constructing queries
  std::vector<CraqDataSetGet*> allQueries;
  craftQueries(allQueries);


  //about to perform gets
  std::cout<<"\n\nAbout to perform a bunch of gets\n\n";
  std::cout.flush();
  int numgetting  = 0;

  usleep(100);


  //initializing timer
  struct timeval first_time, middle_time, finished_time;
  gettimeofday(&first_time,NULL);

  for (int a = 0; a < numTimesLoad; ++a)
  {
    for (int s=numInit; s < numInit+ numPuts; ++s)
    {
      myReader1.getMulti(*(allQueries[s-numInit]));

      ++numgetting;
      if (numgetting %50 == 0)
      {
        iteratedWaitMulti(1,&myReader1,allGets,allTrackedSets);
      }

      //      else
      //      {
      //        usleep(1);
      //      }

    }
  }

  gettimeofday(&middle_time, NULL);
  //  iteratedWait(10000,&myReader1,allGets, allTrackedSets);

  int numIter = 0;
  while ((numIter < 1000) && (myReader1.numStillProcessing() != 0))
  {
    std::cout<<"\n\nThis is numIter:  "<<numIter<<"\n";
    std::cout<<"This is numStillProcessing:   "<<myReader1.numStillProcessing()<<"\n";
    std::cout<<"This is the size of allGets:  "<<allGets.size()<<"\n\n\n";
    std::cout.flush();

    usleep(100);

    iteratedWait(1000,&myReader1,allGets, allTrackedSets);
    ++numIter;
  }

  int allGetsSize = (int) allGets.size();
  std::cout<<"\n\nThis is size of allGets:   "<<allGets.size()<<"\n\n";

  std::cout<<"\n\nThis is response count:  ";
  int counter = myReader1.getRespCount();
  std::cout<<counter<<"\n\n\n\n";



  gettimeofday(&finished_time,NULL);
  double secondsElapsed  = finished_time.tv_sec - first_time.tv_sec;
  double uSecondsElapsed = finished_time.tv_usec - first_time.tv_usec;
  double realTimeTakenSeconds = secondsElapsed + uSecondsElapsed/(1000000);

  double middleSecondsElapsed = middle_time.tv_sec - first_time.tv_sec;
  double middleUSecondsElapsed = middle_time.tv_usec - first_time.tv_usec;
  double middleRealTimeTakenSeconds = middleSecondsElapsed + middleUSecondsElapsed/(1000000);



  //  for (int s=0; s < (int)allGets.size(); ++ s)
  //  {
  //    printResponse(allGets[s]);
  //  }


  std::cout<<"\n\nThis is timer final:  "<<realTimeTakenSeconds<<"\n\n";
  std::cout<<"\n\nThis is size of allGets:   "<<allGetsSize<<"\n\n";
  std::cout<<"\n\nThis is the time it took to do the middle section:  "<<middleRealTimeTakenSeconds<<"\n\n\n";

}


void runMultiUnLoadSpeed(int numTimesLoad)
{

  //declare reader
  AsyncCraqGet myReader1(NULL,NULL);


  //declare craq args
  std::vector<CraqInitializeArgs> craqArgs1;
  CraqInitializeArgs cInitArgs;
  cInitArgs.ipAdd   =     WHICH_SERVER;
  cInitArgs.port    =       WHICH_PORT;



  craqArgs1.push_back(cInitArgs);

  std::vector<CraqOperationResult*> allGets;
  std::vector<CraqOperationResult*> allTrackedSets;


  std::cout<<"\n\nAbout to perform reader initialization.\n\n";
  myReader1.initialize(craqArgs1);

  iteratedWait(10000,&myReader1,allGets, allTrackedSets);

  std::cout<<"\n\nAbout to perform first set of sets\n\n";
  //performing first sets

  iteratedWait(200000,&myReader1,allGets, allTrackedSets);

  //constructing queries
  std::vector<CraqDataSetGet*> allQueries;
  craftQueries(allQueries);


  //about to perform gets
  std::cout<<"\n\nAbout to perform a bunch of gets\n\n";
  std::cout.flush();
  int numgetting  = 0;

  usleep(100);


  //initializing timer
  struct timeval first_time, middle_time, finished_time;
  gettimeofday(&first_time,NULL);

  for (int a = 0; a < numTimesLoad; ++a)
  {
    for (int s=numInit; s < numInit+ numPuts; ++s)
    {
      myReader1.get(*(allQueries[s-numInit]));

      ++numgetting;
      if (numgetting %50 == 0)
      {
        iteratedWait(1,&myReader1,allGets,allTrackedSets);
      }

      //    else
      //      {
      //        usleep(1);
      //      }

    }
  }

  gettimeofday(&middle_time, NULL);
  //  iteratedWait(10000,&myReader1,allGets, allTrackedSets);

  int numIter = 0;
  while ((numIter < 1000) && (myReader1.numStillProcessing() != 0))
  {
    std::cout<<"\n\nThis is numIter:  "<<numIter<<"\n";
    std::cout<<"This is numStillProcessing:   "<<myReader1.numStillProcessing()<<"\n";
    std::cout<<"This is the size of allGets:  "<<allGets.size()<<"\n\n\n";
    std::cout.flush();

    usleep(100);

    iteratedWait(1000,&myReader1,allGets, allTrackedSets);
    ++numIter;
  }

  int allGetsSize = (int) allGets.size();
  std::cout<<"\n\nThis is size of allGets:   "<<allGets.size()<<"\n\n";



  gettimeofday(&finished_time,NULL);
  double secondsElapsed  = finished_time.tv_sec - first_time.tv_sec;
  double uSecondsElapsed = finished_time.tv_usec - first_time.tv_usec;
  double realTimeTakenSeconds = secondsElapsed + uSecondsElapsed/(1000000);

  double middleSecondsElapsed = middle_time.tv_sec - first_time.tv_sec;
  double middleUSecondsElapsed = middle_time.tv_usec - first_time.tv_usec;
  double middleRealTimeTakenSeconds = middleSecondsElapsed + middleUSecondsElapsed/(1000000);



  for (int s=0; s < (int)allGets.size(); ++ s)
  {
    printResponse(allGets[s]);
  }


  std::cout<<"\n\nThis is timer final:  "<<realTimeTakenSeconds<<"\n\n";
  std::cout<<"\n\nThis is size of allGets:   "<<allGetsSize<<"\n\n";
  std::cout<<"\n\nThis is the time it took to do the middle section:  "<<middleRealTimeTakenSeconds<<"\n\n\n";

}


void craftQueries(std::vector<CraqDataSetGet*>& allQueries)
{
  std::string query = "";
  for (int s=numInit; s < numInit+ numPuts; ++s)
  {
    query = "";
    std::stringstream ss;
    ss << s;
    std::string tmper = ss.str();

    for (int t=0; t< CRAQ_DATA_KEY_SIZE - (((int)tmper.size()) + 1); ++t)
    {
      query.append("0");
    }
    query.append(tmper);

    allQueries.push_back(new CraqDataSetGet(query,s,false,CraqDataSetGet::GET));
  }
}





void printResponse(CraqOperationResult* res)
{


  if (res->whichOperation == CraqOperationResult::GET)
  {
    //    printing a get response
    res->objID[CRAQ_DATA_KEY_SIZE-1] = '\0';

    if ((res->servID > 420) || (res->servID < 10))
    {
      std::cout<<"\n\nERROR value here serv:  "<< res->servID<<"\n\n\n";

      //      assert(false);
    }

    for (int a=0; a < 33; ++a)
    {
      if ((res->objID[a] != '0') && (res->objID[a] != '1') && (res->objID[a] != '2') && (res->objID[a] != '3') && (res->objID[a] != '4') && (res->objID[a] != '5') && (res->objID[a] != '6') && (res->objID[a] != '7') && (res->objID[a] != '8') && (res->objID[a] != '9'))
      {
        std::cout<<"\n\nERROR value here:  "<< res->objID<<"\n\n\n";
        std::cout<<"a:  "<<a<<"  "<<(int)res->objID[a]<<   "\n\n";
        assert(false);
      }
    }


    //need to re-comment
       std::cout<<"GETTING: \n";
       std::cout<<"\tObj ID:   "<<res->objID<<"      Server ID:  "<<res->servID<<"\n";
       if (!res->succeeded)
       {
         std::cout<<"\n\nWARNING.  WARNING.  FAIL.  FAIL.\n\n\n\n";
         exit(1);
       }
  }
  //  if(res.whichOperation == CraqOperationResult::SET)
  else
  {
    res->objID[CRAQ_DATA_KEY_SIZE-1] = '\0';
    std::cout<<"SETTING: \n";
    std::cout<<"\tObj ID:   "<<res->objID<<"      Server ID:  "<<res->servID<<"\n";
    if (!res->succeeded)
    {
      std::cout<<"\n\nWARNING.  WARNING.  FAIL.  FAIL.\n\n\n\n";
      exit(1);
    }
  }
}


void iteratedWait(int numWaits,AsyncCraqGet* myReader,std::vector<CraqOperationResult*> &allGetResults,std::vector<CraqOperationResult*>&allTrackedResults)
{
  for (int s=0; s < numWaits; ++s)
  {
    basicWait(myReader,allGetResults,allTrackedResults);
  }
}



void iteratedWaitMulti(int numWaits,AsyncCraqGet* myReader,std::vector<CraqOperationResult*> &allGetResults,std::vector<CraqOperationResult*>&allTrackedResults)
{
  for (int s=0; s < numWaits; ++s)
  {
    basicWaitMulti(myReader,allGetResults,allTrackedResults);
  }
}







void basicWait(AsyncCraqGet* myReader,std::vector<CraqOperationResult*> &allGetResults,std::vector<CraqOperationResult*>&allTrackedResults)
{
  std::vector<CraqOperationResult*> getResults;
  std::vector<CraqOperationResult*> trackedSetResults;

  myReader -> tick(getResults,trackedSetResults);

  allGetResults.insert(allGetResults.end(), getResults.begin(), getResults.end());
  allTrackedResults.insert(allTrackedResults.end(), trackedSetResults.begin(), trackedSetResults.end());
}


void basicWaitMulti(AsyncCraqGet* myReader,std::vector<CraqOperationResult*> &allGetResults,std::vector<CraqOperationResult*>&allTrackedResults)
{
  std::vector<CraqOperationResult*> getResults;
  std::vector<CraqOperationResult*> trackedSetResults;

  myReader -> tickMulti(getResults,trackedSetResults);

  allGetResults.insert(allGetResults.end(), getResults.begin(), getResults.end());
  allTrackedResults.insert(allTrackedResults.end(), trackedSetResults.begin(), trackedSetResults.end());
}

}
