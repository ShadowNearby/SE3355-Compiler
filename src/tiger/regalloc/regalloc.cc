#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"
#include "tiger/util/log.h"
extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */
RegAllocator::RegAllocator(frame::Frame *frame,
                           std::unique_ptr<cg::AssemInstr> instr)
    : frame_(frame), instr_(std::move(instr)) {}

void RegAllocator::LivenessAnalysis() {
  fg::FlowGraphFactory flow_graph_factory{instr_->GetInstrList()};
  flow_graph_factory.AssemFlowGraph();
  fg::FGraphPtr flow_graph = flow_graph_factory.GetFlowGraph();
  live::LiveGraphFactory live_graph_factory{flow_graph};
  live_graph_factory.Liveness();
  auto live_graph = live_graph_factory.GetLiveGraph();
  temp_node_map_ = live_graph_factory.GetTempNodeMap();
  worklist_moves_ = live_graph.moves;
  interf_graph_ = live_graph.interf_graph;

  auto interf_graph_node_list = interf_graph_->Nodes()->GetList();
  interf_graph_node_list.reverse();
  auto reg_map = reg_manager->temp_map_;
  for (const auto node : interf_graph_node_list) {
    // init degree_
    degree_->Enter(node, new int(node->OutDegree()));
    bool precolor = reg_map->Look(node->NodeInfo());
    if (precolor) {
      // init precolored
      precolored_->Append(node);
    } else {
      // init initial
      initial_->Append(node);
    }
    auto node_adj_list = node->Adj()->GetList();
    // init adj_set_
    for (const auto adj_node : node_adj_list) {
      if (adj_set_->Contain(node, adj_node)) {
        adj_set_->Append(node, adj_node);
        adj_set_->Append(adj_node, node);
      }
    }
    // init adj_list_
    if (!precolor) {
      adj_list_->Enter(node, node->Adj());
    }
  }
  auto show_info = [](temp::Temp *t) {
    auto reg = reg_manager->temp_map_->Look(t);
    if (reg) {
      printf("%s", reg->c_str());
    } else {
      printf("%d", t->Int());
    }
  };
  graph::Graph<temp::Temp>::Show(stdout, interf_graph_->Nodes(), show_info);
}
void RegAllocator::Build() {
  auto interf_graph_node_list = interf_graph_->Nodes()->GetList();
  auto worklist_moves_list = worklist_moves_->GetList();
  interf_graph_node_list.reverse();
  for (const auto node : interf_graph_node_list) {
    auto related_moves = new live::MoveList{};
    for (const auto &move : worklist_moves_list) {
      if (move.first == node || move.second == node) {
        related_moves->Append(move.first, move.second);
      }
    }
    move_list_->Enter(node, related_moves);
  }
}
void RegAllocator::MakeWorkList() {
  auto initial_list = initial_->GetList();
  for (const auto node : initial_list) {
    if (*degree_->Look(node) >= K) {
      spill_worklist_->Append(node);
    } else if (MoveRelated(node)) {
      freeze_worklist_->Append(node);
    } else {
      simplify_worklist_->Append(node);
    }
  }
  initial_->Clear();
}
void RegAllocator::Simplify() {
  auto n = simplify_worklist_->GetList().front();
  simplify_worklist_->DeleteNode(n);
  selected_stack_->Append(n);
  for (const auto m : Adjacent(n)->GetList()) {
    DecrementDegree(m);
  }
}

void RegAllocator::Coalesce() {
  auto m = worklist_moves_->GetList().front();
  auto [x, y] = m;
  x = GetAlias(x);
  y = GetAlias(y);
  live::INodePtr u{nullptr}, v{nullptr};
  if (precolored_->Contain(y)) {
    u = y;
    v = x;
  } else {
    u = x;
    v = y;
  }
  worklist_moves_->Delete(m.first, m.second);
  if (u == v) {
    coalesced_moves_->Append(m.first, m.second);
    AddWorkList(u);
  } else if (precolored_->Contain(v) || adj_set_->Contain(u, v)) {
    constrained_moves_->Append(m.first, m.second);
    AddWorkList(u);
    AddWorkList(v);
  } else if (auto adjacent = Adjacent(v)->GetList();
             precolored_->Contain(u) &&
                 std::all_of(adjacent.begin(), adjacent.end(),
                             [&](const auto &t) { return OK(t, u); }) ||
             !precolored_->Contain(u) &&
                 Conservative(Adjacent(u)->Union(Adjacent(v)))) {
    coalesced_moves_->Append(m.first, m.second);
    Combine(u, v);
    AddWorkList(u);
  } else {
    active_moves_->Append(m.first, m.second);
  }
}

void RegAllocator::Freeze() {
  auto u = freeze_worklist_->GetList().front();
  freeze_worklist_->DeleteNode(u);
  simplify_worklist_->Append(u);
  FreezeMoves(u);
}
void RegAllocator::SelectSpill() {
  // TODO
  auto m = spill_worklist_->GetList().front();
  spill_worklist_->DeleteNode(m);
  simplify_worklist_->Append(m);
  FreezeMoves(m);
}
void RegAllocator::AssignColors() {
  while (!selected_stack_->Empty()) {
    auto n = selected_stack_->GetList().back();
    auto ok_colors = new live::INodeList{};
    color_;
  }
}
void RegAllocator::RewriteProgram(live::INodeListPtr nodes) {}
void RegAllocator::AddEdge(live::INodePtr u, live::INodePtr v) {
  if (adj_set_->Contain(u, v) && u != v) {
    adj_set_->Append(u, v);
    adj_set_->Append(v, u);
    if (!precolored_->Contain(u)) {
      adj_list_->Look(u)->Append(v);
      *degree_->Look(u) += 1;
    }
    if (!precolored_->Contain(v)) {
      adj_list_->Look(v)->Append(u);
      *degree_->Look(v) += 1;
    }
  }
}

live::INodeListPtr RegAllocator::Adjacent(live::INodePtr n) {
  auto adj_list = n->Succ();
  return adj_list->Diff(selected_stack_->Union(coalesced_nodes_));
}

live::MoveList *RegAllocator::NodeMoves(live::INodePtr n) {
  auto move_list = move_list_->Look(n);
  return move_list->Intersect(active_moves_->Union(worklist_moves_));
}

bool RegAllocator::MoveRelated(live::INodePtr n) {
  return !NodeMoves(n)->Empty();
}

void RegAllocator::FreezeMoves(live::INodePtr u) {
  auto node_moves_list = NodeMoves(u)->GetList();
  for (const auto m : node_moves_list) {
    auto [x, y] = m;
    live::INodePtr v{nullptr};
    if (GetAlias(y) == GetAlias(u)) {
      v = GetAlias(x);
    } else {
      v = GetAlias(y);
    }
    active_moves_->Delete(m.first, m.second);
    frozen_moves_->Append(m.first, m.second);
    if (NodeMoves(v)->Empty() && *degree_->Look(v) < K) {
      freeze_worklist_->DeleteNode(v);
      simplify_worklist_->Append(v);
    }
  }
}

live::INodePtr RegAllocator::GetAlias(live::INodePtr n) {
  if (coalesced_nodes_->Contain(n)) {
    return GetAlias(alias_->Look(n));
  }
  return n;
}

void RegAllocator::DecrementDegree(live::INodePtr m) {
  auto d = degree_->Look(m);
  auto d_val = *d;
  *d -= 1;
  if (d_val == K) {
    auto adjacent = Adjacent(m);
    adjacent->Append(m);
    EnableMoves(adjacent);
    spill_worklist_->DeleteNode(m);
    if (MoveRelated(m)) {
      freeze_worklist_->Append(m);
    } else {
      simplify_worklist_->Append(m);
    }
  }
}

void RegAllocator::EnableMoves(live::INodeListPtr nodes) {
  for (const auto n : nodes->GetList()) {
    auto node_moves = NodeMoves(n)->GetList();
    for (const auto m : node_moves) {
      if (active_moves_->Contain(m.first, m.second)) {
        active_moves_->Delete(m.first, m.second);
        worklist_moves_->Delete(m.first, m.second);
      }
    }
  }
}

void RegAllocator::AddWorkList(live::INodePtr u) {
  if (precolored_->Contain(u) && !MoveRelated(u) && *degree_->Look(u) < K) {
    freeze_worklist_->DeleteNode(u);
    simplify_worklist_->Append(u);
  }
}

bool RegAllocator::OK(live::INodePtr t, live::INodePtr r) {
  return *degree_->Look(t) < K || precolored_->Contain(t) ||
         adj_set_->Contain(t, r);
}

bool RegAllocator::Conservative(live::INodeListPtr nodes) {
  int k = 0;
  for (const auto n : nodes->GetList()) {
    if (*degree_->Look(n) >= K) {
      k += 1;
    }
  }
  return k < K;
}

void RegAllocator::Combine(live::INodePtr u, live::INodePtr v) {
  if (freeze_worklist_->Contain(v)) {
    freeze_worklist_->DeleteNode(v);
  } else {
    spill_worklist_->DeleteNode(v);
  }
  coalesced_nodes_->Append(v);
  alias_->Enter(v, u);
  move_list_->Look(u)->Union(move_list_->Look(v));
  {
    auto moves = new live::INodeList{};
    moves->Append(v);
    EnableMoves(moves);
  }
  auto adjacent_list = Adjacent(v)->GetList();
  for (const auto t : adjacent_list) {
    AddEdge(t, u);
    DecrementDegree(t);
  }
  if (*degree_->Look(u) >= K && freeze_worklist_->Contain(u)) {
    freeze_worklist_->DeleteNode(u);
    spill_worklist_->Append(u);
  }
}

void RegAllocator::RegAlloc() {
  LivenessAnalysis();
  //  Build();
  //  MakeWorkList();
  //  do {
  //    if (!simplify_worklist_->Empty()) {
  //      Simplify();
  //    } else if (!worklist_moves_->Empty()) {
  //      Coalesce();
  //    } else if (!freeze_worklist_->Empty()) {
  //      Freeze();
  //    } else if (!spill_worklist_->Empty()) {
  //      SelectSpill();
  //    }
  //  } while (simplify_worklist_->Empty() && worklist_moves_->Empty() &&
  //           freeze_worklist_->Empty() && spill_worklist_->Empty());
  //  AssignColors();
  //  if (!spilled_nodes_->Empty()) {
  //    RewriteProgram(spilled_nodes_);
  //    RegAlloc();
  //  }
}
std::unique_ptr<ra::Result> RegAllocator::TransferResult() {
  return std::move(result_);
}
RegAllocator::~RegAllocator() {
  delete precolored_;
  delete initial_;
  delete simplify_worklist_;
  delete freeze_worklist_;
  delete spill_worklist_;
  delete spilled_nodes_;
  delete coalesced_nodes_;
  delete colored_nodes_;
  delete selected_stack_;

  delete coalesced_moves_;
  delete constrained_moves_;
  delete frozen_moves_;
  delete worklist_moves_;
  delete active_moves_;
}
} // namespace ra