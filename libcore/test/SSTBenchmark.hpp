/*  cbr
 *  TimerSpeedBenchmark.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#ifndef _CBR_SST_BENCHMARK_HPP_
#define _CBR_SST_BENCHMARK_HPP_

#include "network/Stream.hpp"
#include "network/StreamListener.hpp"

namespace Sirikata {

/** TimerSpeedBenchmark tests the cost of Timer calls, i.e. how many we can call
 *   per second.
 */
class SSTBenchmark{
  public:
    typedef std::tr1::function<void()> FinishedCallback;

    static SSTBenchmark* create(const FinishedCallback& finished_cb, const String& param, int argc, char **argv=NULL) {
        return new SSTBenchmark(finished_cb,param,argc,argv);
    }

    SSTBenchmark(const FinishedCallback& finished_cb, const String& param, int argc, char**argv=NULL);

    String name();

    void start();
    void stop();

  private:
    std::tr1::function<void()>mPingFunction;
    void pingPoller();
    void connected(Sirikata::Network::Stream::ConnectionStatus,const std::string&reason);
    void remoteConnected(Sirikata::Network::Stream*strm,Sirikata::Network::Stream::ConnectionStatus,const std::string&reason);
    Sirikata::Network::Stream::ReceivedResponse computePingTime(Sirikata::Network::Chunk&chk);
    Sirikata::Network::Stream::ReceivedResponse bouncePing(Sirikata::Network::Stream*, Sirikata::Network::Chunk&chk);
    void newStream(Sirikata::Network::Stream*newStream, Sirikata::Network::Stream::SetCallbacks&cb);
    bool mForceStop;
    bool mOrdered;
    Duration mPingRate;
    size_t mPingSize;
    size_t mNumPings;
    String mStreamOptions;
    String mListenOptions;
    String mStreamPlugin;
    String mPort;
    String mHost;
    Sirikata::Network::IOService*mIOService;
    Sirikata::Network::Stream *mStream;
    Sirikata::Network::StreamListener *mListener;
    Time mStartTime;
    Sirikata::uint64 mPingsSent;
    Sirikata::uint64 mPingsReceived;
    
    std::vector<Time> mOutstandingPings;
    std::vector<Duration> mPingResponses;
}; // class TimerSpeedBenchmark

} // namespace CBR

#endif //_CBR_SST_BENCHMARK_HPP_