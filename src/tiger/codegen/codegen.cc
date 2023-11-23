#include "tiger/codegen/codegen.h"

#include "tiger/util/log.h"
#include <cassert>
#include <iostream>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;

} // namespace

namespace cg {

void CodeGen::Codegen() {
  fs_ = frame_->GetLabel() + "_framesize";
  auto list = assem::InstrList{};
  for (const auto stm : traces_->GetStmList()->GetList()) {
    stm->Munch(list, fs_);
  }
  *assem_instr_->GetInstrList() = list;
  /* TODO: Put your lab5 code here */
}
void CodeGen::PushReg(assem::InstrList &instr_list, temp::Temp *reg) {
  instr_list.Append(new assem::MoveInstr{
      "subq $" + std::to_string(reg_manager->WordSize()) + ", `d0",
      new temp::TempList{reg_manager->StackPointer()}, new temp::TempList{}});
  instr_list.Append(new assem::MoveInstr{
      "movq `s0, (`d0)", new temp::TempList{reg_manager->StackPointer()},
      new temp::TempList{reg}});
}
void CodeGen::PopReg(assem::InstrList &instr_list, temp::Temp *reg) {
  instr_list.Append(
      new assem::MoveInstr{"movq (`s0), `d0", new temp::TempList{reg},
                           new temp::TempList{reg_manager->StackPointer()}});
  instr_list.Append(new assem::MoveInstr{
      "addq $" + std::to_string(reg_manager->WordSize()) + ", `d0",
      new temp::TempList{reg_manager->StackPointer()}, new temp::TempList{}});
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  left_->Munch(instr_list, fs);
  right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  auto instr =
      new assem::LabelInstr{temp::LabelFactory::LabelString(label_), label_};
  instr_list.Append(instr);
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  auto instr = new assem::OperInstr{"jmp `j0", nullptr, nullptr,
                                    new assem::Targets{jumps_}};
  instr_list.Append(instr);
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  auto left = left_->Munch(instr_list, fs);
  auto right = right_->Munch(instr_list, fs);
  instr_list.Append(new assem::MoveInstr{
      "cmpq `s0, `d0", new temp::TempList{right}, new temp::TempList{left}});
  auto targets =
      new assem::Targets{new std::vector<temp::Label *>{true_label_}};
  switch (op_) {
  case EQ_OP:
    instr_list.Append(
        new assem::OperInstr{"je `j0", nullptr, nullptr, targets});
    break;
  case NE_OP:
    instr_list.Append(
        new assem::OperInstr{"jne `j0", nullptr, nullptr, targets});
    break;
  case LT_OP:
  case UGE_OP:
    instr_list.Append(
        new assem::OperInstr{"jl `j0", nullptr, nullptr, targets});
    break;
  case GT_OP:
  case ULE_OP:
    instr_list.Append(
        new assem::OperInstr{"jg `j0", nullptr, nullptr, targets});
    break;
  case LE_OP:
  case UGT_OP:
    instr_list.Append(
        new assem::OperInstr{"jle `j0", nullptr, nullptr, targets});
    break;
  case GE_OP:
  case ULT_OP:
    instr_list.Append(
        new assem::OperInstr{"jge `j0", nullptr, nullptr, targets});
    break;
  case REL_OPER_COUNT:
    break;
  }
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  auto src = src_->Munch(instr_list, fs);
  if (typeid(*dst_) == typeid(tree::MemExp)) {
    auto dst = ((MemExp *)dst_)->exp_->Munch(instr_list, fs);
    instr_list.Append(new assem::MoveInstr{
        "movq `s0, (`d0)", new temp::TempList{dst}, new temp::TempList{src}});
  } else {
    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList{dst_->Munch(instr_list, fs)},
        new temp::TempList{src}));
  }
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  exp_->Munch(instr_list, fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto left = left_->Munch(instr_list, fs);
  auto right = right_->Munch(instr_list, fs);
  switch (op_) {
  case PLUS_OP:
    instr_list.Append(new assem::OperInstr{"addq `s0, `d0",
                                           new temp::TempList{right},
                                           new temp::TempList{left}, nullptr});
    return right;
  case MINUS_OP:
    instr_list.Append(new assem::OperInstr{"subq `s0, `d0",
                                           new temp::TempList{right},
                                           new temp::TempList{left}, nullptr});
    return right;
  case MUL_OP:
    instr_list.Append(new assem::OperInstr{"imulq `s0, `d0",
                                           new temp::TempList{right},
                                           new temp::TempList{left}, nullptr});
    return right;
  case DIV_OP:
    instr_list.Append(new assem::OperInstr{"idivq `s0, `d0",
                                           new temp::TempList{right},
                                           new temp::TempList{left}, nullptr});
    return right;
  case AND_OP:
    break;
  case OR_OP:
    break;
  case LSHIFT_OP:
    break;
  case RSHIFT_OP:
    break;
  case ARSHIFT_OP:
    break;
  case XOR_OP:
    break;
  case BIN_OPER_COUNT:
    break;
  }
  return right;
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  auto res = temp::TempFactory::NewTemp();
  auto exp = exp_->Munch(instr_list, fs);
  instr_list.Append(new assem::MoveInstr(
      "movq (`s0), `d0", new temp::TempList(res), new temp::TempList(exp)));
  return res;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  if (temp_ != reg_manager->FramePointer()) {
    return temp_;
  }
  auto reg = temp::TempFactory::NewTemp();
  std::stringstream ss;
  ss << "leaq " << fs << "(`s0), `d0";
  instr_list.Append(
      new assem::MoveInstr(ss.str(), new temp::TempList(reg),
                           new temp::TempList(reg_manager->StackPointer())));
  return reg;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  stm_->Munch(instr_list, fs);
  return exp_->Munch(instr_list, fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  auto res = temp::TempFactory::NewTemp();
  instr_list.Append(
      new assem::MoveInstr{"leaq " + name_->Name() + "(%rip), `d0",
                           new temp::TempList{res}, new temp::TempList{}});
  return res;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  auto dst = temp::TempFactory::NewTemp();
  auto instr =
      new assem::MoveInstr{"movq $" + std::to_string(consti_) + ", `d0",
                           new temp::TempList{dst}, new temp::TempList{}};
  instr_list.Append(instr);
  return dst;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  auto return_val = temp::TempFactory::NewTemp();
  //  for (const auto &item : args_->GetList()) {
  //    LOG_DEBUG("arg");
  //    item->Print(stdout, 0);
  //    std::cout << std::endl;
  //  }
  auto arg_regs = args_->MunchArgs(instr_list, fs);
  instr_list.Append(
      new assem::MoveInstr("callq " + ((NameExp *)fun_)->name_->Name(),
                           reg_manager->CallerSaves(), arg_regs));
  instr_list.Append(
      new assem::MoveInstr("movq `s0, `d0", new temp::TempList(return_val),
                           new temp::TempList(reg_manager->ReturnValue())));
  auto arg_num = args_->GetList().size();
  static auto arg_reg_num = reg_manager->ArgRegs()->GetList().size();
  auto arg_stack_num = arg_num > arg_reg_num ? arg_num - arg_reg_num : 0;
  if (arg_stack_num != 0) {
    instr_list.Append(new assem::MoveInstr{
        "addq $" + std::to_string(reg_manager->WordSize() * arg_stack_num) +
            ", `d0",
        new temp::TempList{reg_manager->StackPointer()}, new temp::TempList{}});
  }
  return return_val;
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list,
                                   std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto res = new temp::TempList{};
  auto arg_regs = reg_manager->ArgRegs();
  auto exp_it = exp_list_.cbegin();
  for (int i = 0;
       i < (int)arg_regs->GetList().size() && exp_it != exp_list_.cend();
       ++i, ++exp_it) {
    auto reg = arg_regs->NthTemp(i);
    auto exp = *exp_it;
    auto temp = exp->Munch(instr_list, fs);
    instr_list.Append(new assem::MoveInstr{
        "movq `s0, `d0", new temp::TempList{reg}, new temp::TempList{temp}});
    res->Append(reg);
  }
  auto exp_it2 = exp_list_.cend();
  exp_it2--;
  exp_it--;
  for (int i = 0; exp_it2 != exp_it; ++i, --exp_it2) {
    auto exp = *exp_it2;
    instr_list.Append(new assem::MoveInstr{
        "subq $" + std::to_string(reg_manager->WordSize()) + ", `d0",
        new temp::TempList{reg_manager->StackPointer()}, new temp::TempList{}});
    instr_list.Append(new assem::MoveInstr{
        "movq `s0, (`d0)", new temp::TempList{reg_manager->StackPointer()},
        new temp::TempList{exp->Munch(instr_list, fs)}});
  }
  //  for (const auto exp : exp_list_) {
  //    res->Append(exp->Munch(instr_list, fs));
  //  }
  return res;
}

} // namespace tree
