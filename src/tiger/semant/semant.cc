#include "tiger/semant/semant.h"
#include "tiger/absyn/absyn.h"
#include <iostream>
#include <set>
namespace absyn {

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  this->root_->SemAnalyze(venv, tenv, 0, errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  auto entry = venv->Look(sym_);
  if (entry != nullptr && typeid(*entry) == typeid(env::VarEntry)) {
    return dynamic_cast<env::VarEntry *>(entry)->ty_;
  }
  errormsg->Error(pos_, "undefined variable %s", sym_->Name().c_str());
  return type::VoidTy::Instance();
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  auto var_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typeid(*var_ty) != typeid(type::RecordTy)) {
    errormsg->Error(pos_, "not a record type");
    return type::VoidTy::Instance();
  }
  auto record = dynamic_cast<type::RecordTy *>(var_ty);
  for (const auto field : record->fields_->GetList()) {
    if (field->name_->Name() == sym_->Name()) {
      return record;
    }
  }
  errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().c_str());
  return type::VoidTy::Instance();
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  auto var_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if (typeid(*var_ty) != typeid(type::ArrayTy)) {
    errormsg->Error(pos_, "array type required %s", typeid(*var_ty).name());
    return type::VoidTy::Instance();
  }
  auto sub_ty =
      subscript_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if (!sub_ty->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(pos_, "array index must be integer");
    return type::VoidTy::Instance();
  }
  return dynamic_cast<type::ArrayTy *>(var_ty)->ty_->ActualTy();
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  return var_->SemAnalyze(venv, tenv, labelcount, errormsg);
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  auto fun_ty = venv->Look(func_);
  if (fun_ty == nullptr || typeid(*fun_ty) != typeid(env::FunEntry)) {
    errormsg->Error(pos_, "undefined function %s", func_->Name().c_str());
    return type::VoidTy::Instance();
  }
  auto fun_entry = dynamic_cast<env::FunEntry *>(fun_ty);
  if (fun_entry->formals_->GetList().size() != args_->GetList().size()) {
    if (fun_entry->formals_->GetList().size() < args_->GetList().size()) {
      errormsg->Error(pos_, "too many params in function " + func_->Name());
      return type::VoidTy::Instance();
    }
    errormsg->Error(pos_, "para type mismatch");
    return type::VoidTy::Instance();
  }
  auto formal_it = fun_entry->formals_->GetList().cbegin();
  auto arg_it = args_->GetList().cbegin();
  for (size_t i = 0; i < args_->GetList().size(); ++i, ++formal_it, ++arg_it) {
    auto arg_ty = (*arg_it)->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (arg_ty->IsSameType(type::VoidTy::Instance())) {
      return type::VoidTy::Instance();
    }
    if (!arg_ty->IsSameType(*formal_it)) {
      errormsg->Error((*arg_it)->pos_, "para type mismatch");
      return type::VoidTy::Instance();
    }
  }
  return fun_entry->result_;
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  auto left_ty = left_->SemAnalyze(venv, tenv, labelcount, errormsg);
  auto right_ty = right_->SemAnalyze(venv, tenv, labelcount, errormsg);
  switch (oper_) {
  case PLUS_OP:
  case MINUS_OP:
  case TIMES_OP:
  case DIVIDE_OP:
    if (!left_ty->IsSameType(type::IntTy::Instance()) &&
        !errormsg->AnyErrors()) {
      errormsg->Error(left_->pos_, "integer required");
    }
    if (!right_ty->IsSameType(type::IntTy::Instance()) &&
        !errormsg->AnyErrors()) {
      errormsg->Error(right_->pos_, "integer required");
    }
    break;
  case AND_OP:
  case OR_OP:
  case EQ_OP:
  case NEQ_OP:
  case LT_OP:
  case LE_OP:
  case GT_OP:
  case GE_OP:
    if (!left_ty->IsSameType(right_ty) && !errormsg->AnyErrors()) {
      errormsg->Error(pos_, "same type required");
    }
    break;
  case ABSYN_OPER_COUNT:
    errormsg->Error(pos_, "error oper");
    break;
  }
  return type::IntTy::Instance();
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  auto nullable_type_ty = tenv->Look(typ_);
  if (nullable_type_ty == nullptr) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
    return type::NilTy::Instance();
  }
  auto type_ty = nullable_type_ty->ActualTy();
  if (typeid(*type_ty) != typeid(type::RecordTy)) {
    errormsg->Error(pos_, "undefined type %s %s %s", typ_->Name().c_str(),
                    typeid(*type_ty).name(), typeid(type::RecordTy).name());
    return type::NilTy::Instance();
  }
  auto record_ty = dynamic_cast<type::RecordTy *>(type_ty);
  if (fields_->GetList().empty()) {
    return type::NilTy::Instance();
  }
  auto record_fields = record_ty->fields_;
  if (record_fields->GetList().size() != fields_->GetList().size()) {
    errormsg->Error(pos_, "num of fields doesn't match");
  }
  auto size = record_fields->GetList().size();
  auto record_it = record_fields->GetList().cbegin();
  auto field_it = fields_->GetList().cbegin();
  for (size_t i = 0; i < size; ++i, ++record_it, ++field_it) {
    if ((*record_it)->name_->Name() != (*field_it)->name_->Name()) {
      errormsg->Error(pos_, "field %s doesn't exist %s",
                      (*field_it)->name_->Name().c_str(), __LINE__);
    }
    if (!(*field_it)
             ->exp_->SemAnalyze(venv, tenv, labelcount, errormsg)
             ->IsSameType((*record_it)->ty_)) {
      errormsg->Error(pos_, "field %s doesn't match",
                      (*field_it)->name_->Name().c_str());
    }
  }
  return type_ty;
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  type::Ty *ty;
  for (const auto exp : seq_->GetList()) {
    ty = exp->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  return ty;
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  auto exp_ty = exp_->SemAnalyze(venv, tenv, labelcount, errormsg);
  auto var_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!exp_ty->IsSameType(var_ty) &&
      !var_ty->IsSameType(type::VoidTy::Instance())) {
    errormsg->Error(pos_, "unmatched assign exp");
    return type::VoidTy::Instance();
  }
  if (typeid(*var_) == typeid(absyn::SimpleVar)) {
    auto simple_var = dynamic_cast<absyn::SimpleVar *>(var_);
    auto entry = dynamic_cast<env::VarEntry *>(venv->Look(simple_var->sym_));
    if (entry->readonly_) {
      errormsg->Error(pos_, "loop variable can't be assigned");
    }
  }
  return type::VoidTy::Instance();
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  auto test_ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!test_ty->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(test_->pos_, "integer required");
  }
  auto then_ty = then_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (errormsg->AnyErrors()) {
    return then_ty;
  }
  if (elsee_ == nullptr) {
    if (!then_ty->IsSameType(type::VoidTy::Instance())) {
      errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
    }
  } else {
    auto else_ty = elsee_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (!then_ty->IsSameType(else_ty)) {
      errormsg->Error(elsee_->pos_, "then exp and else exp type mismatch");
    }
  }
  return then_ty;
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  auto test_ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!test_ty->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(test_->pos_, "integer required");
  }
  auto body_ty = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
  if (!body_ty->IsSameType(type::VoidTy::Instance())) {
    errormsg->Error(body_->pos_, "while body must produce no value");
  }
  return type::VoidTy::Instance();
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  venv->BeginScope();
  tenv->BeginScope();
  venv->Enter(var_, new env::VarEntry(type::IntTy::Instance(), true));
  auto lo_ty = lo_->SemAnalyze(venv, tenv, labelcount, errormsg);
  auto hi_ty = hi_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!lo_ty->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(lo_->pos_, "for exp's range type is not integer");
  }
  if (!hi_ty->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(hi_->pos_, "for exp's range type is not integer");
  }
  auto body_ty = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
  if (!body_ty->IsSameType(type::VoidTy::Instance())) {
    errormsg->Error(body_->pos_, "for body should produce no value");
  }
  tenv->EndScope();
  venv->EndScope();
  return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  if (labelcount == 0) {
    errormsg->Error(pos_, "break is not inside any loop");
  }
  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  venv->BeginScope();
  tenv->BeginScope();
  for (const auto dec : decs_->GetList()) {
    dec->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  if (body_ != nullptr) {
    body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  tenv->EndScope();
  venv->EndScope();
  return type::VoidTy::Instance();
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  auto nullable_type_ty = tenv->Look(typ_);
  if (nullable_type_ty == nullptr) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
    return type::VoidTy::Instance();
  }
  auto type_ty = nullable_type_ty->ActualTy();
  if (typeid(*type_ty) != typeid(type::ArrayTy)) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
    return type::VoidTy::Instance();
  }
  auto size_ty = size_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!size_ty->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(pos_, "size of array should be int");
    return type::VoidTy::Instance();
  }
  auto init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  auto array_ty = dynamic_cast<type::ArrayTy *>(type_ty);
  if (!init_ty->IsSameType(array_ty->ty_)) {
    errormsg->Error(pos_, "type mismatch");
    return type::VoidTy::Instance();
  }
  return type_ty;
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  std::set<std::string> deduplication;
  for (const auto fun : functions_->GetList()) {
    if (deduplication.count(fun->name_->Name()) != 0) {
      errormsg->Error(pos_, "two functions have the same name");
      return;
    }
    deduplication.insert(fun->name_->Name());
    venv->Enter(fun->name_, new env::FunEntry(
                                fun->params_->MakeFormalTyList(tenv, errormsg),
                                fun->result_ ? tenv->Look(fun->result_)
                                             : type::VoidTy::Instance()));
  }
  for (const auto fun : functions_->GetList()) {
    venv->BeginScope();
    auto formal_list = fun->params_->MakeFormalTyList(tenv, errormsg);
    auto return_ty =
        fun->result_ ? tenv->Look(fun->result_) : type::VoidTy::Instance();
    auto param_it = fun->params_->GetList().cbegin();
    auto formal_it = formal_list->GetList().cbegin();
    for (size_t i = 0; i < fun->params_->GetList().size();
         ++i, ++param_it, ++formal_it) {
      venv->Enter((*param_it)->name_, new env::VarEntry(*formal_it));
    }
    auto body_ty = fun->body_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (!body_ty->IsSameType(return_ty)) {
      errormsg->Error(pos_, "procedure returns value");
    }
    venv->EndScope();
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  auto init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typ_ != nullptr) {
    auto type_entry = tenv->Look(typ_);
    if (type_entry == nullptr) {
      errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
      return;
    }

    if (!init_ty->ActualTy()->IsSameType(type_entry->ActualTy())) {
      errormsg->Error(init_->pos_, "type mismatch");
    }
    venv->Enter(var_, new env::VarEntry(type_entry));
    return;
  }
  if (typeid(*init_ty) == typeid(type::NilTy)) {
    errormsg->Error(pos_, "init should not be nil without type specified %s",
                    typeid(*init_ty).name());
  }
  venv->Enter(var_, new env::VarEntry(init_ty->ActualTy()));
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  std::set<std::string> deduplicate;
  auto check_cycle = [&tenv](const NameAndTy *type) -> bool {
    type::Ty *tmp = tenv->Look(type->name_);
    type::Ty *begin = tmp;
    type::Ty *next;
    while (tmp != nullptr) {
      if (typeid(*tmp) != typeid(type::NameTy)) {
        break;
      }
      next = dynamic_cast<type::NameTy *>(tmp)->ty_;
      if (next == begin) {
        return true;
      }
      tmp = next;
    }
    return false;
  };
  std::list<NameAndTy *> new_list;
  for (const auto item : types_->GetList()) {
    if (deduplicate.count(item->name_->Name()) != 0) {
      errormsg->Error(pos_, "two types have the same name");
      return;
    }
    deduplicate.insert(item->name_->Name());
    tenv->Enter(item->name_, new type::NameTy{item->name_, nullptr});
    new_list.push_front(item);
  }
  for (const auto item : new_list) {
    auto name_ty = dynamic_cast<type::NameTy *>(tenv->Look(item->name_));
    auto inner_type_ty = item->ty_->SemAnalyze(tenv, errormsg);
    if (inner_type_ty == nullptr) {
      errormsg->Error(item->ty_->pos_, "undefined type %s",
                      item->name_->Name().c_str());
      return;
    }
    name_ty->ty_ = inner_type_ty;
    if (check_cycle(item)) {
      errormsg->Error(item->ty_->pos_, "illegal type cycle");
      return;
    }
  }
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  auto ty = tenv->Look(name_);
  if (ty == nullptr) {
    errormsg->Error(pos_, "undefined type %s", name_->Name().c_str());
    return type::VoidTy::Instance();
  }
  return ty;
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  return new type::RecordTy(record_->MakeFieldList(tenv, errormsg));
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  auto ty = tenv->Look(array_);
  if (ty == nullptr) {
    errormsg->Error(pos_, "undefined type %s", array_->Name().c_str());
    return type::VoidTy::Instance();
  }
  return new type::ArrayTy(ty);
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}
} // namespace sem
