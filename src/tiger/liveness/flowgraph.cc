#include "tiger/liveness/flowgraph.h"

namespace fg {

void FlowGraphFactory::AssemFlowGraph() {
  FNode *prev_node = nullptr;
  FNode *curr_node = nullptr;
  assem::Instr *prev_instr = nullptr;
  for (const auto instr : instr_list_->GetList()) {
    curr_node = flowgraph_->NewNode(instr);
    if (prev_instr != nullptr) {
      if (typeid(*prev_instr) == typeid(assem::OperInstr)) {
        auto oper_instr = dynamic_cast<assem::OperInstr *>(prev_instr);
        if (oper_instr->jumps_ == nullptr ||
            oper_instr->jumps_->labels_->empty()) {
          flowgraph_->AddEdge(prev_node, curr_node);
        }
      }
      if (typeid(*prev_instr) == typeid(assem::LabelInstr)) {
        auto label_instr = dynamic_cast<assem::LabelInstr *>(prev_instr);
        label_map_->Enter(label_instr->label_, prev_node);
        flowgraph_->AddEdge(prev_node, curr_node);
      }
      if (typeid(*prev_instr) == typeid(assem::MoveInstr)) {
        flowgraph_->AddEdge(prev_node, curr_node);
      }
    }
    prev_node = curr_node;
    prev_instr = instr;
  }
  for (const auto node : flowgraph_->Nodes()->GetList()) {
    auto instr = node->NodeInfo();
    if (typeid(*instr) != typeid(assem::OperInstr)) {
      continue;
    }
    auto oper_instr = dynamic_cast<assem::OperInstr *>(instr);
    if (oper_instr->jumps_ == nullptr || oper_instr->jumps_->labels_->empty()) {
      continue;
    }
    auto labels = *oper_instr->jumps_->labels_;
    for (const auto jump_to : labels) {
      auto jump_to_node = label_map_->Look(jump_to);
      flowgraph_->AddEdge(node, jump_to_node);
    }
  }
}

} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const { return new temp::TempList{}; }

temp::TempList *MoveInstr::Def() const {
  return dst_ ? dst_ : new temp::TempList{};
}

temp::TempList *OperInstr::Def() const {
  return dst_ ? dst_ : new temp::TempList{};
}

temp::TempList *LabelInstr::Use() const { return new temp::TempList{}; }

temp::TempList *MoveInstr::Use() const {
  return src_ ? src_ : new temp::TempList{};
}

temp::TempList *OperInstr::Use() const {
  return src_ ? src_ : new temp::TempList{};
}
} // namespace assem
