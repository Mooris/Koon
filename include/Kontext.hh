#pragma once

#include <unordered_map>
#include <stack>

#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h> /*TMP */
#include <llvm/IR/Constants.h> /*TMP */

#include "KFuncDecl.hh"

class InstanciatedObject;

using Locals = std::unordered_map<std::string, InstanciatedObject*>;

class KExpression;

class KObjectAttr {
    friend class KObject;
public:
    KObjectAttr(std::string name, std::shared_ptr<KExpression> init = nullptr)
        : _name(std::move(name)), _init(std::move(init)) {}

private:
    std::string _name;
    std::shared_ptr<KExpression> _init;
};
class KObject {
public:
    KObject() : _name("Broken") {}
    KObject(std::string name,
            llvm::StructType *leType,
            std::vector<KObjectAttr> attrs)
        : _name(std::move(name)), _type(leType), _attrs(std::move(attrs)) {}

public:
    inline std::string const &name() const { return _name; }
    inline llvm::StructType *type() const { return _type; }
    inline size_t attr(std::string const &name) const {
        size_t idx;
        for (idx = 0; idx < _type->getStructNumElements(); ++idx){if (_attrs[idx]._name ==  name) break;}
        if (_attrs[idx]._name != name) return std::numeric_limits<size_t>::max();
        return idx;
    }

private:
    std::string _name;
    llvm::StructType *_type;
    std::vector<KObjectAttr> _attrs;
};

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
#include <iostream>
class Kontext {
public:
    inline Kontext()
    : _module(new llvm::Module("main", this->_context)) {}

public:
    llvm::Type*                 type_of(const std::string &);
    inline llvm::Module*        module() const { return _module; }
    llvm::Value*                getStructElem(llvm::Value *str, std::string const &name) {
        if (!str) return nullptr;
        size_t idx = 0;
        llvm::Type *t = str->getType();
        while (!t->isStructTy()) { /* Really not F***ing sure 'bout this */
            t = t->getPointerElementType();
        }
        idx = _types[t->getStructName()].attr(name);
        if (idx == std::numeric_limits<size_t>::max()) return nullptr;
        std::vector<llvm::Value*> vec {
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(this->getContext()), 0, false),
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(this->getContext()), idx, false)
        };
        return llvm::GetElementPtrInst::CreateInBounds(str, vec, name.c_str(), this->currentBlock());
    }
    InstanciatedObject*&                locals(std::string const &name) {
        return _blocks.top()->locals[name];
    }
    inline bool isSystemObject(std::shared_ptr<KExpression> name) { //TODO: not sure bout error handle
        std::shared_ptr<KIdentifier> casted = std::dynamic_pointer_cast<KIdentifier>(name);
        if (casted)
            return casted->getName() == "TopLevel";
        return false;
    }
    inline llvm::BasicBlock*    currentBlock() const
    { return _blocks.top()->block;  }
    inline void                 pushBlock(llvm::BasicBlock* block)
    { _blocks.push(new KodeBlock(block)); }
    inline void                 popBlock() { _blocks.pop(); }
    inline void                 setCurrentReturnValue(llvm::Value* v)
    { _blocks.top()->returnValue = v; }
    inline llvm::Value*         getCurrentReturnValue()
    { return _blocks.top()->returnValue; }
    inline void addType(std::string name, KObject type)
    { _types.emplace(std::move(name), std::move(type)); }
    inline bool haveType(std::string const &name) const { _types.find(name) != _types.end(); }

    inline void setObject(std::string const &oName) { _current = &_types[oName]; }
    inline void popObject() { _current = nullptr; }

    inline llvm::LLVMContext& getContext() { return _module->getContext(); }

    void createBuiltins();

private:
    void createInteger();
    void createIntegerAdd();
    void createIntegerPrint();

protected:
    llvm::LLVMContext       _context;
    llvm::Module*           _module;
    std::stack<KodeBlock*>  _blocks;
    std::unordered_map<std::string, KObject>      _types;
    KObject*                _current;
};
