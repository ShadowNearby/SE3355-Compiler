#include "straightline/slp.h"

#include <iostream>
#include <memory>

namespace A {
int A::CompoundStm::MaxArgs() const {
  return stm1->MaxArgs() > stm2->MaxArgs() ? stm1->MaxArgs() : stm2->MaxArgs();
}

Table *A::CompoundStm::Interp(Table *t) const {
  return stm2->Interp(stm1->Interp(t));
}

int A::AssignStm::MaxArgs() const { return exp->MaxArgs(); }

Table *A::AssignStm::Interp(Table *t) const {
  auto *int_and_table = exp->Interp(t);
  int_and_table->t = int_and_table->t->Update(id, int_and_table->i);
  return int_and_table->t;
}

int A::PrintStm::MaxArgs() const { return exps->MaxArgs(); }

Table *A::PrintStm::Interp(Table *t) const {
  auto *int_and_table = exps->Interp(t);
  return int_and_table->t;
}

auto IdExp::MaxArgs() const -> int { return 1; }
auto IdExp::Interp(A::Table *t) -> IntAndTable * {
  return new IntAndTable(t->Lookup(id), t);
  //  return std::make_shared<IntAndTable>(t->Lookup(id), t).get();
}

auto NumExp::MaxArgs() const -> int { return 1; }
auto NumExp::Interp(A::Table *t) -> IntAndTable * {
  return new IntAndTable(num, t);
  //  return std::make_shared<IntAndTable>(num, t).get();
}

auto OpExp::MaxArgs() const -> int { return 1; }
auto OpExp::Interp(A::Table *t) -> IntAndTable * {
  auto left_int_and_table = left->Interp(t);
  auto right_int_and_table = right->Interp(left_int_and_table->t);
  auto left_value = left_int_and_table->i;
  auto right_value = right_int_and_table->i;
  auto res_value = int();
  switch (oper) {
  case PLUS:
    res_value = left_value + right_value;
    break;
  case MINUS:
    res_value = left_value - right_value;
    break;
  case TIMES:
    res_value = left_value * right_value;
    break;
  case DIV:
    res_value = left_value / right_value;
    break;
  }
  return new IntAndTable(res_value, right_int_and_table->t);
  //  return std::make_shared<IntAndTable>(res_value,
  //  right_int_and_table->t).get();
}

auto EseqExp::MaxArgs() const -> int {
  return stm->MaxArgs() > exp->MaxArgs() ? stm->MaxArgs() : exp->MaxArgs();
}
auto EseqExp::Interp(A::Table *t) -> IntAndTable * {
  return exp->Interp(stm->Interp(t));
}

auto PairExpList::MaxArgs() const -> int {
  return exp->MaxArgs() + tail->MaxArgs();
}
auto PairExpList::NumExps() const -> int { return tail->NumExps() + 1; }
auto PairExpList::Interp(A::Table *t) -> IntAndTable * {
  auto int_and_table = exp->Interp(t);
  printf("%d ", int_and_table->i);
  return tail->Interp(int_and_table->t);
}

auto LastExpList::MaxArgs() const -> int { return 1; }
auto LastExpList::NumExps() const -> int { return 1; }
auto LastExpList::Interp(A::Table *t) -> IntAndTable * {
  auto int_and_table = exp->Interp(t);
  printf("%d\n", int_and_table->i);
  return int_and_table;
}

int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  return new Table(key, val, this);
  //  return std::make_shared<Table>(key, val, this).get();
}
} // namespace A
