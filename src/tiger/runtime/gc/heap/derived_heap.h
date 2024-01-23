#pragma once

#include "heap.h"
#include <algorithm>
#include <map>
#include <vector>

namespace gc {

struct PointerMap {
  uint64_t label;
  uint64_t next;
  uint64_t key;
  uint64_t frame_size;
  std::vector<int64_t> offsets;
};

class FreeList {
private:
  struct Node {
    char *start{nullptr};
    uint64_t size{};
  };
  std::vector<Node> data;

public:
  FreeList() {}
  void Init(char *heap, uint64_t size) { data.emplace_back(Node{heap, size}); }
  char *Allocate(uint64_t size) {
    if (data.empty()) {
      return nullptr;
    }
    if (size > MaxFree()) {
      return nullptr;
    }
    char *ret_add;
    for (auto it = data.begin(); it != data.end(); ++it) {
      if (it->size >= size) {
        // HEAP_LOG("Need size %lu, select %lu", size, (*it).size)
        ret_add = it->start;
        auto rest_size = it->size - size;
        char *new_start = it->start + size;
        // remove current node from list
        data.erase(it);

        // add rest part to freelist
        if (rest_size > 0) {
          data.push_back({new_start, rest_size});
        }
        return ret_add;
      }
    }
    return nullptr;
  }
  uint64_t SumFree() const {
    uint64_t sum = 0;
    std::for_each(data.begin(), data.end(),
                  [&](const auto &item) { sum += item.size; });
    return sum;
  }
  uint64_t MaxFree() const {
    auto max_it = std::max_element(
        data.begin(), data.end(),
        [](const Node &l, const Node &r) { return l.size < r.size; });
    return (*max_it).size;
  }
  void AddFree(char *start, uint64_t size) {
    data.emplace_back(Node{start, size});
  }
};

class Info {

public:
  enum class Type { Record, Array };
  Info(char *start, uint64_t size, Type type)
      : start_(start), size_(size), type_(type){};
  void SetMark() { mark_ = true; }
  void UnMark() { mark_ = false; }
  bool Match(uint64_t address) const { return address == (uint64_t)start_; }

  char *start_{nullptr};
  bool mark_{};
  uint64_t size_{};
  Type type_;
};

class RecordInfo : public Info {
private:
public:
  unsigned char *descriptor_{nullptr};
  uint64_t descriptor_size_{};
  RecordInfo(char *start, uint64_t size)
      : Info(start, size, Info::Type::Record) {}
  RecordInfo(char *start, uint64_t size, unsigned char *descriptor,
             uint64_t descriptor_size)
      : Info(start, size, Info::Type::Record), descriptor_(descriptor),
        descriptor_size_(descriptor_size) {}
};

class ArrayInfo : public Info {
private:
public:
  ArrayInfo(char *start, uint64_t size)
      : Info(start, size, Info::Type::Array) {}
};

class DerivedHeap : public TigerHeap {
  // TODO(lab7): You need to implement those interfaces inherited from TigerHeap
  // correctly according to your design.
private:
  void DFS(uint64_t x);

  void Mark();

  void Sweep();

  void ConstructPointerMaps();

  std::vector<uint64_t> GetRoots(uint64_t *sp);

  uint64_t size_{0};
  std::map<uint64_t, Info *> infos_{};
  std::map<uint64_t, PointerMap *> pointer_maps_{};
  FreeList free_list_{};

public:
  char *Allocate(uint64_t size) override;

  char *AllocRecord(uint64_t size, unsigned char *descriptor,
                    uint64_t descriptor_size) override;

  char *AllocArray(uint64_t size) override;

  bool CheckPointer(uint64_t x);

  Info *GetInfo(uint64_t x);

  uint64_t Used() const override;

  uint64_t MaxFree() const override;

  void Initialize(uint64_t size) override;

  void GC() override;
};

} // namespace gc
