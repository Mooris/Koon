#pragma once

#include <unordered_map>
#include <stack>

#include <llvm/IR/Module.h>

using Locals = std::unordered_map<std::string, llvm::Value*>;

class KodeBlock {
    friend class Kontext;
public:
    inline KodeBlock(llvm::BasicBlock* b)
        : block(b), returnValue(nullptr) {}

private:
    llvm::BasicBlock*   block;
    llvm::Value*        returnValue;
    Locals              locals;
};

class Kontext {
public:
    inline Kontext()
    : _module(new llvm::Module("main", llvm::getGlobalContext())) {}

public:
    inline llvm::Module*        module() const { return _module; }
    inline Locals&              locals()
    { return _blocks.top()->locals; }
    inline llvm::BasicBlock*    currentBlock() const
    { return _blocks.top()->block;  }
    inline void                 pushBlock(llvm::BasicBlock* block)
    { _blocks.push(new KodeBlock(block)); }
    inline void                 popBlock() { _blocks.pop(); }
    inline void                 setCurrentReturnValue(llvm::Value* v)
    { _blocks.top()->returnValue = v; }
    inline llvm::Value*         getCurrentReturnValue()
    { return _blocks.top()->returnValue; }

protected:
    llvm::Module*           _module;
    std::stack<KodeBlock*>  _blocks;
};
