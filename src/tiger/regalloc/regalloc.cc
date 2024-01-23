#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"
// #include "tiger/util/log.h"
extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */
RegAllocator::RegAllocator(frame::Frame *frame,
                           std::unique_ptr<cg::AssemInstr> instr)
    : frame_(frame), assem_instr_(std::move(instr)) {
  K = (int)reg_manager->Registers()->GetList().size();
}

void RegAllocator::LivenessAnalysis() {
  fg::FlowGraphFactory flow_graph_factory{assem_instr_->GetInstrList()};
  flow_graph_factory.AssemFlowGraph();
  fg::FGraphPtr flow_graph = flow_graph_factory.GetFlowGraph();
  live::LiveGraphFactory live_graph_factory{flow_graph};
  live_graph_factory.Liveness();
  auto live_graph = live_graph_factory.GetLiveGraph();
  temp_node_map_ = live_graph_factory.GetTempNodeMap();
  worklist_moves_ = live_graph.moves;
  interf_graph_ = live_graph.interf_graph;
  auto interf_graph_node_list = interf_graph_->Nodes()->GetList();
  //  auto show_info = [](temp::Temp *t) {
  //    auto reg = reg_manager->temp_map_->Look(t);
  //    if (reg) {
  //      printf("%s", reg->c_str());
  //    } else {
  //      printf("%d", t->Int());
  //    }
  //  };
  //  graph::Graph<temp::Temp>::Show(stdout, interf_graph_->Nodes(), show_info);
  //  live_graph.moves->Dump(reg_map);
}
void RegAllocator::Build() {
  auto interf_graph_node_list = interf_graph_->Nodes()->GetList();
  auto worklist_moves_list = worklist_moves_->GetList();
  auto reg_map = reg_manager->temp_map_;
  for (const auto node : interf_graph_node_list) {
    auto related_moves = new live::MoveList{};
    for (const auto &move : worklist_moves_list) {
      if (move.first == node || move.second == node) {
        related_moves->Union(move.first, move.second);
      }
    }
    move_list_->Enter(node, related_moves);
    alias_->Enter(node, node);
    degree_->Enter(node, new int(node->OutDegree()));
    bool precolor = reg_map->Look(node->NodeInfo());
    if (precolor) {
      // init precolored
      precolored_->Union(node);
    } else {
      // init initial
      initial_->Union(node);
    }
  }
}
void RegAllocator::MakeWorkList() {
  auto initial_list = initial_->GetList();
  for (const auto node : initial_list) {
    if (*degree_->Look(node) >= K) {
      spill_worklist_->Union(node);
    } else if (MoveRelated(node)) {
      freeze_worklist_->Union(node);
    } else {
      simplify_worklist_->Union(node);
    }
  }
  initial_->Clear();
}
void RegAllocator::Simplify() {
  auto n = simplify_worklist_->GetList().front();
  simplify_worklist_->DeleteNode(n);
  selected_stack_->Union(n);
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
  auto adjacent_v = Adjacent(v)->GetList();
  if (u == v) {
    coalesced_moves_->Union(m.first, m.second);
    AddWorkList(u);
  } else if (precolored_->Contain(v) || u->Adj(v) || v->Adj(u)) {
    constrained_moves_->Union(m.first, m.second);
    AddWorkList(u);
    AddWorkList(v);
  } else if (auto precolor = precolored_->Contain(u);
             (precolor &&
              std::all_of(adjacent_v.begin(), adjacent_v.end(),
                          [&](const auto &t) { return OK(t, u); })) ||
             !precolor && Conservative(Adjacent(u)->Union(Adjacent(v)))) {
    coalesced_moves_->Union(m.first, m.second);
    Combine(u, v);
    AddWorkList(u);
  } else {
    active_moves_->Union(m.first, m.second);
  }
}

void RegAllocator::Freeze() {
  auto u = freeze_worklist_->GetList().front();
  freeze_worklist_->DeleteNode(u);
  simplify_worklist_->Union(u);
  FreezeMoves(u);
}
void RegAllocator::SelectSpill() {
  if (spill_worklist_->GetList().empty()) {
    return;
  }
  auto u = spill_worklist_->GetList().front();
  for (auto t : spill_worklist_->GetList()) {
    if (t->Degree() > u->Degree()) {
      u = t;
    }
  }
  spill_worklist_->DeleteNode(u);
  simplify_worklist_->Union(u);
  FreezeMoves(u);
}
void RegAllocator::AssignColors() {
  while (!selected_stack_->Empty()) {
    //    let n = pop(SelectStack)
    auto stack = selected_stack_->GetList();
    auto n = stack.back();
    selected_stack_->PopBack();
    // ok_colors <- {0...K-1}
    color_.ClearOKColor();
    for (temp::Temp *temp : reg_manager->Registers()->GetList()) {
      auto *color = reg_manager->temp_map_->Look(temp);
      color_.InsertOKColor(color);
    }
    //    forall w ∈ adjList[n]
    //        if GetAlias(w) ∈ (coloredNodes ∪ precolored) then
    //            okColors ← okColors \ {color[GetAlias(w)]}
    for (const auto w : n->Succ()->GetList()) {
      auto w_alias = GetAlias(w);
      if (colored_nodes_->Union(precolored_)->Contain(w_alias)) {
        auto color = color_.LookColor(w_alias->NodeInfo());
        color_.RemoveOKColor(color);
      }
    }
    //    if okColors = {} then
    //          spilledNodes ← spilledNodes ∪ {n}
    if (color_.OKColorEmpty()) {
      spilled_nodes_->Union(n);
    } else {
      //      else
      //          coloredNodes ← coloredNodes ∪ {n}
      //      let c ∈ okColors
      //          color[n] ← c
      colored_nodes_->Union(n);
      auto c = color_.FrontOKColor();
      color_.AssignColor(n->NodeInfo(), c);
    }
  }
  //  forall n ∈ coalescedNodes
  //      color[n] ← color[GetAlias(n)]
  for (const auto n : coalesced_nodes_->GetList()) {
    color_.AssignColor(n->NodeInfo(),
                       color_.LookColor(GetAlias(n)->NodeInfo()));
  }
}
void RegAllocator::RewriteProgram() {
  //  In the program (instructions), insert a store after each
  //  definition of a vi , a fetch before each use of a vi .
  auto new_temps = new live::INodeList;
  for (const auto v : spilled_nodes_->GetList()) {
    auto new_assem_list = new assem::InstrList{};
    //  Allocate memory locations for each v ∈ spilledNodes,
    auto access =
        dynamic_cast<frame::InFrameAccess *>(frame_->AllocLocal(true));
    auto fs = frame_->GetLabel() + "_framesize";
    auto v_temp = v->NodeInfo();
    //  Create a new temporary vi for each definition and each use,
    auto vi = temp::TempFactory::NewTemp();

    for (const auto instr : assem_instr_->GetInstrList()->GetList()) {
      assert(!precolored_->Contain(v));
      instr->Replace(v_temp, vi);
      auto use_list = instr->Use();
      if (use_list->Contain(vi)) {
        new_assem_list->Append(new assem::OperInstr{
            "movq (" + fs + std::to_string(frame_->offset_) + ")(`s0), `d0",
            new temp::TempList{vi},
            new temp::TempList{reg_manager->StackPointer()}, nullptr});
      }
      new_assem_list->Append(instr);
      auto def_list = instr->Def();
      if (def_list->Contain(vi)) {
        new_assem_list->Append(new assem::OperInstr{
            "movq `s0, (" + fs + std::to_string(frame_->offset_) + ")(`d0)",
            new temp::TempList{reg_manager->StackPointer()},
            new temp::TempList{vi}, nullptr});
      }
    }
    assem_instr_ = std::make_unique<cg::AssemInstr>(new_assem_list);
    auto new_node = interf_graph_->NewNode(vi);
    //  Put all the vi into a set newTemps.
    new_temps->Union(new_node);
  }
  assem_instr_->Print(
      stdout, temp::Map::LayerMap(reg_manager->temp_map_, temp::Map::Name()));
  //  spilledNodes ← {}
  //  initial ← coloredNodes ∪ coalescedNodes ∪ newTemps
  //  coloredNodes ← {}
  //  coalescedNodes ← {}
  spilled_nodes_->Clear();
  initial_ = colored_nodes_->Union(coalesced_nodes_)->Union(new_temps);
  colored_nodes_->Clear();
  coalesced_nodes_->Clear();
}
void RegAllocator::AddEdge(live::INodePtr u, live::INodePtr v) {
  if (!u->Adj(v) && u != v) {
    if (!precolored_->Contain(u)) {
      interf_graph_->AddEdge(u, v);
      *degree_->Look(u) += 1;
    }
    if (!precolored_->Contain(v)) {
      interf_graph_->AddEdge(v, u);
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
    frozen_moves_->Union(m.first, m.second);
    if (NodeMoves(v)->Empty() && *degree_->Look(v) < K) {
      freeze_worklist_->DeleteNode(v);
      simplify_worklist_->Union(v);
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
  if (precolored_->Contain(m)) {
    return;
  }
  auto d = degree_->Look(m);
  auto d_val = *d;
  *d -= 1;
  if (d_val == K) {
    auto adjacent = Adjacent(m);
    adjacent->Union(m);
    EnableMoves(adjacent);
    spill_worklist_->DeleteNode(m);
    if (MoveRelated(m)) {
      freeze_worklist_->Union(m);
    } else {
      simplify_worklist_->Union(m);
    }
  }
}

void RegAllocator::EnableMoves(live::INodeListPtr nodes) {
  for (const auto n : nodes->GetList()) {
    auto node_moves = NodeMoves(n)->GetList();
    for (const auto m : node_moves) {
      if (active_moves_->Contain(m.first, m.second)) {
        active_moves_->Delete(m.first, m.second);
        worklist_moves_->Union(m.first, m.second);
      }
    }
  }
}

void RegAllocator::AddWorkList(live::INodePtr u) {
  if (!precolored_->Contain(u) && !MoveRelated(u) && *degree_->Look(u) < K) {
    freeze_worklist_->DeleteNode(u);
    simplify_worklist_->Union(u);
  }
}

bool RegAllocator::OK(live::INodePtr t, live::INodePtr r) {
  return *degree_->Look(t) < K || precolored_->Contain(t) || t->Adj(r);
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
  coalesced_nodes_->Union(v);
  alias_->Enter(v, u);
  move_list_->Look(u)->Union(move_list_->Look(v));
  auto moves = new live::INodeList{};
  moves->Union(v);
  EnableMoves(moves);
  auto adjacent_list = Adjacent(v)->GetList();
  for (const auto t : adjacent_list) {
    AddEdge(t, u);
    DecrementDegree(t);
  }
  if (*degree_->Look(u) >= K && freeze_worklist_->Contain(u)) {
    freeze_worklist_->DeleteNode(u);
    spill_worklist_->Union(u);
  }
}

void RegAllocator::RegAlloc() {
  LivenessAnalysis();
  ClearAll();
  Build();
  MakeWorkList();
  do {
    if (!simplify_worklist_->Empty()) {
      Simplify();
    } else if (!worklist_moves_->Empty()) {
      Coalesce();
    } else if (!freeze_worklist_->Empty()) {
      Freeze();
    } else if (!spill_worklist_->Empty()) {
      SelectSpill();
    }
  } while (!(simplify_worklist_->Empty() && worklist_moves_->Empty() &&
             freeze_worklist_->Empty() && spill_worklist_->Empty()));
  AssignColors();
  if (!spilled_nodes_->Empty()) {
    RewriteProgram();
    RegAlloc();
  }
}
std::unique_ptr<ra::Result> RegAllocator::TransferResult() {
  auto color = color_.GetColorResult();
  RemoveRedundantMove(color);
  return std::make_unique<ra::Result>(color, assem_instr_->GetInstrList());
}

void RegAllocator::RemoveRedundantMove(temp::Map *coloring) {
  auto *new_instr_list = new assem::InstrList();
  for (auto instr : assem_instr_->GetInstrList()->GetList()) {
    if (typeid(*instr) != typeid(assem::MoveInstr)) {
      new_instr_list->Append(instr);
      continue;
    }
    auto move_instr = dynamic_cast<assem::MoveInstr *>(instr);
    auto src = move_instr->src_;
    auto dst = move_instr->dst_;
    if (!src || !dst) {
      new_instr_list->Append(instr);
      continue;
    }
    if (src->GetList().size() != 1 || dst->GetList().size() != 1) {
      new_instr_list->Append(instr);
      continue;
    }
    auto src_color = coloring->Look(src->GetList().front());
    auto dst_color = coloring->Look(dst->GetList().front());
    if (src_color != dst_color) {
      new_instr_list->Append(instr);
      continue;
    }
  }
  assem_instr_ = std::make_unique<cg::AssemInstr>(new_instr_list);
}

void RegAllocator::ClearAll() {

  precolored_->Clear();
  initial_->Clear();
  simplify_worklist_->Clear();
  freeze_worklist_->Clear();
  spill_worklist_->Clear();
  spilled_nodes_->Clear();
  coalesced_nodes_->Clear();
  colored_nodes_->Clear();
  selected_stack_->Clear();

  coalesced_moves_->Clear();
  constrained_moves_->Clear();
  frozen_moves_->Clear();
  worklist_moves_->Clear();
  active_moves_->Clear();

  delete degree_;
  delete alias_;
  delete move_list_;
  degree_ = new tab::Table<live::INode, int>;
  alias_ = new tab::Table<live::INode, live::INode>;
  move_list_ = new tab::Table<live::INode, live::MoveList>;
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