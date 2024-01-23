#include "derived_heap.h"
#include <chrono>
#include <cstring>
#include <memory>
#include <stack>
#include <stdio.h>

namespace gc {
// TODO(lab7): You need to implement those interfaces inherited from TigerHeap
// correctly according to your design.

char *DerivedHeap::Allocate(uint64_t size) { return free_list_.Allocate(size); }

char *DerivedHeap::AllocRecord(uint64_t size, unsigned char *descriptor,
                               uint64_t descriptor_size) {
  //  printf("alloc record %lu %lu %s\n", size, descriptor_size, descriptor);
  char *start = Allocate(size);
  if (start == nullptr) {
    return nullptr;
  }
  auto info = new RecordInfo(start, size, descriptor, descriptor_size);
  infos_[(uint64_t)info->start_] = info;
  return start;
}

char *DerivedHeap::AllocArray(uint64_t size) {
  //  printf("alloc array %lu\n", size);
  char *start = Allocate(size);
  if (start == nullptr) {
    return nullptr;
  }
  auto info = new ArrayInfo(start, size);
  infos_[(uint64_t)info->start_] = info;
  return start;
}

void DerivedHeap::DFS(uint64_t x) {
  // if x is a pointer into the heap
  //    if record x is not marked
  //      mark x
  //      for each field fi of record x
  //        DFS(x.fi)

  auto info = GetInfo(x);
  if (info == nullptr) {
    return;
  }
  if (info->mark_) {
    return;
  }
  info->SetMark();
  if (info->type_ != Info::Type::Record) {
    return;
  }
  auto record_info = (RecordInfo *)(info);
  auto start = (uint64_t)record_info->start_;
  for (int i = 0; i < record_info->descriptor_size_; ++i) {
    if (record_info->descriptor_[i] != '1') {
      continue;
    }
    auto field = *(uint64_t *)(start + WORD_SIZE * i);
    DFS(field);
  }
}

void DerivedHeap::Mark() {
  auto roots = GetRoots(stack);
  for (const auto root : roots) {
    DFS(root);
  }
}

void DerivedHeap::Sweep() {
  // p ← first address in heap
  // while p < last address in heap
  //     if record p is marked
  //	unmark p
  //     else let f1 be the first field in p
  //	p.f1 ← freelist
  //	freelist ← p
  //     p ← p + (size of record p)
  //  std::cout << "do sweep" << std::endl;
  auto exist_infos = std::map<uint64_t, Info *>{};
  for (const auto [key, info] : infos_) {
    if (info == nullptr) {
      continue;
    }
    if (info->mark_) {
      exist_infos[(uint64_t)info->start_] = info;
      info->UnMark();
      continue;
    }
    free_list_.AddFree(info->start_, info->size_);
  }
  infos_ = exist_infos;
  return;
}

Info *DerivedHeap::GetInfo(uint64_t x) { return infos_[x]; }

void DerivedHeap::ConstructPointerMaps() {
  auto start = &GLOBAL_GC_ROOTS;
  auto tmp = start;
  int64_t offset;
  while (true) {
    auto point_map_node = new PointerMap;
    point_map_node->label = (uint64_t)tmp;
    point_map_node->next = *tmp;
    ++tmp;
    point_map_node->key = *tmp;
    ++tmp;
    point_map_node->frame_size = *tmp;
    ++tmp;
    while (true) {
      offset = *tmp;
      ++tmp;
      if (offset == -1 || offset == 0) {
        break;
      }
      point_map_node->offsets.emplace_back(offset);
    }
    pointer_maps_[point_map_node->key] = point_map_node;
    if (offset == 0) {
      break;
    }
  }
}

std::vector<uint64_t> DerivedHeap::GetRoots(uint64_t *sp) {
  std::vector<uint64_t> roots;
  while (true) {
    uint64_t return_address = *(sp - 1);
    auto pointer_map = pointer_maps_[return_address];
    for (const auto offset : pointer_map->offsets) {
      uint64_t *pointer_address =
          (uint64_t *)(offset + (int64_t)sp + (int64_t)pointer_map->frame_size);
      roots.emplace_back(*pointer_address);
    }
    sp += (pointer_map->frame_size / WORD_SIZE + 1);
    if (sp == main_stack) {
      return roots;
    }
  }
}

uint64_t DerivedHeap::Used() const { return size_ - free_list_.SumFree(); }

uint64_t DerivedHeap::MaxFree() const { return free_list_.MaxFree(); }

void DerivedHeap::Initialize(uint64_t size) {
  size_ = size;
  heap = (char *)malloc(size);
  free_list_.Init(heap, size);
  ConstructPointerMaps();
}

void DerivedHeap::GC() {
  Mark();
  Sweep();
}
} // namespace gc
