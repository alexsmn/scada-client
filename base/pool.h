#pragma once

#include "base/memory/ref_counted.h"

#include <map>

template <class Key, class Payload>
class Pool;

template <class Key, class Payload>
class PoolItem : public base::RefCounted<Payload> {
 public:
  PoolItem() {}
  ~PoolItem();

  Pool<Key, Payload>* pool_ = nullptr;
  typename std::map<Key, Payload*>::iterator pos_;
};

template <class Key, class Payload>
class Pool {
 public:
  Pool() {}
  ~Pool() { assert(payload_map_.empty()); }

  scoped_refptr<Payload> Get(const Key& key) {
    auto p = payload_map_.try_emplace(key);
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

template <class Key, class Payload>
inline PoolItem<Key, Payload>::~PoolItem() {
  if (pool_)
    pool_->payload_map_.erase(pos_);
}
