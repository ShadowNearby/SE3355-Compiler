#include "tiger/liveness/liveness.h"
#include "tiger/util/log.h"

extern frame::RegManager *reg_manager;

namespace live {

bool MoveList::Contain(INodePtr src, INodePtr dst) {
  return std::any_of(move_list_.cbegin(), move_list_.cend(),
                     [src, dst](std::pair<INodePtr, INodePtr> move) {
                       return move.first == src && move.second == dst;
                     });
}

void MoveList::Delete(INodePtr src, INodePtr dst) {
  assert(src && dst);
  auto move_it = move_list_.begin();
  for (; move_it != move_list_.end(); move_it++) {
    if (move_it->first == src && move_it->second == dst) {
      break;
    }
  }
  move_list_.erase(move_it);
}

MoveList *MoveList::Union(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : move_list_) {
    res->move_list_.push_back(move);
  }
  for (auto move : list->GetList()) {
    if (!res->Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

MoveList *MoveList::Intersect(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

std::set<temp::Temp *> MakeSet(temp::TempList *list) {
  std::set<temp::Temp *> set;
  for (const auto temp : list->GetList()) {
    set.insert(temp);
  }
  return set;
}

std::set<temp::Temp *> MakeSet(const std::list<temp::Temp *> &list) {
  std::set<temp::Temp *> set;
  for (const auto temp : list) {
    set.insert(temp);
  }
  return set;
}

temp::TempList *MakeList(const std::set<temp::Temp *> &set) {
  auto list = new temp::TempList{};
  for (const auto item : set) {
    list->Append(item);
  }
  return list;
}

void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */
  for (const auto node : flowgraph_->Nodes()->GetList()) {
    in_->Enter(node, new temp::TempList{});
    out_->Enter(node, new temp::TempList{});
  }

  bool finish = false;
  while (!finish) {
    finish = true;
    for (const auto curr_node : flowgraph_->Nodes()->GetList()) {
      auto curr_instr = curr_node->NodeInfo();
      auto curr_def_list = curr_instr->Def();
      auto curr_use_list = curr_instr->Use();
      auto curr_out_set = std::set<temp::Temp *>{};
      for (const auto succ_node : curr_node->Succ()->GetList()) {
        auto succ_in_list = in_->Look(succ_node);
        auto succ_in_set = MakeSet(succ_in_list);
        curr_out_set.merge(succ_in_set);
      }
      auto curr_in_set = std::set<temp::Temp *>{};
      auto prev_out_set = MakeSet(out_->Look(curr_node));
      auto curr_def_set = MakeSet(curr_def_list);
      auto curr_use_set = MakeSet(curr_use_list);
      auto diff_set = std::set<temp::Temp *>{};
      std::set_difference(prev_out_set.begin(), prev_out_set.end(),
                          curr_def_set.begin(), curr_def_set.end(),
                          std::inserter(diff_set, diff_set.begin()));
      curr_in_set.merge(curr_use_set);
      curr_in_set.merge(diff_set);
      auto prev_in_set = MakeSet(in_->Look(curr_node));
      if (prev_in_set != curr_in_set || prev_out_set != curr_out_set) {
        finish = false;
        in_->Set(curr_node, MakeList(curr_in_set));
        out_->Set(curr_node, MakeList(curr_out_set));
      }
    }
  }
}

void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */
  auto precolored_list = reg_manager->Registers()->GetList();
  auto interf_graph = live_graph_.interf_graph;
  for (const auto precolored_temp : precolored_list) {
    auto precolored_node = interf_graph->NewNode(precolored_temp);
    temp_node_map_->Enter(precolored_temp, precolored_node);
  }
  for (const auto left_temp : precolored_list) {
    for (const auto right_temp : precolored_list) {
      if (left_temp == right_temp) {
        continue;
      }
      auto left_node = temp_node_map_->Look(left_temp);
      auto right_node = temp_node_map_->Look(right_temp);
      interf_graph->AddEdge(left_node, right_node);
    }
  }

  auto flowgraph_node_list = flowgraph_->Nodes()->GetList();
  for (const auto curr_node : flowgraph_node_list) {
    auto instr = curr_node->NodeInfo();
    for (const auto temp : instr->Def()->GetList()) {
      if (temp_node_map_->Look(temp)) {
        continue;
      }
      auto node = interf_graph->NewNode(temp);
      temp_node_map_->Enter(temp, node);
    }
    for (const auto temp : instr->Use()->GetList()) {
      if (temp_node_map_->Look(temp)) {
        continue;
      }
      auto node = interf_graph->NewNode(temp);
      temp_node_map_->Enter(temp, node);
    }
  }
  flowgraph_node_list.reverse();
  for (const auto curr_node : flowgraph_node_list) {
    auto instr = curr_node->NodeInfo();
    auto def_list = instr->Def()->GetList();
    auto use_list = instr->Use()->GetList();
    auto out_list = out_->Look(curr_node)->GetList();
    if (typeid(*instr) != typeid(assem::MoveInstr)) {
      for (const auto def_temp : def_list) {
        auto def_node = temp_node_map_->Look(def_temp);
        for (const auto out_temp : out_list) {
          auto out_node = temp_node_map_->Look(out_temp);
          if (out_node == def_node) {
            continue;
          }
          //          LOG_DEBUG("add edge %d %d", def_node->NodeInfo()->Int(),
          //                    out_node->NodeInfo()->Int());
          interf_graph->AddEdge(def_node, out_node);
          interf_graph->AddEdge(out_node, def_node);
        }
      }
      continue;
    }
    for (const auto def_temp : def_list) {
      auto def_node = temp_node_map_->Look(def_temp);
      auto use_set = MakeSet(use_list);
      auto out_set = MakeSet(out_list);
      auto diff_set = std::set<temp::Temp *>{};
      std::set_difference(out_set.begin(), out_set.end(), use_set.begin(),
                          use_set.end(),
                          std::inserter(diff_set, diff_set.begin()));
      for (const auto diff_temp : diff_set) {
        auto diff_node = temp_node_map_->Look(diff_temp);
        if (diff_node == def_node) {
          continue;
        }
        //        LOG_DEBUG("add edge %d %d", def_node->NodeInfo()->Int(),
        //                  diff_node->NodeInfo()->Int());
        interf_graph->AddEdge(diff_node, def_node);
        interf_graph->AddEdge(def_node, diff_node);
      }
      for (const auto use_temp : use_list) {
        auto use_node = temp_node_map_->Look(use_temp);
        live_graph_.moves->Append(use_node, def_node);
      }
    }
  }
}

void LiveGraphFactory::Liveness() {
  LiveMap();
  InterfGraph();
}

} // namespace live
