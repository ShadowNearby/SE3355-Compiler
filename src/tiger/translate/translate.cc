#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>

#include <iostream>
#include <string>
#include <utility>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/x64frame.h"
#include "tiger/util/log.h"
#include "translate.h"

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;
namespace debug {

std::string PrintTy(type::Ty *ty) {
  if (typeid(*ty) == typeid(type::IntTy)) {
    return "Int";
  }
  if (typeid(*ty) == typeid(type::NilTy)) {
    return "Nil";
  }
  if (typeid(*ty) == typeid(type::VoidTy)) {
    return "Void";
  }
  if (typeid(*ty) == typeid(type::StringTy)) {
    return "String";
  }
  if (typeid(*ty) == typeid(type::RecordTy)) {
    auto record_ty = dynamic_cast<type::RecordTy *>(ty);
    std::string res = "{";
    for (const auto field : record_ty->fields_->GetList()) {
      res += field->name_->Name();
      res += ":";
      res += PrintTy(field->ty_);
      res += " ";
    }
    res.pop_back();
    res += "}";
    return res;
  }
  if (typeid(*ty) == typeid(type::ArrayTy)) {
    auto array_ty = dynamic_cast<type::ArrayTy *>(ty);
    std::string res = "[" + PrintTy(array_ty->ty_) + "]";
    return res;
  }
  if (typeid(*ty) == typeid(type::NameTy)) {
    auto name_ty = dynamic_cast<type::NameTy *>(ty);
    std::string res =
        "(" + name_ty->sym_->Name() + ":" + PrintTy(name_ty->ty_) + ")";
    return res;
  }
  return "ErrorInTy";
}

} // namespace debug

tree::Exp *GetStaticLink(const tr::Level *curr_level,
                         const tr::Level *target_level) {
  auto level = curr_level;
  tree::Exp *frame_ptr = new tree::TempExp(reg_manager->FramePointer());
  while (level != target_level) {
    auto name = level->frame_->name_->Name();
    frame_ptr = level->frame_->formals_.front()->ToExp(frame_ptr);
    level = level->parent_;
  }
  return frame_ptr;
}

namespace tr {

Access *Access::AllocLocal(Level *level, bool escape) {
  /* TODO: Put your lab5 code here */
  return new Access(level, level->frame_->AllocLocal(escape));
}

class Cx {
public:
  PatchList trues_;
  PatchList falses_;
  tree::Stm *stm_;

  Cx(PatchList trues, PatchList falses, tree::Stm *stm)
      : trues_(std::move(trues)), falses_(std::move(falses)), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() override { return exp_; }
  [[nodiscard]] tree::Stm *UnNx() override { return new tree::ExpStm(exp_); }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    auto stm = new tree::CjumpStm(tree::NE_OP, exp_, new tree::ConstExp(0),
                                  nullptr, nullptr);
    auto trues = PatchList{{&stm->true_label_}};
    auto falses = PatchList{{&stm->false_label_}};
    return {trues, falses, stm};
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() override {
    return new tree::EseqExp(stm_, new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() override { return stm_; }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    LOG_ERROR("error transfer Nx to Cx");
    assert(false);
    //    return {PatchList{{}}, PatchList{{}}, nullptr};
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(PatchList trues, PatchList falses, tree::Stm *stm)
      : cx_(std::move(trues), std::move(falses), stm) {}

  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    auto r = temp::TempFactory::NewTemp();
    auto t = temp::LabelFactory::NewLabel();
    auto f = temp::LabelFactory::NewLabel();
    cx_.trues_.DoPatch(t);
    cx_.falses_.DoPatch(f);
    return new tree::EseqExp(
        new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(1)),
        new tree::EseqExp(
            cx_.stm_,
            new tree::EseqExp(
                new tree::LabelStm(f),
                new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r),
                                                    new tree::ConstExp(0)),
                                  new tree::EseqExp(new tree::LabelStm(t),
                                                    new tree::TempExp(r))))));
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return cx_.stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override { return cx_; }
};

void ProgTr::Translate() { /* TODO: Put your lab5 code here */
  LOG_DEBUG("begin translate");
  absyn_tree_->Translate(venv_.get(), tenv_.get(), main_level_.get(),
                         temp::LabelFactory::NamedLabel("__main__"),
                         errormsg_.get());
  LOG_DEBUG("end translate");
}

} // namespace tr

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  LOG_DEBUG("begin root frame %s", level->frame_->name_->Name().c_str());
  return root_->Translate(venv, tenv, level, label, errormsg);
  LOG_DEBUG("end root");
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  LOG_DEBUG("begin SimpleVar name:%s", sym_->Name().c_str());
  auto var_entry = dynamic_cast<env::VarEntry *>(venv->Look(sym_));
  auto staticlink = GetStaticLink(level, var_entry->access_->level_);
  LOG_DEBUG("end SimpleVar name:%s type:%s", sym_->Name().c_str(),
            debug::PrintTy(var_entry->ty_).c_str());
  return new tr::ExpAndTy{
      new tr::ExExp{var_entry->access_->access_->ToExp(staticlink)},
      var_entry->ty_};
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  LOG_DEBUG("begin FieldVar name:%s", sym_->Name().c_str());
  auto var_exp_ty = var_->Translate(venv, tenv, level, label, errormsg);
  auto record_ty = dynamic_cast<type::RecordTy *>(var_exp_ty->ty_);
  type::Ty *res_ty;
  int index = 0;
  for (const auto field : record_ty->fields_->GetList()) {
    if (field->name_->Name() == sym_->Name()) {
      res_ty = field->ty_;
      break;
    }
    index++;
  }
  auto dst_mem_exp = new tree::MemExp{
      new tree::BinopExp(tree::PLUS_OP, var_exp_ty->exp_->UnEx(),
                         new tree::ConstExp{reg_manager->WordSize() * index})};
  LOG_DEBUG("begin FieldVar name:%s", sym_->Name().c_str());
  return new tr::ExpAndTy{new tr::ExExp(dst_mem_exp), res_ty};
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  LOG_DEBUG("begin SubscriptVar");
  auto var_exp_ty = var_->Translate(venv, tenv, level, label, errormsg);
  auto sub_exp_ty = subscript_->Translate(venv, tenv, level, label, errormsg);
  auto dst_mem_exp = new tree::MemExp{new tree::BinopExp(
      tree::PLUS_OP, var_exp_ty->exp_->UnEx(),
      new tree::BinopExp{tree::MUL_OP, sub_exp_ty->exp_->UnEx(),
                         new tree::ConstExp{reg_manager->WordSize()}})};
  auto array_ty = dynamic_cast<type::ArrayTy *>(var_exp_ty->ty_);
  LOG_DEBUG("begin SubscriptVar");
  return new tr::ExpAndTy{new tr::ExExp(dst_mem_exp),
                          array_ty->ty_->ActualTy()};
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  return var_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                          type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  //  LOG_DEBUG("translate Int value:%d", val_);
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(val_)),
                          type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  auto new_label = temp::LabelFactory::NewLabel();
  auto str_frag = new frame::StringFrag(new_label, str_);
  frags->PushBack(str_frag);
  //  LOG_DEBUG("translate String value:%s", str_.c_str());
  return new tr::ExpAndTy(new tr::ExExp(new tree::NameExp(new_label)),
                          type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  LOG_DEBUG("begin CallExp name:%s", func_->Name().c_str());
  auto fun_entry = dynamic_cast<env::FunEntry *>(venv->Look(func_));
  auto arg_list = new tree::ExpList{};
  std::string log_arg = "{";
  if (fun_entry->level_->parent_) {
    if (fun_entry->label_ == label) {
      arg_list->Append(level->frame_->formals_.front()->ToExp(
          new tree::TempExp(reg_manager->FramePointer())));
    } else {
      arg_list->Append(GetStaticLink(level, fun_entry->level_->parent_));
    }
  }

  for (const auto arg_exp : args_->GetList()) {
    auto arg_exp_ty = arg_exp->Translate(venv, tenv, level, label, errormsg);
    arg_list->Append(arg_exp_ty->exp_->UnEx());
    log_arg += debug::PrintTy(arg_exp_ty->ty_) + ", ";
  }
  log_arg = log_arg.substr(0, log_arg.size() - 2);
  log_arg += "}";
  LOG_DEBUG("end CallExp name:%s arg:%s return:%s", func_->Name().c_str(),
            log_arg.c_str(), debug::PrintTy(fun_entry->result_).c_str());
  return new tr::ExpAndTy(
      new tr::ExExp(new tree::CallExp(new tree::NameExp(func_), arg_list)),
      fun_entry->result_);
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto left_exp_ty = left_->Translate(venv, tenv, level, label, errormsg);
  auto right_exp_ty = right_->Translate(venv, tenv, level, label, errormsg);
  LOG_DEBUG("begin OpExp type:%d", oper_);
  if (oper_ == Oper::PLUS_OP || oper_ == Oper::MINUS_OP ||
      oper_ == Oper::TIMES_OP || oper_ == Oper::DIVIDE_OP) {
    tree::BinOp binop;
    switch (oper_) {
    case PLUS_OP:
      binop = tree::PLUS_OP;
      break;
    case MINUS_OP:
      binop = tree::MINUS_OP;
      break;
    case TIMES_OP:
      binop = tree::MUL_OP;
      break;
    case DIVIDE_OP:
      binop = tree::DIV_OP;
      break;
    default:
      assert(0);
    }
    LOG_DEBUG("end OpExp type:%d", oper_);
    return new tr::ExpAndTy{
        new tr::ExExp{new tree::BinopExp{binop, left_exp_ty->exp_->UnEx(),
                                         right_exp_ty->exp_->UnEx()}},
        type::IntTy::Instance()};
  }
  if (oper_ == Oper::GE_OP || oper_ == Oper::GT_OP || oper_ == Oper::LE_OP ||
      oper_ == Oper::LT_OP || oper_ == EQ_OP || oper_ == NEQ_OP) {
    tree::RelOp op;
    switch (oper_) {
    case GE_OP:
      op = tree::GE_OP;
      break;
    case GT_OP:
      op = tree::GT_OP;
      break;
    case LE_OP:
      op = tree::LE_OP;
      break;
    case LT_OP:
      op = tree::LT_OP;
      break;
    case EQ_OP:
      op = tree::EQ_OP;
      break;
    case NEQ_OP:
      op = tree::NE_OP;
      break;
    case ABSYN_OPER_COUNT:
      op = tree::REL_OPER_COUNT;
    default:
      assert(0);
    }
    if ((oper_ == EQ_OP || oper_ == NEQ_OP) &&
        left_exp_ty->ty_->IsSameType(type::StringTy::Instance())) {
      auto arg_list = new tree::ExpList{left_exp_ty->exp_->UnEx(),
                                        right_exp_ty->exp_->UnEx()};
      auto call_exp = new tree::CallExp{
          new tree::NameExp{temp::LabelFactory::NamedLabel("string_equal")},
          arg_list};
      auto stm =
          new tree::CjumpStm{tree::EQ_OP, tr::ExExp(call_exp).UnEx(),
                             right_exp_ty->exp_->UnEx(), nullptr, nullptr};
      return new tr::ExpAndTy{new tr::CxExp{tr::PatchList{{&stm->true_label_}},
                                            tr::PatchList{{&stm->false_label_}},
                                            stm},
                              type::IntTy::Instance()};
    }
    auto stm = new tree::CjumpStm{op, left_exp_ty->exp_->UnEx(),
                                  right_exp_ty->exp_->UnEx(), nullptr, nullptr};
    return new tr::ExpAndTy{new tr::CxExp{tr::PatchList{{&stm->true_label_}},
                                          tr::PatchList{{&stm->false_label_}},
                                          stm},
                            type::IntTy::Instance()};
  }
  auto new_label = temp::LabelFactory::NewLabel();
  auto left_cx = left_exp_ty->exp_->UnCx(errormsg);
  auto right_cx = right_exp_ty->exp_->UnCx(errormsg);
  if (oper_ == Oper::AND_OP) {
    auto trues = tr::PatchList{right_cx.trues_};
    auto falses = tr::PatchList::JoinPatch(left_cx.falses_, right_cx.falses_);
    left_cx.trues_.DoPatch(new_label);
    auto stm = new tree::SeqStm(
        left_cx.stm_,
        new tree::SeqStm(new tree::LabelStm(new_label), right_cx.stm_));
    return new tr::ExpAndTy(new tr::CxExp(trues, falses, stm),
                            type::IntTy::Instance());
  }
  if (oper_ == Oper::OR_OP) {
    auto trues = tr::PatchList::JoinPatch(left_cx.trues_, right_cx.trues_);
    auto falses = tr::PatchList{right_cx.falses_};
    left_cx.falses_.DoPatch(new_label);
    auto stm = new tree::SeqStm(
        left_cx.stm_,
        new tree::SeqStm(new tree::LabelStm(new_label), right_cx.stm_));
    return new tr::ExpAndTy(new tr::CxExp(trues, falses, stm),
                            type::IntTy::Instance());
  }
  assert(0);
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto record_ty = tenv->Look(typ_);
  auto exp_list = tree::ExpList{};
  for (const auto field : fields_->GetList()) {
    auto it = field->exp_->Translate(venv, tenv, level, label, errormsg);
    exp_list.Append(it->exp_->UnEx());
  }
  auto args = new tree::ExpList{};
  args->Append(new tree::ConstExp((int)fields_->GetList().size() *
                                  reg_manager->WordSize()));
  auto dst = temp::TempFactory::NewTemp();
  auto call_exp = new tree::CallExp{
      new tree::NameExp(temp::LabelFactory::NamedLabel("alloc_record")), args};
  auto move_stm = new tree::MoveStm(new tree::TempExp(dst), call_exp);
  tree::Stm *stm = move_stm;
  auto field_it = exp_list.GetList().cbegin();
  for (int i = 0; i < fields_->GetList().size(); ++i, ++field_it) {
    stm = new tree::SeqStm{
        stm,
        new tree::MoveStm{
            new tree::BinopExp(tree::PLUS_OP, new tree::TempExp(dst),
                               new tree::ConstExp(i * reg_manager->WordSize())),
            (*field_it)}};
  }
  return new tr::ExpAndTy{
      new tr::ExExp(new tree::EseqExp{stm, new tree::TempExp(dst)}), record_ty};
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto res = new tr::ExExp(new tree::ConstExp(0));
  tr::ExpAndTy *it = nullptr;
  for (const auto exp : seq_->GetList()) {
    it = exp->Translate(venv, tenv, level, label, errormsg);
    res = new tr::ExExp{new tree::EseqExp{res->UnNx(), it->exp_->UnEx()}};
  }
  return new tr::ExpAndTy{res, it->ty_};
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  LOG_DEBUG("begin AssignExp");
  auto var_exp_ty = var_->Translate(venv, tenv, level, label, errormsg);
  auto exp_exp_ty = exp_->Translate(venv, tenv, level, label, errormsg);
  LOG_DEBUG("end AssignExp");
  return new tr::ExpAndTy{
      new tr::NxExp{new tree::MoveStm(var_exp_ty->exp_->UnEx(),
                                      exp_exp_ty->exp_->UnEx())},
      type::VoidTy::Instance()};
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto test_exp_ty = test_->Translate(venv, tenv, level, label, errormsg);
  auto then_exp_ty = then_->Translate(venv, tenv, level, label, errormsg);
  auto true_label = temp::LabelFactory::NewLabel();
  auto false_label = temp::LabelFactory::NewLabel();
  auto test_stm =
      new tree::CjumpStm{tree::NE_OP, test_exp_ty->exp_->UnEx(),
                         new tree::ConstExp(0), true_label, false_label};
  if (elsee_ == nullptr) {
    auto result_stm = new tree::SeqStm{
        test_stm,
        new tree::SeqStm{new tree::LabelStm(true_label),
                         new tree::SeqStm{then_exp_ty->exp_->UnNx(),
                                          new tree::LabelStm(false_label)}}};
    return new tr::ExpAndTy{new tr::NxExp(result_stm), then_exp_ty->ty_};
  }
  auto else_exp_ty = elsee_->Translate(venv, tenv, level, label, errormsg);
  auto return_val = new tree::TempExp(temp::TempFactory::NewTemp());
  auto result_stm = new tree::SeqStm{
      test_stm,
      new tree::SeqStm{
          new tree::LabelStm(true_label),
          new tree::SeqStm{
              new tree::MoveStm{return_val, then_exp_ty->exp_->UnEx()},
              new tree::SeqStm{
                  new tree::LabelStm(false_label),
                  new tree::MoveStm{return_val, else_exp_ty->exp_->UnEx()}}}}};
  return new tr::ExpAndTy{
      new tr::ExExp(new tree::EseqExp(result_stm, return_val)),
      then_exp_ty->ty_};
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto test_exp_ty = test_->Translate(venv, tenv, level, label, errormsg);
  auto body_exp_ty = body_->Translate(venv, tenv, level, label, errormsg);
  auto test_label = temp::LabelFactory::NewLabel();
  auto body_label = temp::LabelFactory::NewLabel();
  auto done_label = temp::LabelFactory::NewLabel();
  auto test_stm =
      new tree::CjumpStm{tree::NE_OP, test_exp_ty->exp_->UnEx(),
                         new tree::ConstExp(0), body_label, done_label};
  auto result_stm = new tree::SeqStm{
      new tree::JumpStm{new tree::NameExp(test_label),
                        new std::vector<temp::Label *>{test_label}},
      new tree::SeqStm{
          new tree::LabelStm(body_label),
          new tree::SeqStm{
              body_exp_ty->exp_->UnNx(),
              new tree::SeqStm{new tree::LabelStm(test_label),
                               new tree::SeqStm{test_stm, new tree::LabelStm(
                                                              done_label)}}}}};
  return new tr::ExpAndTy{new tr::NxExp(result_stm), type::VoidTy::Instance()};
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  LOG_DEBUG("begin ForExp");
  venv->BeginScope();
  auto loop_var_entry = new env::VarEntry(
      tr::Access::AllocLocal(level, escape_), type::IntTy::Instance(), true);
  venv->Enter(var_, loop_var_entry);
  auto lo_exp_ty = lo_->Translate(venv, tenv, level, label, errormsg);
  auto hi_exp_ty = hi_->Translate(venv, tenv, level, label, errormsg);
  auto body_exp_ty = body_->Translate(venv, tenv, level, label, errormsg);
  auto limit_value_exp =
      new tr::ExExp(new tree::TempExp(temp::TempFactory::NewTemp()));
  auto loop_var_exp = new tr::ExExp(new tree::TempExp(
      dynamic_cast<frame::InRegAccess *>(loop_var_entry->access_->access_)
          ->reg));
  auto body_label = temp::LabelFactory::NewLabel();
  auto done_label = temp::LabelFactory::NewLabel();
  auto test_label = temp::LabelFactory::NewLabel();
  auto init_loop_var_stm =
      new tree::MoveStm(loop_var_exp->UnEx(), lo_exp_ty->exp_->UnEx());
  auto init_limit_value_stm =
      new tree::MoveStm(limit_value_exp->UnEx(), hi_exp_ty->exp_->UnEx());
  auto init_stm = new tree::SeqStm{init_loop_var_stm, init_limit_value_stm};
  auto body_label_stm = new tree::SeqStm{new tree::LabelStm(body_label),
                                         body_exp_ty->exp_->UnNx()};
  auto add_loop_var_stm =
      new tree::MoveStm(loop_var_exp->UnEx(),
                        new tree::BinopExp(tree::PLUS_OP, loop_var_exp->UnEx(),
                                           new tree::ConstExp(1)));
  auto test_stm = new tree::SeqStm{
      new tree::CjumpStm{tree::LT_OP, loop_var_exp->UnEx(),
                         limit_value_exp->UnEx(), body_label, done_label},
      new tree::LabelStm(done_label)};
  auto test_label_stm =
      new tree::SeqStm{new tree::LabelStm(test_label), test_stm};
  auto stm = new tree::SeqStm{
      init_stm,
      new tree::SeqStm{
          new tree::JumpStm{new tree::NameExp{test_label},
                            new std::vector<temp::Label *>{test_label}},
          new tree::SeqStm{body_label_stm, new tree::SeqStm{add_loop_var_stm,
                                                            test_label_stm}}}};
  LOG_DEBUG("end ForExp");
  venv->EndScope();
  return new tr::ExpAndTy(new tr::NxExp(stm), type::VoidTy::Instance());
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(
      new tr::NxExp(new tree::JumpStm(new tree::NameExp(label),
                                      new std::vector<temp::Label *>{label})),
      type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  LOG_DEBUG("LetExp level %s", level->frame_->name_->Name().c_str());

  if (decs_ && !decs_->GetList().empty()) {
    auto dec_list = decs_->GetList();
    tree::SeqStm *seq_stm = nullptr;
    for (const auto dec : dec_list) {
      auto stm = dec->Translate(venv, tenv, level, label, errormsg)->UnNx();
      if (seq_stm == nullptr) {
        seq_stm =
            new tree::SeqStm{new tree::ExpStm(new tree::ConstExp(0)), stm};
      }
      seq_stm = new tree::SeqStm{seq_stm, stm};
    }
    auto body_exp_ty = body_->Translate(venv, tenv, level, label, errormsg);
    return new tr::ExpAndTy{
        new tr::ExExp(new tree::EseqExp{seq_stm, body_exp_ty->exp_->UnEx()}),
        body_exp_ty->ty_};
  }
  auto body_exp_ty = body_->Translate(venv, tenv, level, label, errormsg);
  return body_exp_ty;
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto ty = tenv->Look(typ_);
  LOG_DEBUG("begin ArrayExp type:%s", debug::PrintTy(ty).c_str());
  auto size_exp_ty = size_->Translate(venv, tenv, level, label, errormsg);
  auto init_exp_ty = init_->Translate(venv, tenv, level, label, errormsg);
  auto args = new tree::ExpList{};
  args->Append(size_exp_ty->exp_->UnEx());
  args->Append(init_exp_ty->exp_->UnEx());
  auto call_stm = new tree::CallExp{
      new tree::NameExp(temp::LabelFactory::NamedLabel("init_array")), args};
  LOG_DEBUG("end ArrayExp type:%s", debug::PrintTy(ty).c_str());
  return new tr::ExpAndTy(new tr::NxExp(new tree::ExpStm(call_stm)), ty);
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                          type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  for (const auto fun_dec : functions_->GetList()) {
    // Warn staticlink
    auto escapes = std::list<bool>{true};
    auto tys = fun_dec->params_->MakeFormalTyList(tenv, errormsg);
    for (const auto param_field : fun_dec->params_->GetList()) {
      escapes.emplace_back(param_field->escape_);
    }
    auto new_level =
        new tr::Level{new frame::X64Frame{fun_dec->name_, escapes}, level};
    auto new_label = temp::LabelFactory::NamedLabel(fun_dec->name_->Name());
    auto result_ty = fun_dec->result_ == nullptr ? type::VoidTy::Instance()
                                                 : tenv->Look(fun_dec->result_);
    auto fun_entry = new env::FunEntry{new_level, new_label, tys, result_ty};
    venv->Enter(fun_dec->name_, fun_entry);
  }
  for (const auto fun_dec : functions_->GetList()) {
    LOG_DEBUG("begin FunDec name:%s", fun_dec->name_->Name().c_str());
    venv->BeginScope();
    std::string log_param = "{";
    auto fun_entry = dynamic_cast<env::FunEntry *>(venv->Look(fun_dec->name_));
    auto param_it = fun_dec->params_->GetList().cbegin();
    // Warn staticlink
    auto access_it = fun_entry->level_->frame_->formals_.cbegin()++;
    auto ty_it = fun_entry->formals_->GetList().cbegin();
    for (uint32_t i = 0; i < fun_dec->params_->GetList().size(); ++i) {
      log_param +=
          (*param_it)->name_->Name() + ":" + debug::PrintTy(*ty_it) + ", ";
      venv->Enter((*param_it)->name_,
                  new env::VarEntry(
                      new tr::Access(fun_entry->level_, *access_it), *ty_it));
      param_it++;
      access_it++;
      ty_it++;
    }
    log_param = log_param.substr(0, log_param.size() - 2);
    log_param += "}";
    auto body_exp_ty = fun_dec->body_->Translate(venv, tenv, fun_entry->level_,
                                                 fun_entry->label_, errormsg);
    venv->EndScope();
    frags->PushBack(new frame::ProcFrag{
        frame::ProcEntryExit1(
            fun_entry->level_->frame_,
            new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()),
                              body_exp_ty->exp_->UnEx())),
        fun_entry->level_->frame_});
    LOG_DEBUG("end FunDec name:%s param:%s return:%s",
              fun_dec->name_->Name().c_str(), log_param.c_str(),
              debug::PrintTy(fun_entry->result_).c_str());
  }
  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  LOG_DEBUG("begin VarDec name:%s type:%s escape:%d", var_->Name().c_str(),
            typ_ == nullptr ? "null" : typ_->Name().c_str(), escape_);
  auto init_exp_ty = init_->Translate(venv, tenv, level, label, errormsg);
  auto access = tr::Access::AllocLocal(level, escape_);
  venv->Enter(var_, new env::VarEntry{access, init_exp_ty->ty_->ActualTy()});
  auto left_exp =
      access->access_->ToExp(new tree::TempExp(reg_manager->FramePointer()));
  auto right_exp = init_exp_ty->exp_->UnEx();
  auto res = new tr::NxExp(new tree::MoveStm(left_exp, right_exp));
  LOG_DEBUG("end VarDec name:%s type:%s", var_->Name().c_str(),
            debug::PrintTy(init_exp_ty->ty_).c_str());
  return res;
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  for (const auto type_name_ty : types_->GetList()) {
    tenv->Enter(type_name_ty->name_,
                new type::NameTy(type_name_ty->name_, nullptr));
  }
  for (const auto type_name_ty : types_->GetList()) {
    LOG_DEBUG("begin TypeDec name:%s", type_name_ty->name_->Name().c_str());
    auto name_ty =
        dynamic_cast<type::NameTy *>(tenv->Look(type_name_ty->name_));
    name_ty->ty_ = type_name_ty->ty_->Translate(tenv, errormsg);
    tenv->Set(type_name_ty->name_, name_ty);
    LOG_DEBUG("end TypeDec name:%s type:%s",
              type_name_ty->name_->Name().c_str(),
              debug::PrintTy(name_ty->ty_).c_str());
  }
  return new tr::ExExp(new tree::ConstExp(0));
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  return new type::NameTy(name_, tenv->Look(name_));
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  auto field_list = new type::FieldList();
  for (const auto field : record_->GetList()) {
    auto entry = tenv->Look(field->typ_);
    field_list->Append(new type::Field(field->name_, entry));
  }
  return new type::RecordTy(field_list);
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  return new type::ArrayTy(tenv->Look(array_));
}

} // namespace absyn
