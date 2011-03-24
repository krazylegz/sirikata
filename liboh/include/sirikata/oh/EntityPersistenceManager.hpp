#ifndef _ENTITY_PERSISTENCE_MANAGER_HPP_
#define _ENTITY_PERSISTENCE_MANAGER_HPP_


#include <sirikata/oh/Persistence.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/oh/ObjectHost.hpp>
#include <sirikata/core/util/MotionVector.hpp>
#include "Protocol_EntityPersistence.pbj.hpp"

#include <string>

namespace Sirikata
{

class EntityPersistenceManager : public PersistenceManager
{
  private:
  HostedObject* hostedObject_;

  void getTimedMotionVector(SpaceObjectReference&, Protocol::ITimedMotionVector&);
  void getPerPresenceData(SpaceObjectReference&, EntityPersistence::IPerPresenceData&);
  public:
  EntityPersistenceManager(ObjectHost*, std::string*);

  std::string* serializeEntity(HostedObject*);

  HostedObject* deserializeEntity();

  void persistLocation(SpaceObjectReference&);

  void persistPresenceData(SpaceObjectReference&);

  void persistScriptData(ObjectScript*);

};


}


#endif
