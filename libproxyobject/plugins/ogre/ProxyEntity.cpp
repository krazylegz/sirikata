/*  Sirikata Graphical Object Host
 *  Entity.cpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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

#include "ProxyEntity.hpp"
#include <sirikata/ogre/OgreRenderer.hpp>

using namespace Sirikata::Transfer;

namespace Sirikata {
namespace Graphics {

ProxyEntityListener::~ProxyEntityListener() {
}

ProxyEntity::ProxyEntity(OgreRenderer *scene, const ProxyObjectPtr &ppo)
 : Entity(scene, ppo->getObjectReference().toString()),
   mProxy(ppo)
{
    ppo->ProxyObjectProvider::addListener(this);
    ppo->PositionProvider::addListener(this);
    ppo->MeshProvider::addListener(this);

    mDestroyTimer = Network::IOTimer::create(
        mScene->context()->ioService,
        std::tr1::bind(&ProxyEntity::handleDestroyTimeout, this)
    );
}

ProxyEntity::~ProxyEntity() {
    getProxy().MeshProvider::removeListener(this);

    getProxy().ProxyObjectProvider::removeListener(this);
    getProxy().PositionProvider::removeListener(this);
}

BoundingSphere3f ProxyEntity::bounds() {
    return getProxy().getBounds();
}

float32 ProxyEntity::priority() {
    return mProxy->priority;
}

void ProxyEntity::tick(const Time& t, const Duration& deltaTime) {
    // Update location from proxy as well as doing normal updates
    Entity::tick(t, deltaTime);
    extrapolateLocation(t);
}

bool ProxyEntity::isDynamic() const {
    return Entity::isDynamic() || !getProxy().isStatic();
}

ProxyEntity *ProxyEntity::fromMovableObject(Ogre::MovableObject *movable) {
    return static_cast<ProxyEntity*>( Entity::fromMovableObject(movable) );
}

void ProxyEntity::updateLocation(const TimedMotionVector3f &newLocation, const TimedMotionQuaternion& newOrient, const BoundingSphere3f& newBounds) {
    SILOG(ogre,detailed,"UpdateLocation "<<this<<" to "<<newLocation.position()<<"; "<<newOrient.position());
    setOgrePosition(Vector3d(newLocation.position()));
    setOgreOrientation(newOrient.position());
    updateScale( newBounds.radius() );
    checkDynamic();
}

void ProxyEntity::validated() {
    mDestroyTimer->cancel();
    processMesh( mProxy->getMesh() );
}

void ProxyEntity::invalidated() {
    // To mask very quick removal/addition sequences, defer unloading
    mDestroyTimer->wait(Duration::seconds(1));
}

void ProxyEntity::handleDestroyTimeout() {
    unloadMesh();
}

void ProxyEntity::destroyed() {
    Provider<ProxyEntityListener*>::notify(&ProxyEntityListener::proxyEntityDestroyed, this);
    delete this;
}

void ProxyEntity::extrapolateLocation(TemporalValue<Location>::Time current) {
    Location loc (getProxy().extrapolateLocation(current));
    setOgrePosition(loc.getPosition());
    setOgreOrientation(loc.getOrientation());
}

/////////////////////////////////////////////////////////////////////
// overrides from MeshListener
// MCB: integrate these with the MeshObject model class

void ProxyEntity::onSetMesh (ProxyObjectPtr proxy, Transfer::URI const& meshFile )
{

}

void ProxyEntity::onSetScale (ProxyObjectPtr proxy, float32 scale )
{
    updateScale(scale);
}


}
}
