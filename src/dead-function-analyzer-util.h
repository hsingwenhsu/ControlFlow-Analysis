#ifndef LLVM_DEAD_FUNCTION_ANALYZER_UTIL_H
#define LLVM_DEAD_FUNCTION_ANALYZER_UTIL_H
#pragma once

#include "tee.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include <fstream>
#include <iostream>
#include <set>
#include <type_traits>
#include <vector>

using namespace std;
using namespace llvm;

// trim from start (in place)
static inline void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
    return !std::isspace(ch);
  }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
  ltrim(s);
  rtrim(s);
}

class OptimizationResultWriter {
private:
  std::fstream file;
  Tee<std::fstream &, std::ostream &> tee;
  string moduleName;

  std::string instructionToString(Instruction *inst) {
    std::string s = "";
    raw_string_ostream *strm = new raw_string_ostream(s);
    inst->print(*strm);
    std::string instStr = strm->str();
    trim(instStr);
    return instStr;
  }

public:
  OptimizationResultWriter(Module &module)
      : file(module.getName().str() + ".txt", std::ios::out | std::ios::trunc),
        tee(file, std::cout) {
    moduleName = module.getName().str();
    int idx = 0;
    int sz = moduleName.size();
    for (; idx < sz; idx++) {
      if (moduleName[idx] == '.') {
        break;
      }
    }
    if (idx < sz - 1) {
      moduleName = moduleName.substr(0, idx);
    }
  }

  void printDeadFunctions(vector<Function *> &deadFunctions) {
    tee << "=============================================================\n";
    tee << "Printing Dead Functions\n";
    tee << "-------------------------------------------------------------\n\n";
    for (auto func : deadFunctions) {
      tee << func->getName().str() << "\n";
    }
    tee << "=============================================================\n";
  }

  void printCallGraph(map<Function *, vector<Function *>> &callGraph) {
    std::fstream callGraphFile(moduleName + ".dot",
                               std::ios::out | std::ios::trunc);
    cout << "=============================================================\n";
    cout << "Printing Call Graph\n";
    cout << "-------------------------------------------------------------\n\n";
    Tee<std::fstream &, std::ostream &> callGraphTee(callGraphFile, std::cout);
    callGraphTee << "digraph " << moduleName << "{\n";
    for (auto graphList : callGraph) {
      for (auto callee : graphList.second) {
        callGraphTee << "\t" << graphList.first->getName().str() << " -> "
                     << callee->getName().str() << ";\n";
      }
    }
    callGraphTee << "}\n";
    callGraphFile.close();
    cout << "Call graph printed to: " << moduleName + ".dot\n";
    cout << "Run the following command to generate the PDF:\n";
    cout << "\t\"dot -Tpdf " << moduleName + ".dot -o " << moduleName + ".pdf\"\n";
    cout << "=============================================================\n";
  }

  void writeModifiedModule(Module &m) {
    std::fstream modifiedModuleFile(moduleName + "-modified.ll",
                                    std::ios::out | std::ios::trunc);
    Tee<std::fstream &> modifiedLLTee(modifiedModuleFile);
    std::string s = "";
    raw_string_ostream *strm = new raw_string_ostream(s);
    m.print(*strm, nullptr);
    modifiedLLTee << strm->str();
    modifiedModuleFile.close();
  }

  void printDeadInstruction(Function *function, Instruction* deadInstruction) {
    tee << "Dead instruction in " << function->getName().str() << ":\t";
    tee << instructionToString(deadInstruction) << "\n";
  }

  void printDeadInstructions(Function *function, vector<Instruction*> deadInstructions) {
    tee << "=============================================================\n";
    tee << "Dead Instructions in " << function->getName().str() << "\n";
    tee << "-------------------------------------------------------------\n\n";
    for(auto deadInstruction : deadInstructions) {
      tee << instructionToString(deadInstruction) << "\n";
    }
    tee << "=============================================================\n";
  }
};
#endif // LLVM_DEAD_FUNCTION_ANALYZER_UTIL_H
