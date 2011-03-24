#include <sirikata/oh/EntityPersistenceManager.hpp>
#include "Protocol_EntityPersistence.pbj.hpp"
#include <sirikata/core/util/MotionVector.hpp>
#include <string>


namespace Sirikata
{

EntityPersistenceManager::EntityPersistenceManager(ObjectHost* _objectHost, std::string* _moduleID) : PersistenceManager(_objectHost, _moduleID)
{
 
}

void EntityPersistenceManager::getTimedMotionVector(SpaceObjectReference& _sporef, Protocol::ITimedMotionVector& tm)
{
  ProxyObjectPtr poptr = hostedObject_->self(_sporef);
  Vector3f curVel = (Vector3f)(poptr->getVelocity());
  Vector3f curPos = (Vector3f)(hostedObject_->requestCurrentPosition(_sporef.space(), _sporef.object()));
  tm.set_velocity(curVel);
  tm.set_position(curPos);
  tm.set_t(hostedObject_->currentLocalTime());

}


void EntityPersistenceManager::persistLocation(SpaceObjectReference& _sporef)
{
  Protocol::TimedMotionVector tm;

  getTimedMotionVector(_sporef, tm);

  std::string serialized_loc;
  tm.SerializeToString(&serialized_loc);

  //write this out
  std::string key = _sporef.toString() ;
  persist(&key, &serialized_loc);
}



void EntityPersistenceManager::persistScriptData(ObjectScript* _objectScript)
{

}


void EntityPersistenceManager::getPerPresenceData(SpaceObjectReference& _sporef, EntityPersistence::IPerPresenceData& ppd)
{
  ProxyObjectPtr poptr = hostedObject_->self(_sporef);
  
    //mesh
  
  std::string mesh = poptr->getMesh().toString();

  //space id

  std::string spaceid = _sporef.space().toString();

  //objectref

  std::string objref = _sporef.object().toString();



  ppd.set_spaceid(spaceid);
  ppd.set_objectref(objref);
  ppd.set_mesh(mesh);

  Protocol::ITimedMotionVector tm = ppd.mutable_location();
  getTimedMotionVector(_sporef, tm);

}


void EntityPersistenceManager::persistPresenceData(SpaceObjectReference& _sporef)
{
  //persist mesh, location, spaceobject ref

  //location
  EntityPersistence::PerPresenceData ppd;
  getPerPresenceData(_sporef, ppd);

  std::string serialized_ppd;
  ppd.SerializeToString(&serialized_ppd);

  //write this out
  std::string key = _sporef.toString() ;
  persist(&key, &serialized_ppd);
   
}



std::string* EntityPersistenceManager::serializeEntity(HostedObject* _hostedObject)
{
  EntityPersistence::PersistedEntity pe;
  HostedObject::SpaceObjRefVec ss;

  _hostedObject->getSpaceObjRefs(ss);

  HostedObject::SpaceObjRefVec::iterator it = ss.begin();
  
  for(; it != ss.end(); it++)
  {
    SpaceObjectReference sporef = *it;
    ProxyObjectPtr poptr = _hostedObject->self(sporef);
    EntityPersistence::IPersistenceData ppd = pe.add_presences_data();

    getPerPresenceData(sporef, ppd);
    
  }

  // Do something for script data


  std::string serialized_entity;
  pe.SerializetoString(&serialized_entity);

  //write this out
  
  


}

HostedObject* EntityPersistenceManager::deserializeEntity()
{
  return NULL;
}


}
