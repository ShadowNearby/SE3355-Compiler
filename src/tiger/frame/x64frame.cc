#include "tiger/frame/x64frame.h"
#include "tiger/util/log.h"
#include <sstream>
extern frame::RegManager *reg_manager;

namespace frame {

temp::TempList *X64RegManager::Registers() {
  static auto *list = new temp::TempList{rax, rdi, rsi, rdx, rcx, r8,  r9, r10,
                                         r11, rbx, rbp, r12, r13, r14, r15};
  return list;
}

temp::TempList *X64RegManager::ArgRegs() {
  static auto *list = new temp::TempList{rdi, rsi, rdx, rcx, r8, r9};
  return list;
}

temp::TempList *X64RegManager::CallerSaves() {
  static auto *list =
      new temp::TempList{rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11};
  return list;
}

temp::TempList *X64RegManager::CalleeSaves() {
  static auto *list = new temp::TempList{rbx, rbp, r12, r13, r14, r15};
  return list;
}

temp::TempList *X64RegManager::ReturnSink() {
  static auto *list = CalleeSaves();
  list->Append(StackPointer());
  list->Append(ReturnValue());
  return list;
}

int X64RegManager::WordSize() { return 8; }

temp::Temp *X64RegManager::FramePointer() { return rbp; }

temp::Temp *X64RegManager::StackPointer() { return rsp; }

temp::Temp *X64RegManager::ReturnValue() { return rax; }

/* TODO: Put your lab5 code here */
assem::Proc *ProcEntryExit3(Frame *frame, assem::InstrList *body) {
  std::stringstream prolog;
  auto name = frame->name_->Name();
  auto size = frame->GetSize();
  prolog << ".set " << name << "_framesize, " << size << "\n";
  prolog << name << ":"
         << "\n";
  prolog << "subq $" << reg_manager->WordSize() << ", %rsp"
         << "\n";
  prolog << "movq %rbp, (%rsp)"
         << "\n";
  prolog << "movq %rsp, %rbp"
         << "\n";
  prolog << "subq $" << size << ", %rsp"
         << "\n";
  std::stringstream epilog;
  epilog << "movq %rbp, %rsp"
         << "\n";
  epilog << "movq (%rsp), %rbp"
         << "\n";
  epilog << "addq $" << reg_manager->WordSize() << ", %rsp"
         << "\n";
  epilog << "retq"
         << "\n";
  epilog << ".END"
         << "\n";
  return new assem::Proc(prolog.str(), body, epilog.str());
}
assem::InstrList *ProcEntryExit2(assem::InstrList *body) {
  body->Append(new assem::OperInstr("", new temp::TempList(),
                                    reg_manager->ReturnSink(), nullptr));
  return body;
}

tree::Stm *ProcEntryExit1(Frame *frame, tree::Stm *stm) {
  auto arg_reg_num = reg_manager->ArgRegs()->GetList().size();
  auto formal_it = frame->formals_.cbegin();
  auto seq_stm = new tree::SeqStm{new tree::ExpStm(new tree::ConstExp(0)),
                                  new tree::ExpStm(new tree::ConstExp(0))};
  for (int i = 0; i < arg_reg_num && formal_it != frame->formals_.cend();
       ++i, ++formal_it) {
    auto formal = *formal_it;
    auto dst = formal->ToExp(new tree::TempExp(reg_manager->FramePointer()));
    auto src = new tree::TempExp(reg_manager->ArgRegs()->NthTemp(i));
    seq_stm = new tree::SeqStm{seq_stm, new tree::MoveStm{dst, src}};
  }
  for (int i = 1; formal_it != frame->formals_.cend(); ++i, ++formal_it) {
    auto formal = *formal_it;
    auto dst = formal->ToExp(new tree::TempExp(reg_manager->FramePointer()));
    auto src = new tree::MemExp(new tree::BinopExp{
        tree::PLUS_OP, new tree::TempExp(reg_manager->FramePointer()),
        new tree::ConstExp((i + 1) * reg_manager->WordSize())});
    seq_stm = new tree::SeqStm{seq_stm, new tree::MoveStm{dst, src}};
  }
  return new tree::SeqStm{seq_stm, stm};
}
tree::Exp *ExternalCall(const std::string &func_name, tree::ExpList *args) {
  return new tree::CallExp(
      new tree::NameExp(temp::LabelFactory::NamedLabel(func_name)), args);
}

} // namespace frame
