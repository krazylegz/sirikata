#ifndef _SIRIKATA_PERSISTENCE_HPP_
#define _SIRIKATA_PERSISTENCE_HPP_



#include <sirikata/oh/Platform.hpp>
#include <sirikata/oh/ObjectHost.hpp>
#include <string>

using namespace std;

namespace Sirikata
{


class  SIRIKATA_OH_EXPORT PersistenceKey
{
  uint8_t* key_;
  uint32_t size_;
  public:
  PersistenceKey(uint8_t* _key, uint32_t _size) : key_(_key), size_(_size){}
  inline uint8_t* key() { return key_;}
  inline uint32_t size() { return size_;}
};

class SIRIKATA_OH_EXPORT PersistenceHandle
{
  
  public:
  virtual ~PersistenceHandle(){}

  virtual std::string* read(PersistenceKey*){ return NULL; }
  virtual void write(PersistenceKey*, std::string*){}
  virtual PersistenceKey* key(std::string*){ return NULL;}
  
  
};


class SIRIKATA_OH_EXPORT PersistenceManager
{

  ObjectHost* objectHost_; 
  PersistenceHandle* handle_;
  std::string* moduleID_;  
  
  public:
  PersistenceManager(ObjectHost* _objectHost, std::string* _moduleID);
  inline PersistenceHandle* handle() { return handle_;}
  inline std::string* moduleID() { return moduleID_;}
  
  void persist(std::string*, std::string*);
  
  std::string* lookup(std::string*);

};


// one simple PersistenceHandle

class SIRIKATA_OH_EXPORT FilePersistenceHandle : public PersistenceHandle
{
  std::string fileName_;
  map<std::string, std::string*>* map_;
  void writeMap();
  public:
  FilePersistenceHandle(std::string _fileName) : fileName_(_fileName){ map_ = new map<std::string, std::string*>();}
  std::string* read(PersistenceKey*);
  void write(PersistenceKey*, std::string*);
  PersistenceKey* key(std::string*);
};


}



#endif
