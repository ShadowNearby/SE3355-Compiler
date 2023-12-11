#ifndef TIGER_LIVENESS_LIVENESS_H_
#define TIGER_LIVENESS_LIVENESS_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/x64frame.h"
#include "tiger/liveness/flowgraph.h"
#include "tiger/util/graph.h"
// #include "tiger/util/log.h"
#include <set>

namespace live {

using INode = graph::Node<temp::Temp>;
using INodePtr = graph::Node<temp::Temp> *;
using INodeList = graph::NodeList<temp::Temp>;
using INodeListPtr = graph::NodeList<temp::Temp> *;
using IGraph = graph::Graph<temp::Temp>;
using IGraphPtr = graph::Graph<temp::Temp> *;

class MoveList {
public:
  MoveList() = default;

  [[nodiscard]] const std::list<std::pair<INodePtr, INodePtr>> &
  GetList() const {
    return move_list_;
  }
  void Append(INodePtr src, INodePtr dst) { move_list_.emplace_back(src, dst); }
  bool Contain(INodePtr src, INodePtr dst);
  void Delete(INodePtr src, INodePtr dst);
  void Prepend(INodePtr src, INodePtr dst) {
    move_list_.emplace_front(src, dst);
  }
  bool Empty() { return move_list_.empty(); }
  void Clear() { move_list_.clear(); }
  void Union(INodePtr src, INodePtr dst) {
    if (!Contain(src, dst)) {
      Append(src, dst);
    }
  }
  MoveList *Union(MoveList *list);
  MoveList *Intersect(MoveList *list);

  void Dump(temp::Map *map) {
    printf("Dump move list\n");
    for (const auto &[src, dst] : move_list_) {
      auto src_str = map->Look(src->NodeInfo())
                         ? *map->Look(src->NodeInfo())
                         : "t" + std::to_string(src->NodeInfo()->Int());

      auto dst_str = map->Look(dst->NodeInfo())
                         ? *map->Look(dst->NodeInfo())
                         : "t" + std::to_string(dst->NodeInfo()->Int());
      printf("movq %s -> %s\n", src_str.c_str(), dst_str.c_str());
    }
  }

private:
  std::list<std::pair<INodePtr, INodePtr>> move_list_;
};

struct LiveGraph {
  IGraphPtr interf_graph;
  MoveList *moves;

  LiveGraph(IGraphPtr interf_graph, MoveList *moves)
      : interf_graph(interf_graph), moves(moves) {}
};

class LiveGraphFactory {
public:
  explicit LiveGraphFactory(fg::FGraphPtr flowgraph)
      : flowgraph_(flowgraph), live_graph_(new IGraph(), new MoveList()),
        in_(std::make_unique<graph::Table<assem::Instr, temp::TempList>>()),
        out_(std::make_unique<graph::Table<assem::Instr, temp::TempList>>()),
        temp_node_map_(new tab::Table<temp::Temp, INode>()) {}
  void Liveness();
  LiveGraph GetLiveGraph() { return live_graph_; }
  tab::Table<temp::Temp, INode> *GetTempNodeMap() { return temp_node_map_; }

private:
  fg::FGraphPtr flowgraph_;
  LiveGraph live_graph_;

  std::unique_ptr<graph::Table<assem::Instr, temp::TempList>> in_;
  std::unique_ptr<graph::Table<assem::Instr, temp::TempList>> out_;
  tab::Table<temp::Temp, INode> *temp_node_map_;

  void LiveMap();
  void InterfGraph();
};
std::set<temp::Temp *> MakeSet(temp::TempList *list);
std::set<temp::Temp *> MakeSet(const std::list<temp::Temp *> &list);
temp::TempList *MakeList(const std::set<temp::Temp *> &set);
} // namespace live

#endif