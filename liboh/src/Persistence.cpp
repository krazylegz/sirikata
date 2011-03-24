#include <sirikata/oh/Platform.hpp>
#include <sirikata/oh/Persistence.hpp>
#include <sirikata/oh/ObjectHost.hpp>
#include <iostream>
#include <fstream>

using  namespace std;
namespace Sirikata
{

std::string* FilePersistenceHandle::read(PersistenceKey* _key)
{
  ifstream in(fileName_.c_str(), ios::binary);
  in.read((char*)map_, sizeof(*map_));
  in.close(); 

  std::string key = std::string((const char*)(_key->key()));
  map<std::string, std::string*>::iterator it = map_->find(key);
  if( it == map_->end())
  {
    
    return NULL;
  }
  return (*it).second;
}

void FilePersistenceHandle::write(PersistenceKey* _key, std::string* _value)
{
  std::string key = std::string((const char*)(_key->key()));
  map_->insert(std::pair<std::string, std::string*>(key, _value));
  writeMap();
}


void FilePersistenceHandle::writeMap()
{
  std::cout << "\nEnter FilePersistence::WriteMap()\n";
  map<std::string, std::string*>::iterator it = map_->begin();
  ofstream out(fileName_.c_str(), ios::binary);
  out.write((char*)map_, sizeof(*map_));
  out.close();
  std::cout << " \nExit FilePersistence::WriteMap()\n";
}

PersistenceKey* FilePersistenceHandle::key(std::string* _data)
{
  //create a PersistenceKey  out of this data  
  //take the first 16 bytes..that's it
  uint8_t* c = new uint8_t[17];
  memset(c, 0, 17);
  uint32_t s = _data->size();
  memcpy(c, _data->c_str(), (s <= 16)?s:16);
  return new PersistenceKey(c, 16);

}

PersistenceManager::PersistenceManager(ObjectHost* _objectHost, std::string* _moduleID)
{
  moduleID_ = _moduleID;
  handle_ = _objectHost->persistenceHandle();
}

void PersistenceManager::persist(std::string* _key, std::string* _value)
{
  std::string new_key = *_key;
  handle_->write(handle_->key(&new_key), _value);    
}

std::string* PersistenceManager::lookup(std::string* _key)
{
  std::string new_key = *_key;
  return handle_->read(handle_->key(&new_key));
}


}
