/*  cbr
 *  ServerMessageReceiver.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#include "ServerMessageReceiver.hpp"
#include "SpaceContext.hpp"
#include "Network.hpp"

namespace CBR {

ServerMessageReceiver::ServerMessageReceiver(SpaceContext* ctx, Network* net, Listener* listener)
        : mContext(ctx),
          mReceiverStrand(ctx->ioService->createStrand()),
          mNetwork(net),
          mListener(listener),
          mTotalWeightSum(0.0),
          mCapacityEstimator(Duration::milliseconds((int64)200).toSeconds())
{
    mProfiler = mContext->profiler->addStage("Server Message Receiver");
    net->listen(mContext->id(), this);
}

ServerMessageReceiver::~ServerMessageReceiver() {
}

double ServerMessageReceiver::totalWeight() {
    return mTotalWeightSum;
}

double ServerMessageReceiver::capacity() {
    return mCapacityEstimator.get();
}

void ServerMessageReceiver::updateSenderStats(ServerID sid, double total_weight, double used_weight) {
    // FIXME we add things in here but we have no removal process
    WeightMap::iterator weight_it = mTotalWeights.find(sid);
    float old_total_weight = weight_it == mTotalWeights.end() ? 0.0 : weight_it->second;
    mTotalWeights[sid] = total_weight;
    mTotalWeightSum += (total_weight - old_total_weight);

    mReceiverStrand->post(
        std::tr1::bind(
            &ServerMessageReceiver::handleUpdateSenderStats, this,
            sid, total_weight, used_weight
        )
    );
}

void ServerMessageReceiver::updatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_segmentation) {
    NOT_IMPLEMENTED();
}

} // namespace CBR
