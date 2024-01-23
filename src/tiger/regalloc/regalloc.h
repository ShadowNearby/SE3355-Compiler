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
  //  std::unique_ptr<ra::Result> result_;
  std::unique_ptr<cg::AssemInstr> assem_instr_;
  frame::Frame *frame_;
  live::IGraphPtr interf_graph_{nullptr};
  tab::Table<temp::Temp, live::INode> *temp_node_map_{nullptr};

  live::INodeListPtr precolored_{new live::INodeList{}};
  live::INodeListPtr initial_{new live::INodeList{}};
  live::INodeListPtr simplify_worklist_{new live::INodeList{}};
  live::INodeListPtr freeze_worklist_{new live::INodeList{}};
  live::INodeListPtr spill_worklist_{new live::INodeList{}};
  live::INodeListPtr spilled_nodes_{new live::INodeList{}};
  live::INodeListPtr coalesced_nodes_{new live::INodeList{}};
  live::INodeListPtr colored_nodes_{new live::INodeList{}};
  live::INodeListPtr selected_stack_{new live::INodeList{}};

  live::MoveList *coalesced_moves_{new live::MoveList{}};
  live::MoveList *constrained_moves_{new live::MoveList{}};
  live::MoveList *frozen_moves_{new live::MoveList{}};
  live::MoveList *worklist_moves_{new live::MoveList{}};
  live::MoveList *active_moves_{new live::MoveList{}};
  live::MoveList *adj_set_{new live::MoveList{}};

  tab::Table<live::INode, int> *degree_{new tab::Table<live::INode, int>};
  tab::Table<live::INode, live::INode> *alias_{
      new tab::Table<live::INode, live::INode>};
  tab::Table<live::INode, live::MoveList> *move_list_{
      new tab::Table<live::INode, live::MoveList>};
  col::Color color_;
  tab::Table<live::INode, live::INodeList> *adj_list_{
      new tab::Table<live::INode, live::INodeList>{}};

  void LivenessAnalysis();
  void Build();
  void MakeWorkList();
  void Simplify();
  void Coalesce();
  void Freeze();
  void SelectSpill();
  void AssignColors();
  void RewriteProgram();
  void AddEdge(live::INodePtr u, live::INodePtr v);
  live::INodeListPtr Adjacent(live::INodePtr n);
  live::MoveList *NodeMoves(live::INodePtr n);
  void DecrementDegree(live::INodePtr m);
  void EnableMoves(live::INodeListPtr nodes);
  void AddWorkList(live::INodePtr u);
  bool OK(live::INodePtr t, live::INodePtr r);
  bool Conservative(live::INodeListPtr nodes);
  void Combine(live::INodePtr u, live::INodePtr v);
  bool MoveRelated(live::INodePtr n);
  void FreezeMoves(live::INodePtr u);
  live::INodePtr GetAlias(live::INodePtr n);
  void ClearAll();
  void RemoveRedundantMove(temp::Map *coloring);
  int K{};

public:
  RegAllocator(frame::Frame *frame, std::unique_ptr<cg::AssemInstr> instr);
  void RegAlloc();
  std::unique_ptr<ra::Result> TransferResult();
  ~RegAllocator();
};

} // namespace ra

#endif