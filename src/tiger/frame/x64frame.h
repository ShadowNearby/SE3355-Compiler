//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {
class X64RegManager : public RegManager {
public:
  X64RegManager() : RegManager() {
    temp_map_->Enter(rax, new std::string("%rax"));
    temp_map_->Enter(rdi, new std::string("%rdi"));
    temp_map_->Enter(rsi, new std::string("%rsi"));
    temp_map_->Enter(rdx, new std::string("%rdx"));
    temp_map_->Enter(rcx, new std::string("%rcx"));
    temp_map_->Enter(r8, new std::string("%r8"));
    temp_map_->Enter(r9, new std::string("%r9"));
    temp_map_->Enter(r10, new std::string("%r10"));
    temp_map_->Enter(r11, new std::string("%r11"));
    temp_map_->Enter(rbx, new std::string("%rbx"));
    temp_map_->Enter(rbp, new std::string("%rbp"));
    temp_map_->Enter(r12, new std::string("%r12"));
    temp_map_->Enter(r13, new std::string("%r13"));
    temp_map_->Enter(r14, new std::string("%r14"));
    temp_map_->Enter(r15, new std::string("%r15"));
    temp_map_->Enter(rsp, new std::string("%rsp"));
  }
  temp::TempList *Registers() override;
  temp::TempList *ArgRegs() override;
  temp::TempList *CallerSaves() override;
  temp::TempList *CalleeSaves() override;
  temp::TempList *ReturnSink() override;
  int WordSize() override;
  temp::Temp *FramePointer() override;
  temp::Temp *StackPointer() override;
  temp::Temp *ReturnValue() override;
  temp::Temp *GetRegister(std::string name) override;

private:
  temp::Temp *rax{temp::TempFactory::NewTemp()};
  temp::Temp *rdi{temp::TempFactory::NewTemp()};
  temp::Temp *rsi{temp::TempFactory::NewTemp()};
  temp::Temp *rdx{temp::TempFactory::NewTemp()};
  temp::Temp *rcx{temp::TempFactory::NewTemp()};
  temp::Temp *r8{temp::TempFactory::NewTemp()};
  temp::Temp *r9{temp::TempFactory::NewTemp()};
  temp::Temp *r10{temp::TempFactory::NewTemp()};
  temp::Temp *r11{temp::TempFactory::NewTemp()};
  temp::Temp *rbx{temp::TempFactory::NewTemp()};
  temp::Temp *rbp{temp::TempFactory::NewTemp()};
  temp::Temp *r12{temp::TempFactory::NewTemp()};
  temp::Temp *r13{temp::TempFactory::NewTemp()};
  temp::Temp *r14{temp::TempFactory::NewTemp()};
  temp::Temp *r15{temp::TempFactory::NewTemp()};
  temp::Temp *rsp{temp::TempFactory::NewTemp()};
};

class X64Frame : public Frame {

public:
  explicit X64Frame(temp::Label *label, const std::list<bool> &formals)
      : Frame(label) {
    NewFrame(formals);
  }

  frame::Access *AllocLocal(bool escape) override {
    if (escape) {
      offset_ -= 8;
      return new InFrameAccess(offset_);
    }
    return new InRegAccess(temp::TempFactory::NewTemp());
  }

  void NewFrame(const std::list<bool> &formals) {
    for (const auto escape : formals) {
      this->formals_.emplace_back(this->AllocLocal(escape));
    }
  }
};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
