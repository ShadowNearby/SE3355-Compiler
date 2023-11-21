#include "tiger/frame/x64frame.h"

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

temp::TempList *X64RegManager::ReturnSink() { return nullptr; }

int X64RegManager::WordSize() { return 8; }

temp::Temp *X64RegManager::FramePointer() { return rbp; }

temp::Temp *X64RegManager::StackPointer() { return rsp; }

temp::Temp *X64RegManager::ReturnValue() { return rax; }

/* TODO: Put your lab5 code here */
assem::Proc *ProcEntryExit3(Frame *pFrame, assem::InstrList *pList) {
  return nullptr;
}
assem::Proc *ProcEntryExit2(Frame *pFrame, assem::InstrList *pList) {
  return nullptr;
}

tree::Stm *ProcEntryExit1(Frame *frame, tree::Stm *stm) { return stm; }

} // namespace frame
