#include <iostream>
#include <llvm/IR/Value.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>
#include "Nodes.hh"
#include <Kontext.hh>

static std::unordered_map<std::string, llvm::Type*> g_types = {
    { "int32",      llvm::Type::getInt32Ty(llvm::getGlobalContext()) },
    { "void",       llvm::Type::getVoidTy(llvm::getGlobalContext()) },
    { "none",       llvm::Type::getVoidTy(llvm::getGlobalContext()) },
    { "double",     llvm::Type::getDoubleTy(llvm::getGlobalContext()) }
};

static llvm::Type *type_of(const std::string &type) {
    using namespace llvm;
    Type *retType = g_types[type];

    if (!retType) {
        // Should throw personal except and catch;
        throw std::runtime_error("Unknown type: " + type);
    }
    return retType;
}

ValuePtr KIdentifier::codeGen(Kontext &kontext) {
    if (!kontext.locals()[_name]) {
        throw std::runtime_error("No such variable: " + _name);
    }
    return kontext.locals()[_name];
}

llvm::Type* KIdentifier::getType(Kontext &kontext) {
    if (!kontext.locals()[_name]) {
        throw std::runtime_error("No such variable: " + _name);
    }
    return kontext.locals()[_name]->getType();
}

ValuePtr KBlock::codeGen(Kontext &kontext) {
    llvm::Value *ret = nullptr;

    for (auto& stmt: _stmts) {
        ret = stmt->codeGen(kontext);
    }
    return ret;
}

ValuePtr KArg::codeGen(Kontext &kontext) {
    return nullptr;
}

KFuncDecl::KFuncDecl(KIdentifier &&type, KArgList &&args, KBlock &&block, llvm::Module *module)
    : _type(std::move(type)), _args(std::move(args)), _block(std::move(block))
{
    using namespace llvm;

    FunctionType* fType = nullptr;
    if (_args.size() == 1 && _args[0].getType() == "none") {
        fType = FunctionType::get(type_of(_type.getName()), false);
    } else {
        std::vector<Type *> argTypes;
        for (auto &arg: _args) {
            argTypes.push_back(type_of(arg.getType()));
        }
        fType = FunctionType::get(type_of(_type.getName()), makeArrayRef(argTypes), false);
    }
    auto f = Function::Create(fType,
            GlobalValue::ExternalLinkage,
            _args[0].getCallName().c_str(),
            module);
}

ValuePtr KFuncDecl::codeGen(Kontext &kontext) {
    using namespace llvm;

    auto function = kontext.module()->getFunction(_args[0].getCallName().c_str());
    auto leBlock = BasicBlock::Create(getGlobalContext(),
                    "entry",
                    function,
                    nullptr);

    kontext.pushBlock(leBlock);

    unsigned idx = 0;
    for (auto &arg : function->args()) {
        kontext.locals()[_args[idx].getName()] = &arg;
        arg.setName(_args[idx++].getName());
    }

    _block.codeGen(kontext);

    ReturnInst::Create(getGlobalContext(),
                kontext.getCurrentReturnValue(),
                leBlock);

    kontext.popBlock();
    return function;
}

ValuePtr KInt::codeGen(Kontext &) {
    return  llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(llvm::getGlobalContext()),
            _inner, true);
}

llvm::Type* KInt::getType(Kontext &) {
    return llvm::Type::getInt32Ty(llvm::getGlobalContext());
}

ValuePtr KExpressionStatement::codeGen(Kontext &kontext) {
    return _expr->codeGen(kontext);
}

ValuePtr KReturnStatement::codeGen(Kontext &kontext) {
    ValuePtr retval = nullptr;
    if (_expr) {
        retval = _expr->codeGen(kontext);
    }
    kontext.setCurrentReturnValue(retval);
    return retval;
}

ValuePtr KVarDecl::codeGen(Kontext &kontext) {
    llvm::Type *t = nullptr;
    if (this->_type) {
        t = type_of(*_type);
    } else if (_expr) {
        t = _expr->getType(kontext);
    }
    if (!t)
        throw std::runtime_error("Can't get type of " + _name.getName() + "dude :/");

    //llvm::AllocaInst *alloc = new llvm::AllocaInst(t, _name.getName(), kontext.currentBlock());
    //kontext.locals()[_name.getName()] = alloc;
    if (_expr != nullptr) {
        KAssignment assn(_name, _expr);

        kontext.locals()[_name.getName()] = assn.codeGen(kontext);
    }
    return kontext.locals()[_name.getName()];
}

ValuePtr KAssignment::codeGen(Kontext &kontext) {
    //if (!kontext.locals()[_lhs.getName()]) {
        //throw std::runtime_error("No such variable bruh: " + _lhs.getName());
    //}
    //return new llvm::StoreInst(_rhs->codeGen(kontext), kontext.locals()[_lhs.getName()], false, kontext.currentBlock());
    return _rhs->codeGen(kontext);
}

llvm::Type* KAssignment::getType(Kontext &kontext) {
    return _lhs.getType(kontext);
}

ValuePtr KFuncCall::codeGen(Kontext &kontext) {
    using namespace llvm;

    auto function = kontext.module()->getFunction(_args[0].getName().c_str());
    if (!function)
        throw std::runtime_error("Can't find function: " + _args[0].getName());
    std::vector<Value*> args;
    if (_args.size() > 1 || _args[0].haveExpr())
        for (auto &arg: _args) {
            args.emplace_back(arg.codeGen(kontext));
        }
    return CallInst::Create(function, makeArrayRef(args), "", kontext.currentBlock());
}

llvm::Type* KFuncCall::getType(Kontext &kontext) {
    return kontext.module()->getFunction(_args[0].getName().c_str())->getReturnType();
}

ValuePtr KCallArg::codeGen(Kontext &kontext) {
    if (!_expr) throw std::runtime_error("try to fuck with me !\nI'll fuck you up !!!");
    return _expr->codeGen(kontext);
}
