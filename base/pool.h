#pragma once

#include <cassert>
#include <map>
#include <memory>

template <class Key, class Payload>
class Pool;

template <class Key, class Payload>
class PoolItem : public std::enable_shared_from_this<Payload> {
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

  Pool(const Pool&) = delete;
  Pool& operator=(const Pool&) = delete;

  std::shared_ptr<Payload> Get(const Key& key) {
    auto p = payload_map_.try_emplace(key);
    Payload*& payload = p.first->second;
    if (!payload) {
      auto shared = std::make_shared<Payload>(key);
      payload = shared.get();
      payload->pool_ = this;
      payload->pos_ = p.first;
      return shared;
    }
    return std::static_pointer_cast<Payload>(payload->shared_from_this());
  }

  typedef std::map<Key, Payload*> PayloadMap;
  PayloadMap payload_map_;
};

template <class Key, class Payload>
inline PoolItem<Key, Payload>::~PoolItem() {
  if (pool_)
    pool_->payload_map_.erase(pos_);
}
