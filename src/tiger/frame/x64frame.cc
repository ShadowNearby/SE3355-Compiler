#include "tiger/frame/x64frame.h"
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
  prolog << "PROCEDURE " << temp::LabelFactory::LabelString(frame->name_)
         << "\n";
  std::stringstream epilog;
  epilog << "END\n";
  return new assem::Proc(prolog.str(), body, epilog.str());
}
assem::InstrList *ProcEntryExit2(assem::InstrList *body) {
  body->Append(new assem::OperInstr("", new temp::TempList(),
                                    reg_manager->ReturnSink(), nullptr));
  return body;
}

tree::Stm *ProcEntryExit1(Frame *frame, tree::Stm *stm) { return stm; }
tree::Exp *ExternalCall(const std::string &func_name, tree::ExpList *args) {
  return new tree::CallExp(
      new tree::NameExp(temp::LabelFactory::NamedLabel(func_name)), args);
}

} // namespace frame
