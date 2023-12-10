#include "tiger/frame/x64frame.h"
#include <sstream>
extern frame::RegManager *reg_manager;

namespace frame {

temp::TempList *X64RegManager::Registers() {
  static auto *list = new temp::TempList{rax, rbx, rcx, rdx, rsi, rdi, rbp, r8,
                                         r9,  r10, r11, r12, r13, r14, r15};
  return list;
}

temp::TempList *X64RegManager::ArgRegs() {
  static auto *list = new temp::TempList{rdi, rsi, rdx, rcx, r8, r9};
  return list;
}

temp::TempList *X64RegManager::CallerSaves() {
  static auto *list = new temp::TempList{
      rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11,
  };
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

temp::Temp *X64RegManager::GetRegister(std::string name) {
  if (name == "rax")
    return rax;
  if (name == "rdi")
    return rdi;
  if (name == "rdi")
    return rdi;
  if (name == "rdx")
    return rdx;
  if (name == "rcx")
    return rcx;
  if (name == "r8")
    return r8;
  if (name == "r9")
    return r9;
  if (name == "r10")
    return r10;
  if (name == "r11")
    return r11;
  if (name == "rbx")
    return rbx;
  if (name == "rbp")
    return rbp;
  if (name == "r12")
    return r12;
  if (name == "r13")
    return r13;
  if (name == "r14")
    return r14;
  if (name == "r15")
    return r15;
  if (name == "rsp")
    return rsp;
  return nullptr;
}

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
  return new assem::Proc(prolog.str(), body, epilog.str());
}
assem::InstrList *ProcEntryExit2(assem::InstrList *body) {
  body->Append(new assem::OperInstr("", new temp::TempList(),
                                    reg_manager->ReturnSink(), nullptr));
  return body;
}

tree::Stm *ProcEntryExit1(Frame *frame, tree::Stm *stm) {
  auto seq_stm = new tree::SeqStm{new tree::ExpStm(new tree::ConstExp(0)),
                                  new tree::ExpStm(new tree::ConstExp(0))};
  auto *callee_saved = new temp::TempList();
  for (auto reg : reg_manager->CalleeSaves()->GetList()) {
    temp::Temp *dst = temp::TempFactory::NewTemp();
    seq_stm =
        new tree::SeqStm(seq_stm, new tree::MoveStm(new tree::TempExp(dst),
                                                    new tree::TempExp(reg)));
    callee_saved->Append(dst);
  }
  auto arg_reg_num = reg_manager->ArgRegs()->GetList().size();
  auto formal_it = frame->formals_.cbegin();
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
  auto saved = callee_saved->GetList().cbegin();
  for (auto reg : reg_manager->CalleeSaves()->GetList()) {
    stm = new tree::SeqStm(stm, new tree::MoveStm(new tree::TempExp(reg),
                                                  new tree::TempExp(*saved)));
    ++saved;
  }
  return new tree::SeqStm{seq_stm, stm};
}
tree::Exp *ExternalCall(const std::string &func_name, tree::ExpList *args) {
  return new tree::CallExp(
      new tree::NameExp(temp::LabelFactory::NamedLabel(func_name)), args);
}

} // namespace frame
