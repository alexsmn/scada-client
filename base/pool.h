#pragma once

#include "base/memory/ref_counted.h"

#include <map>

template<class Key, class Payload> class Pool;

template<class Key, class Payload>
class PoolItem : public base::RefCounted<Payload> {
 public:
  PoolItem() : pool_(NULL) {}
  ~PoolItem();

  Pool<Key, Payload>* pool_;
  typename std::map<Key, Payload*>::iterator pos_;
};

template<class Key, class Payload>
class Pool {
 public:
  Pool() {}

  scoped_refptr<Payload> Get(const Key& key) {
    std::pair<PayloadMap::iterator, bool> p =
        payload_map_.insert(PayloadMap::value_type(
            key, static_cast<Payload*>(NULL)));
    Payload*& payload = p.first->second;
    if (!payload) {
      payload = new Payload(key);
      payload->pool_ = this;
      payload->pos_ = p.first;
    }
    return scoped_refptr<Payload>(payload);
  }

  typedef std::map<Key, Payload*> PayloadMap;
  PayloadMap payload_map_;

  DISALLOW_COPY_AND_ASSIGN(Pool);
};

template<class Key, class Payload>
PoolItem<Key, Payload>::~PoolItem() {
  if (pool_)
    pool_->payload_map_.erase(pos_);
}
