#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <list>
#include <memory>
#include <string>

#include "tiger/codegen/assem.h"
#include "tiger/frame/temp.h"
#include "tiger/translate/tree.h"

namespace frame {

class RegManager {
public:
  RegManager() : temp_map_(temp::Map::Empty()) {}
  temp::Temp *GetRegister(int regno) { return regs_[regno]; }
  [[nodiscard]] virtual temp::Temp *GetRegister(std::string name) = 0;

  /**
   * Get general-purpose registers except RSI
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  [[nodiscard]] virtual temp::TempList *Registers() = 0;

  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  [[nodiscard]] virtual temp::TempList *ArgRegs() = 0;

  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CallerSaves() = 0;

  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CalleeSaves() = 0;

  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  [[nodiscard]] virtual temp::TempList *ReturnSink() = 0;

  /**
   * Get word size
   */
  [[nodiscard]] virtual int WordSize() = 0;

  [[nodiscard]] virtual temp::Temp *FramePointer() = 0;

  [[nodiscard]] virtual temp::Temp *StackPointer() = 0;

  [[nodiscard]] virtual temp::Temp *ReturnValue() = 0;

  temp::Map *temp_map_;

protected:
  std::vector<temp::Temp *> regs_;
};

class Access {
public:
  /* TODO: Put your lab5 code here */
  enum class AccessType { REG, FRAME };

  explicit Access(AccessType type) : type_(type) {}

  virtual tree::Exp *ToExp(tree::Exp *framePtr) const = 0;

  virtual ~Access() = default;

  AccessType type_;
};

class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset)
      : offset(offset), Access(Access::AccessType::FRAME) {}

  tree::Exp *ToExp(tree::Exp *framePtr) const override {
    return new tree::MemExp(new tree::BinopExp(tree::BinOp::PLUS_OP, framePtr,
                                               new tree::ConstExp(offset)));
  }
  /* TODO: Put your lab5 code here */
};

class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg)
      : reg(reg), Access(Access::AccessType::REG) {}

  tree::Exp *ToExp(tree::Exp *framePtr) const override {
    return new tree::TempExp(reg);
  }
  /* TODO: Put your lab5 code here */
};

class Frame {
  /* TODO: Put your lab5 code here */
public:
  virtual ~Frame() = default;

  virtual frame::Access *AllocLocal(bool escape) = 0;

  explicit Frame(temp::Label *label) : name_(label), offset_(0), formals_{} {}

  [[nodiscard]] auto GetLabel() const -> std::string { return name_->Name(); }
  [[nodiscard]] auto GetSize() const -> int { return -offset_; }
  int offset_;
  temp::Label *name_;
  std::list<frame::Access *> formals_;
};

/**
 * Fragments
 */

class Frag {
public:
  virtual ~Frag() = default;

  enum OutputPhase {
    Proc,
    String,
  };

  /**
   *Generate assembly for main program
   * @param out FILE object for output assembly file
   */
  virtual void OutputAssem(FILE *out, OutputPhase phase,
                           bool need_ra) const = 0;
};

class StringFrag : public Frag {
public:
  temp::Label *label_;
  std::string str_;

  StringFrag(temp::Label *label, std::string str)
      : label_(label), str_(std::move(str)) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class ProcFrag : public Frag {
public:
  tree::Stm *body_;
  Frame *frame_;

  ProcFrag(tree::Stm *body, Frame *frame) : body_(body), frame_(frame) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class Frags {
public:
  Frags() = default;
  void PushBack(Frag *frag) { frags_.emplace_back(frag); }
  const std::list<Frag *> &GetList() { return frags_; }

private:
  std::list<Frag *> frags_;
};

/* TODO: Put your lab5 code here */
assem::Proc *ProcEntryExit3(Frame *frame, assem::InstrList *body);
assem::InstrList *ProcEntryExit2(assem::InstrList *body);
tree::Stm *ProcEntryExit1(Frame *frame, tree::Stm *stm);
tree::Exp *ExternalCall(const std::string &func_name, tree::ExpList *args);
} // namespace frame

#endif