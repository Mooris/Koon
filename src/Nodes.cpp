#include <iostream>
#include <llvm/IR/Value.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>
#include "Nodes.hh"
#include "InstanciatedObject.hh"
#include <Kontext.hh>

static std::unordered_map<std::string, llvm::Type*> g_types = {
    { "void",       llvm::Type::getVoidTy(llvm::getGlobalContext()) },
    { "none",       llvm::Type::getVoidTy(llvm::getGlobalContext()) },
    { "double",     llvm::Type::getDoubleTy(llvm::getGlobalContext()) }
};

llvm::Type *Kontext::type_of(const std::string &type) {
    using namespace llvm;

    Type *retType = g_types[type];
    if (!retType) retType = this->_types[type].type();

    if (!retType) {
        // Should throw personal except and catch;
        throw std::runtime_error("Unknown type: " + type);
    }
    return retType;
}

ValuePtr KIdentifier::codeGen(Kontext &kontext) {
    if (!kontext.locals(_name)) {
        /* try dereference `self` */
        if (kontext.locals("self")) {
            return kontext.locals("self")->getStructElem(kontext, _name);
        }

        throw std::runtime_error("No such variable: " + _name);
    }
    return kontext.locals(_name)->get(kontext);
}

llvm::Type* KIdentifier::getType(Kontext &kontext) {
    if (!kontext.locals(_name)) {
        throw std::runtime_error("No such variable: " + _name);
    }
    return kontext.locals(_name)->getType();
}

ValuePtr KBinaryOperator::codeGen(Kontext &k) {
    return KFuncCall(_lhs, KCallArgList(1, KCallArg("operator+", _rhs->codeGen(k)))).codeGen(k);
}

llvm::Type* KBinaryOperator::getType(Kontext &) {
    return nullptr;
}

bool KBinaryOperator::isAssignable() {
    return false;
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

ValuePtr KInt::codeGen(Kontext &kontext) {
    auto type = kontext.type_of("Integer");
    auto object = InstanciatedObject::Create("temp", type, kontext, KCallArgList(1, KCallArg("Integer", llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(llvm::getGlobalContext()),
            _inner, true))));
    return  object->get(kontext);
}

llvm::Type* KInt::getType(Kontext &kontext) {
    return kontext.type_of("Integer");
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

llvm::Type* KVarDecl::getType(Kontext &kontext) {
    return _expr ? _expr->getType(kontext) : kontext.type_of(*_type);
}

ValuePtr KFuncCall::codeGen(Kontext &kontext) {
    using namespace llvm;

    llvm::Function* function = nullptr;
    std::string objName;
    /* TODO: Push toplevel `object` to remove if */
    if (kontext.isSystemObject(_object)) {
        function = kontext.module()->getFunction(_args[0].getName().c_str());
        objName = "TopLevel";
    } else {
        auto val = _object->codeGen(kontext);
        llvm::Type* t = val->getType();
        while (!t->isStructTy()) t = t->getPointerElementType();
        function = kontext.module()->getFunction(t->getStructName().str() + "_" + _args[0].getName());

        _args.emplace(_args.cbegin(), "self", val);
        objName = t->getStructName();
    }
    if (!function)
        throw std::runtime_error("Can't find function: " + objName + "::" + _args[( _args[0].getName() == "self" ? 1 : 0)].getName()); //TODO: broken
    std::vector<Value*> args;
    for (auto &arg: _args) {
        if (arg.haveExpr()) args.emplace_back(arg.codeGen(kontext));
    }
    auto call = CallInst::Create(function, makeArrayRef(args), "", kontext.currentBlock());
    call->setCallingConv(llvm::CallingConv::C);
    return call;
}

bool KFuncCall::isAssignable() {
    return false;
}

llvm::Type* KFuncCall::getType(Kontext &kontext) {
    return kontext.module()->getFunction(_args[0].getName().c_str())->getReturnType();
}

ValuePtr KCallArg::codeGen(Kontext &kontext) {
    if (_value) return _value;
    if (!_expr) throw std::runtime_error("try to fuck with me !\nI'll fuck you up !!!");
    return _expr->codeGen(kontext);
}

ValuePtr KVarDecl::codeGen(Kontext &kontext) {
    if (this->_inObject) { // TODO: Used only for ctor, should be better
        if (_lvalue || _expr)
            kontext.locals("self")->setStructElem(kontext, _name, _lvalue ? _lvalue : new llvm::LoadInst(_expr->codeGen(kontext), "", false, kontext.currentBlock()));
        return nullptr;
    }

    llvm::Type *t = nullptr;
    if (this->_type) {
        t = kontext.type_of(*_type);
    } else if (_expr) {
        t = _expr->getType(kontext);
    } else if (_ltype) {
        t = _ltype;
    }
    if (!t)
        throw std::runtime_error("Can't get type of " + _name + " dude :/");

    auto object = InstanciatedObject::Create(_name, t, kontext, KCallArgList());

    if (_expr != nullptr) {
        object->setValue(_expr->codeGen(kontext));
        object->store(kontext);
    } else if (_lvalue) {
        object->setValue(_lvalue);
        object->store(kontext);
    }
    return nullptr;
}

KFuncDecl::KFuncDecl(KIdentifier &&type, KArgList &&args, KBlock &&block, Kontext *k)
    : _k(k), _type(std::move(type)), _args(std::move(args)), _block(std::move(block))
{
    using namespace llvm;

    FunctionType* fType = nullptr;
    if (_args.size() == 1 && _args[0].getType() == "none") {
        fType = FunctionType::get(k->type_of(_type.getName()), false);
    } else {
        std::vector<Type *> argTypes;
        for (auto &arg: _args) {
            argTypes.push_back(_k->type_of(arg.getType())->getPointerTo());
        }
        fType = FunctionType::get(_k->type_of(_type.getName()), makeArrayRef(argTypes), false);
    }
    this->me = Function::Create(fType,
            GlobalValue::ExternalLinkage,
            _args[0].getCallName().c_str(),
            k->module());
}

void KFuncDecl::remangle(const KObject &parent) {
    using namespace llvm;

    this->me->removeFromParent();
    FunctionType* fType = nullptr;
    std::string name = _args[0].getCallName();
    if (_args[0].getType() == "none")
        _args.erase(_args.cbegin());
    _args.emplace(_args.cbegin(), KArg(KIdentifier("self"), std::string(parent.type()->getStructName())));
    if (!(_args.size() == 1 && _args[0].getType() == "none")) { /* TODO: should remove */
        std::vector<Type *> argTypes;
        for (auto &arg: _args) {
            auto type = _k->type_of(arg.getType())->getPointerTo();
            argTypes.push_back(type);
        }
        fType = FunctionType::get(_k->type_of(_type.getName()), makeArrayRef(argTypes), false);
    }
    this->me = Function::Create(fType,
            GlobalValue::ExternalLinkage,
            (parent.name() + "_" + name).c_str(),
            _k->module());
}

ValuePtr KFuncDecl::codeGen(Kontext &kontext) {
    using namespace llvm;

    auto leBlock = BasicBlock::Create(getGlobalContext(),
                    "entry",
                    this->me,
                    nullptr);

    kontext.pushBlock(leBlock);

    unsigned idx = 0;
    for (auto &arg : this->me->args()) {
        auto obj = InstanciatedObject::Create(_args[idx].getName(), &arg, kontext, KCallArgList());
        arg.setName(_args[idx].getName());
        obj->store(kontext);
        idx += 1;
    }

    _block.codeGen(kontext);

    ReturnInst::Create(getGlobalContext(),
                kontext.getCurrentReturnValue(),
                leBlock);

    kontext.popBlock();
    return this->me;
}

KObjectDecl::KObjectDecl(std::string &&name, KObjFieldList &&stmts, Kontext *kontext)
    : _name(std::move(name)), _stmts(std::move(stmts)) {
    using namespace llvm;

    std::vector<Type *> types;
    std::vector<KObjectAttr> attributes;
    auto leBlock = KBlock();

    for (auto &field: _stmts) {
        if (!field->isVariable()) continue;
        types.emplace_back(field->getType(*kontext));
        attributes.emplace_back(field->getName(), field->getExpr());
        leBlock.emplaceStatement(field);
    }

    auto leType = StructType::create(getGlobalContext(), makeArrayRef(types), _name, false);

    KObject o(_name, leType, std::move(attributes));
    kontext->addType(_name, o);

    for (auto &field: _stmts) {
        if (!field->isVariable()) field->remangle(o);
    }

    auto f = kontext->module()->getFunction((_name + "_" + _name));
    if (!f) {
        _ctor = std::make_shared<KFuncDecl>(KIdentifier("void"),
                                            KArgList(1, KArg(KIdentifier(std::string(_name)), KIdentifier("none"))),
                                            std::move(leBlock), kontext);
        _ctor->remangle(o);
    }
}

ValuePtr KObjectDecl::codeGen(Kontext &kontext) {
    kontext.setObject(_name);

    if (_ctor) {
        _ctor->codeGen(kontext);
    }

    for (auto &f: _stmts) {
        if (f->isVariable()) continue;
        f->codeGen(kontext);
    }
    kontext.popObject();
    return nullptr;
}

void KObjectDecl::remangle(const KObject &parent) {

}
