#ifndef TIGER_COMPILER_COLOR_H
#define TIGER_COMPILER_COLOR_H

#include "tiger/codegen/assem.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/util/graph.h"
#include "tiger/util/log.h"

namespace col {
struct Result {
  Result() : coloring(nullptr), spills(nullptr) {}
  Result(temp::Map *coloring, live::INodeListPtr spills)
      : coloring(coloring), spills(spills) {}
  temp::Map *coloring;
  live::INodeListPtr spills;
};

class Color {

  /* TODO: Put your lab6 code here */
public:
  Color();

  void AssignColor(temp::Temp *t, std::string *color);
  std::string *LookColor(temp::Temp *t) const;
  void InsertOKColor(std::string *color);

  void RemoveOKColor(std::string *color);
  void ClearOKColor();
  temp::Map *GetColorResult();
  bool OKColorEmpty();
  std::string *FrontOKColor();

  void ClearAll();

private:
  temp::Map *coloring_;
  std::set<std::string *> ok_colors_;
};
} // namespace col

#endif // TIGER_COMPILER_COLOR_H
