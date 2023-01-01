add_llvm_library(LLVMOptimizer MODULE BUILDTREE_ONLY
        dead-function-analyzer.cpp dead-function-analyzer-util.h tee.h
        DEPENDS
        intrinsics_gen
        PLUGIN_TOOL
        opt
)
