# Tiger Compiler Labs in C++

## Contents

- [Tiger Compiler Labs in C++](#tiger-compiler-labs-in-c)
    - [Contents](#contents)
    - [Overview](#overview)
    - [Difference Between C Labs and C++ Labs](#difference-between-c-labs-and-c-labs)
    - [Installing Dependencies](#installing-dependencies)
    - [Compiling and Debugging](#compiling-and-debugging)
    - [Testing Your Labs](#testing-your-labs)
    - [Submitting Your Labs](#submitting-your-labs)
    - [Formatting Your Codes](#formatting-your-codes)
    - [Other Commands](#other-commands)
    - [Contributing to Tiger Compiler](#contributing-to-tiger-compiler)
    - [External Documentations](#external-documentations)

## Overview

We rewrote the Tiger Compiler labs using the C++ programming language because some features in C++ like inheritance and
polymorphism
are more suitable for these labs and less error-prone.

We provide you all the codes of all labs at one time. In each lab, you only
need to code in some of the directories.

## Difference Between C Labs and C++ Labs

1. Tiger compiler in C++ uses [flexc++](https://fbb-git.gitlab.io/flexcpp/manual/flexc++.html)
   and [bisonc++](https://fbb-git.gitlab.io/bisoncpp/manual/bisonc++.html) instead of flex and bison because flexc++ and
   bisonc++ is more flexc++ and bisonc++ are able to generate pure C++ codes instead of C codes wrapped in C++ files.

2. Tiger compiler in C++ uses namespace for modularization and uses inheritance and polymorphism to replace unions used
   in the old labs.

3. Tiger compiler in C++ uses CMake instead of Makefile to compile and build the target.

<!---4. We've introduced lots of modern C++-style codes into tiger compiler, e.g., smart pointers, RAII, RTTI. To get familiar with the features of modern C++ and get recommendations for writing code in modern C++ style, please refer to [this doc](https://ipads.se.sjtu.edu.cn/courses/compilers/tiger-compiler-cpp-style.html) on our course website.-->

## Installing Dependencies

We provide you a Docker image that has already installed all the dependencies. You can compile your codes directly in
this Docker image.

1. Install [Docker](https://docs.docker.com/).

2. Run a docker container and mount the lab directory on it.

```bash
# Run this command in the root directory of the project
docker run -it --privileged -p 2222:22 -v $(pwd):/home/stu/tiger-compiler ipadsse302/tigerlabs_env:latest  # or make docker-run
```

## Compiling and Debugging

There are five makeable targets in total, including `test_slp`, `test_lex`, `test_parse`, `test_semant`,
and `tiger-compiler`.

1. Run container environment and attach to it

```bash
# Run container and directly attach to it
docker run -it --privileged -p 2222:22 \
    -v $(pwd):/home/stu/tiger-compiler ipadsse302/tigerlabs_env:latest  # or `make docker-run`
# Or run container in the backend and attach to it later
docker run -dt --privileged -p 2222:22 \
    -v $(pwd):/home/stu/tiger-compiler ipadsse302/tigerlabs_env:latest
docker attach ${YOUR_CONTAINER_ID}
```

2. Build in the container environment

```bash
mkdir build && cd build && cmake .. && make test_xxx  # or `make build`
```

3. Debug using gdb or any IDEs

```bash
gdb test_xxx # e.g. `gdb test_slp`
```

**Note: we will use `-DCMAKE_BUILD_TYPE=Release` to grade your labs, so make
sure your lab passed the released version**

## Testing Your Labs

Use `make`

```bash
make gradelabx
```

You can test all the labs by

```bash
make gradeall
```

## Submitting Your Labs

Push your code to your GitLab repo

```bash
git add somefiles
git commit -m "A message"
git push
```

**Note, each experiment has a separate branch, such as `lab1`. When you finish the `lab1`, you must submit the code to
the `lab1` branch. Otherwise, you won't get a full score in your lab.**

## Formatting Your Codes

We provide an LLVM-style .clang-format file in the project directory. You can use it to format your code.

Use `clang-format` command

```
find . \( -name "*.h" -o -iname "*.cc" \) | xargs clang-format -i -style=file  # or make format
```

or config the clang-format file in your IDE and use the built-in format feature in it.

## Other Commands

Utility commands can be found in the `Makefile`. They can be directly run by `make xxx` in a Unix shell. Windows users
cannot use the `make` command, but the contents of `Makefile` can still be used as a reference for the available
commands.

## Contributing to Tiger Compiler

You can post questions, issues, feedback, or even MR proposals
through [our main GitLab repository](https://ipads.se.sjtu.edu.cn:2020/compilers-2021/compilers-2021/issues). We are
rapidly refactoring the original C tiger compiler implementation into modern C++ style, so any suggestion to make this
lab better is welcomed.

## External Documentations

You can read external documentations on our course website:

- [Lab Assignments](https://ipads.se.sjtu.edu.cn/courses/compilers/labs.shtml)
- [Environment Configuration of Tiger Compiler Labs](https://ipads.se.sjtu.edu.cn/courses/compilers/tiger-compiler-environment.html)

<!---- [Tiger Compiler in Modern C++ Style](https://ipads.se.sjtu.edu.cn/courses/compilers/tiger-compiler-cpp-style.html)-->

## Explain

### Lab1：Straight-line Program Interpreter

在 `lab1` 中，我们实现了一个简单的 **直线式程序解释器**。

主要完成了各语句和各表达式的 `int maxargs(A_stm)` 和 `void interp(A_stm)`
两个函数，分别用来告知给定语句中任意子表达式内的 `print` 语句的参数个数和进行”解释“。

主要代码位于 `src/straightline/slp.h` 和 `src/straightline/slp.cc` 中。代码样例如下所示：

```C++
int A::CompoundStm::MarArgs() const {
	return stm1->MaxArgs() > stm2->MaxArgs() ? stm1->MaxArgs() : stm2->MaxArgs();
};

Table* A::CompoundStm::Interp(Table* t) const {
	return stm2->Interp(stm1->Interp(t));
};
```

注：这个 `lab` 仅用于帮助我们更好地了解 **环境**、**抽象语法**、**递归结构** 和 **代码风格** 等，不属于  `tiger` 编译器的一部分。

### Lab2：Lexical Analysis

在 `lab2` 中，运用 **flexc++** 来完成 `tiger` 的 **词法分析** 部分。**词法分析** 是指将输入分解成一个个独立的词法符号，即
“单词符号”（token）。

主要完成了 **普通词法**、**字符串** 和 **嵌套注释** 三部分的词法分析。 **flexc++**
所使用的 [语法规则](https://fbb-git.github.io/flexcpp/manual/flexc++.html) 。

主要代码位于 `src/tiger/lex/tiger.lex` 中

### Lab3：Parsing

在 `lab3` 中，我们运用 **Bisonc++** 来完成 `tiger` 的 **语法分析**。**语法分析** 是指分析程序的短语结构。

完成了 **表达式**、**变量**、**函数声明**、**类型声明** 等方面的语法分析，其中运用 **定义优先级** 来解决 **冲突问题**
。难点在于学会 **Bisonc++** 所使用的的 [语法规则](https://fbb-git.gitlab.io/bisoncpp/manual/bisonc++.html)。

主要代码位于 `src/tiger/parse/tiger.y` 中

### Lab4：Type Checking

在 `lab4` 中，我们完成了 `tiger` 的 **语义分析** 。**语义分析**
是指将变量的定义与它们的各个使用联系起来，检查每一个表达式是否有正确的类型，并将抽象语法转换成更简单的、适合于生成机器代码的表示。

主要完成了对各种 **表达式**、**变量** 和 **声明** 等的类型检查，并且会在遇到类型不匹配、参数个数不匹配等问题时进行 **报错**
，还有嵌套循环中的 _break_ 检查。

主要代码位于 `src/tiger/semant/semant.cc` 中

### Lab5 Part1：Escape Analysis and Translation

在 `lab5 Part1` 中，我们完成了 `tiger` 的 **逃逸分析** 和 **中间表示树** 部分。

**逃逸变量** 是指一个变量是传地址实参、被取了地址或者内层的嵌套函数对其进行了访问。在 _tiger_中，`record` 和 `array`
作为函数参数时都看做逃逸变量，不存在取地址情况，主要的逃逸变量来源于内层嵌套函数访问外层变量。

**中间表示** 是指一种抽象机器语言，可以表示目标机的操作而不需太多涉及机器相关的操作，且独立于源语言的细节。一个可移植的编译器先将源语言转换成
**IR**，然后再将 **IR** 转换成机器语言，这样便只需要 $N$ 个前端和 $M$ 个后端。

**逃逸分析** 部分利用 **递归** 主要完成了对各个变量是否是逃逸变量的标记。

主要代码位于 `src/tiger/escape/escape.cc`

**中间表示树** 部分将 **抽象表示** 中个别复杂事情（如数组下标、过程调用）化解成一组恰当的机器指令，尤其是对 `While` 和 `For`
循环语句的设计以及 **静态链** 和 **真值标号回填表** 等结构。

主要代码位于 `src/tiger/translate/translate.cc` 中

### Lab5：Tiger Compiler without register allocation

在 `lab5` 中，我们完成了 `tiger` 的 **帧栈** 和 **指令选择** 部分。

**帧栈** 部分主要参考 `X86-64` 的帧栈模式，需要特殊考虑 `caller-saves`、`callee-saves`、超出 $6$ 个的函数参数以及静态链。

主要代码位于 `src/tiger/frame/frame.h` 和 `src/tiger/frame/x64frame.cc` 中

**指令选择** 部分和 `Translate` 部分直接挂钩，特别注意静态链的处理以及寄存器的保护等

主要代码位于 `src/tiger/codegen/codegen.cc`

### Lab6：Register Allocation

在 `lab6` 中，我们完成了 `tiger` 的 **活跃分析** 和 **寄存器分配** 部分。

在 **活跃分析** 部分，我们在 `src/tiger/liveness/flowgraph.cc` 中完成了程度 **控制流图**
的构建，主要是给顺序执行的语句前后结点添加边、以及处理直接跳转 `jmp` 情况

并且巧妙利用 **集合的不可重复性** 来完成 **不动点的判定** ，完成了所有结点的 **活跃性分析**

然后，我们在 `src/tiger/liveness/liveness.cc` 中 `InterGraph` 函数中完成了冲突图的构建，主要参考以下三个原则：

- 对于任何对变量 $a$ 定值 `def` 的 **非传送指令**，以及在该指令处是 **出口活跃** `out` 的变量 $b_1, \cdots, b_j$
  ，添加冲突边 $(a, b_1), \cdots, (a, b_j)$ 。
- 对于 **传送指令** $a \leftarrow c$，如果变量 $b_1, \cdots, b_j$ 在该指令处是 **出口活跃** `out` 的，则对每一个 **不同于
  $c$ 的 $b_i$** 添加冲突边 $(a, b_1), \cdots, (a, b_j)$ 。
- **机械寄存器** 相互之间都一定有冲突边。


1. **构造**：构造冲突图。在 `liveness` 部分实现。

2. **简化**：每次一个地从图中删除 **低度数** 的 （度 $\lt K$）与 **传送无关** 的结点。

  ```C++
  /* Find a degree temp whose degree < K */
  if (!simplify_work_list_->GetList().empty()) {
      auto node = simplify_work_list_->GetList().front();
      simplify_work_list_->DeleteNode(node);
      select_stack_->Append(node);
      for (auto tmp : Adjacent(node)->GetList()) 
        DecrementDegree(tmp);
  };
  ```

3. **合并**：对简化阶段得到的简化图实施保守的合并，即遵循 $Briggs$ 原则或 $George$ 原则。

4. **冻结**：如果 **简化** 和 **合并** 都不能再进行，就寻找一个 **度数较低** 的 **传送有关** 的结点，我们 **冻结**
   这个结点所关联的传送指令，即放弃对这些传送指令进行合并的希望。

5. **溢出**：如果没有低度数的结点，选择一个潜在可能溢出的高度数结点并将其压入栈。我的实现中，在 **简化**
   部分选择结点时并没有完全限制结点的度数必须小于 $K$ ，而是如果小于 $K$ 则满足条件直接选择该结点，如果均不满足则选择度数最小的那个高度数结点，因此也就包含了溢出部分。

6. **选择**：弹出整个栈并指派颜色。先尝试给栈中的每一个结点分配颜色。

如果分配不成功，则实际溢出一个高度数结点，然后再重新进行 **简化** 、**选择** 等流程。

最后，我们在 `src/tiger/regalloc/regalloc.cc` 中整合整个寄存器分配的全过程，主要算法如下：

```C++
LivenessAnalysis(); /* 构造流程图 */
ClearAll();
Build();
MakeWorkList();
do {
  if (!simplify_worklist_->Empty()) {
    Simplify();
  } else if (!worklist_moves_->Empty()) {
    Coalesce();
  } else if (!freeze_worklist_->Empty()) {
    Freeze();
  } else if (!spill_worklist_->Empty()) {
    SelectSpill();
  }
} while (!(simplify_worklist_->Empty() && worklist_moves_->Empty() &&
           freeze_worklist_->Empty() && spill_worklist_->Empty()));
AssignColors();
if (!spilled_nodes_->Empty()) {
  RewriteProgram();
  RegAlloc();
}
```

### lab7

在`lab7`中完成一个为record和array分配空间的heap和一个Garbage Collector

可以选择**Mark and Sweep**(without data moving, easy to implement), **Directly Copying with Forwardee Pointer**(one time
pass,
easy to implement with less fragment), **Mark and Compact**(make full use of heap space, less fragment)三种算法

在本代码中选择了**Mark and Sweep**算法，使用freelist

**Pointer Map**
用于跟踪堆中对象的指针信息，在函数返回出设置label，获取返回的堆中的地址，以此来追踪变量
在frame中记录是pointer的变量在栈上的位置
根据这些指针变量的信息在汇编文件的末尾生成pointer map

```c++

class Frame {

public:
  virtual ~Frame() = default;

  virtual frame::Access *AllocLocal(bool escape) = 0;

  virtual void AddPointer(frame::Access *access) = 0;
  
  explicit Frame(temp::Label *label)
      : name_(label), offset_(0), formals_{}, pointers_{} {}

  [[nodiscard]] auto GetLabel() const -> std::string { return name_->Name(); }
  [[nodiscard]] auto GetSize() const -> int { return -offset_; }
  int offset_;
  temp::Label *name_;
  std::list<frame::Access *> formals_;
  std::list<frame::Access *> pointers_;
};
```

```c++
extern gc::PointerMap *global_roots;
// merge各个fragment的roots
if (need_ra) {
    gc::Roots roots{frame_, il};
    global_roots->Merge(roots.GeneratePointerMaps());
}
// 在汇编文件中生成pointer map内容
if (need_ra) {
    fprintf(out_, ".global %s\n", gc::GC_ROOTS.c_str());
    fprintf(out_, ".data\n");
    fprintf(out_, "%s:\n", gc::GC_ROOTS.c_str());
    global_roots->Build();
    global_roots->Print(out_);
}
```

**Runtime**
采用mark sweep方法，先读取汇编文件中的pointer map信息，初始化堆，用freelist管理空闲的空间，需要分配空间时从freelist中选取第一个有足够空间
的块，空间不足时，调用GC函数

GC函数中，先进行可达性分析，通过DFS收集通过root可以到达的变量，并将其mark，对其他进行sweep，将清理出来的空间返还给freelist

代码实现在`src/tiger/runtime/gc/heap/derived_heap.cc` `src/tiger/runtime/gc/roots/roots.h` `src/tiger/runtime/gc/runtime.cc`
