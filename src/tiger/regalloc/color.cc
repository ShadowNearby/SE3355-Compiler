#include "tiger/regalloc/color.h"

extern frame::RegManager *reg_manager;

namespace col {
/* TODO: Put your lab6 code here */
Color::Color() {
  coloring_ = temp::Map::LayerMap(temp::Map::Empty(), reg_manager->temp_map_);
};

void Color::AssignColor(temp::Temp *t, std::string *color) {
  //  LOG_DEBUG("assign %s to t%d", (*color).c_str(), t->Int());
  coloring_->Enter(t, color);
}
std::string *Color::LookColor(temp::Temp *t) const {
  return coloring_->Look(t);
}
void Color::InsertOKColor(std::string *color) { ok_colors_.insert(color); }

void Color::RemoveOKColor(std::string *color) { ok_colors_.erase(color); }
void Color::ClearOKColor() { ok_colors_.clear(); }
temp::Map *Color::GetColorResult() {
  //  coloring_->DumpMap(stdout);
  return coloring_;
}
bool Color::OKColorEmpty() { return ok_colors_.empty(); };
std::string *Color::FrontOKColor() { return *ok_colors_.begin(); }
void Color::ClearAll() {
  ok_colors_.clear();
  delete coloring_;
  coloring_ = temp::Map::LayerMap(temp::Map::Empty(), reg_manager->temp_map_);
}
} // namespace col
