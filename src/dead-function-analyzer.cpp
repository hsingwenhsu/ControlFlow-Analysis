#include "dead-function-analyzer-util.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/PassManager.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/Analysis/CallGraph.h"


#include <algorithm>
#include <map>
#include <set>
#include <stack>
#include <vector>
#include <queue>

using namespace llvm;
using namespace std;

struct DeadFuncOptimizer : public ModulePass {
  static char ID; // Pass identification, replacement for typeid
  DeadFuncOptimizer() : ModulePass(ID) {}

  /*
   * We are assuming that the "main" function is the entry point of a module.
   * This function extracts the function pointer from the function named "main."
   */
  Function *extractEntryFunction(vector<Function *> &allFunctions) {
    for (auto function : allFunctions) {
      if (function->getName().str() == "main") {
        return function;
      }
    }
    return nullptr;
  }

  /*
   * Function for extracting call graphs.
   * Given a list of all functions, we iterate over the body of the function.
   * And check for `CallInst`. We extract the callee from a CallInst.
   * When A callee is found, we add an edge from caller to callee.
   */
  map<Function *, vector<Function *>>
  getCallGraph(vector<Function *> &allFunctions) {
    map<Function *, vector<Function *>> callGraph;
    for (Function *f : allFunctions) {
      vector<Function *> callee;
      for (auto &basicBlock : *f) {
        for (auto &terminator : basicBlock) {
          if(isa<CallBase>(&terminator)) {
            CallBase *cbase = dyn_cast<CallBase>(&terminator);
            Function *called = cbase->getCalledFunction();
            callee.push_back(called);
          }
        }
      }
      
      callGraph[f] = callee;
    }
    return callGraph;
  }


  /*
   * Extract list of dead functions, given the callGraph and
   * the Function * entryFunction.
   */
  vector<Function *> getDeadFunctions(vector<Function *> &allFunctions,
                    map<Function *, vector<Function *>> &callGraph,
                    Function *entryFunction) {
    vector<Function *> dead;
    
    stack <Function*> s;
    set <Function*> visitedSet;
    s.push(entryFunction);
    while(!s.empty()) {
      Function *fTop = s.top();
      visitedSet.insert(fTop);
      s.pop();
      for(auto fTmp: callGraph[fTop]) {
        s.push(fTmp);
      }
    }
    
    for(Function *f : allFunctions) {
      if(visitedSet.find(f)==visitedSet.end()) dead.push_back(f);
    }
    return dead;
  }

  void removeDeadFunctions(Module &M, vector<Function *> &deadFunctions) {
    for(Function *f : deadFunctions) {
      f->replaceAllUsesWith(UndefValue::get(f->getType()));
      f->eraseFromParent();
    }
  }

  bool runOnModule(Module &M) override {
    OptimizationResultWriter writer(M);
    vector<Function *> allFunctions;
    for (Function &F : M) {
      allFunctions.push_back(&F);
    }
    map<Function *, vector<Function *>> callGraph = getCallGraph(allFunctions);
    writer.printCallGraph(callGraph);
    Function *entryFunction = extractEntryFunction(allFunctions);
    if (entryFunction != nullptr) {
      vector<Function *> deadFunctions =
          getDeadFunctions(allFunctions, callGraph, entryFunction);
      writer.printDeadFunctions(deadFunctions);
      removeDeadFunctions(M, deadFunctions);
      writer.writeModifiedModule(M);
    }
    return true;
  }
};

char DeadFuncOptimizer::ID = 0;
static RegisterPass<DeadFuncOptimizer> X("optimize", "Optimization Pass");
