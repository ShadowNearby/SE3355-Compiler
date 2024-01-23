#ifndef TIGER_RUNTIME_GC_ROOTS_H
#define TIGER_RUNTIME_GC_ROOTS_H

#include "tiger/frame/frame.h"
#include <iostream>
#include <utility>

namespace gc {
const std::string GC_ROOTS = "GLOBAL_GC_ROOTS";

struct PointerMapNode {
  std::string label{};
  std::string next{};
  std::string key{};
  std::string frame_size{};
  std::vector<int> offsets{};
  std::string end_map{"-1"};
};

class PointerMap {
public:
  explicit PointerMap(std::vector<gc::PointerMapNode *> nodes)
      : nodes_(std::move(nodes)){};
  std::vector<gc::PointerMapNode *> nodes_{};
  void Merge(const PointerMap &map) {
    nodes_.insert(nodes_.end(), map.nodes_.begin(), map.nodes_.end());
  }
  void Merge(const PointerMap &&map) {
    nodes_.insert(nodes_.end(), map.nodes_.begin(), map.nodes_.end());
  }
  void Build() {
    nodes_.back()->end_map = "0";
    nodes_.back()->next = "0";
    for (int i = 0; i < nodes_.size() - 1; ++i) {
      nodes_[i]->next = nodes_[i + 1]->label;
    }
  }
  void Print(FILE *out) {
    for (const auto map : nodes_) {
      fprintf(out, "%s:\n", map->label.c_str());
      fprintf(out, ".quad %s\n", map->next.c_str());
      fprintf(out, ".quad %s\n", map->key.c_str());
      fprintf(out, ".quad %s\n", map->frame_size.c_str());
      for (const auto &offset : map->offsets) {
        fprintf(out, ".quad %d\n", offset);
      }
      fprintf(out, ".quad %s\n", map->end_map.c_str());
    }
  }
};

class Roots {
  // Todo(lab7): define some member and methods here to keep track of gc roots;
private:
  frame::Frame *frame_;
  assem::InstrList *instr_list_;

public:
  Roots(frame::Frame *frame, assem::InstrList *instr_list)
      : frame_(frame), instr_list_(instr_list) {}
  gc::PointerMap GeneratePointerMaps() {
    std::vector<PointerMapNode *> nodes;
    bool generate = false;
    for (const auto instr : instr_list_->GetList()) {
      if (!generate && typeid(*instr) == typeid(assem::OperInstr)) {
        std::string ass = dynamic_cast<assem::OperInstr *>(instr)->assem_;
        if (ass.find("call") == std::string::npos) {
          generate = false;
          continue;
        }
        generate = true;
        continue;
      }
      if (generate && typeid(*instr) == typeid(assem::LabelInstr)) {
        auto key = dynamic_cast<assem::LabelInstr *>(instr)->label_->Name();
        std::vector<int> offsets{};
        for (const auto access : frame_->pointers_) {
          auto frame_access = dynamic_cast<frame::InFrameAccess *>(access);
          offsets.emplace_back(frame_access->offset);
        }
        auto node = new PointerMapNode{
            "L" + key, "", key, frame_->GetLabel() + "_framesize", offsets};
        nodes.emplace_back(node);
      }
    }
    return gc::PointerMap{nodes};
  }
};

} // namespace gc

#endif // TIGER_RUNTIME_GC_ROOTS_H