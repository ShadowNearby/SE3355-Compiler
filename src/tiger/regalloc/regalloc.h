#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/codegen/codegen.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/regalloc/color.h"
#include "tiger/util/graph.h"

namespace ra {

class Result {
public:
  temp::Map *coloring_;
  assem::InstrList *il_;

  Result() : coloring_(nullptr), il_(nullptr) {}
  Result(temp::Map *coloring, assem::InstrList *il)
      : coloring_(coloring), il_(il) {}
  Result(const Result &result) = delete;
  Result(Result &&result) = delete;
  Result &operator=(const Result &result) = delete;
  Result &operator=(Result &&result) = delete;
  ~Result() = default;
};

class RegAllocator {
  /* TODO: Put your lab6 code here */
private:
  std::unique_ptr<ra::Result> result_;

  live::INodeListPtr *precolored_;
  live::INodeListPtr *initial_;
  live::INodeListPtr *simplify_worklist_;
  live::INodeListPtr *freeze_worklist_;
  live::INodeListPtr *spill_worklist_;
  live::INodeListPtr *spilled_nodes_;
  live::INodeListPtr *coalesced_nodes_;
  live::INodeListPtr *colored_nodes_;
  live::INodeListPtr *selected_stack_;

  live::MoveList *coalesced_moves_;
  live::MoveList *constrained_moves_;
  live::MoveList *frozen_moves_;
  live::MoveList *worklist_moves_;
  live::MoveList *active_moves_;

  tab::Table<live::INode, int> *degree_;
  tab::Table<live::INode, live::INode> *alias_;
  tab::Table<live::INode, live::MoveList> *move_list_;
  tab::Table<live::INode, col::Color> color_;

  void LivenessAnalysis();
  void Build();
  void MakeWorkList();
  void Simplify();
  void Coalesce();
  void Freeze();
  void SelectSpill();
  void AssginColors();
  void RewriteProgram();
  void AddEdge(live::INodePtr u, live::INodePtr v);
  void Adjacent(live::INodePtr n);
  live::MoveList *NodeMoves(live::INodePtr n);
  bool MoveRelated(live::INodePtr n);

public:
  RegAllocator(frame::Frame *frame, std::unique_ptr<cg::AssemInstr> instr);
  void RegAlloc();
  std::unique_ptr<ra::Result> TransferResult();
};

} // namespace ra

#endif