#pragma once

#include <memory>
#include <iostream>
#include "Kontext.hh"

class InstanciatedObject {
public:
	InstanciatedObject(std::string name, llvm::Type *type, Kontext &kontext)
		: _name(std::move(name)), _type(type), _val(nullptr) {
			_alloc = new llvm::AllocaInst(_type, _name, kontext.currentBlock());
    		kontext.locals(_name) = this;
		}
	virtual ~InstanciatedObject() {}

public:
	inline llvm::Type *getType() { return _type; }
	inline void setValue(llvm::Value *val) {
		if (val->getType() != _type) throw std::runtime_error("Wrong assign");
		_val = val;
	}
	virtual void store(Kontext &kontext) {
		if (!_val) throw std::runtime_error("No Value to assign");
        new llvm::StoreInst(_val, _alloc, false, kontext.currentBlock());
	}

	virtual llvm::Value *get(Kontext &) const = 0;
	virtual llvm::Value *load(Kontext &) const = 0;
	virtual llvm::Value *getStructElem(Kontext &, std::string const &) {
		throw 42; //TODO: real f*ckin' throw .......
	}
	virtual void setStructElem(Kontext &, std::string const &, llvm::Value *) {
		throw 43; //TODO: same ..............
	}

public:
	static InstanciatedObject* Create(std::string const &, llvm::Value *, Kontext &, KCallArgList &&);
	static InstanciatedObject* Create(std::string const &, llvm::Type *, Kontext &, KCallArgList &&);

protected:
	std::string 	_name;
	llvm::Type* 	_type;
	llvm::AllocaInst* 	_alloc;
	llvm::Value* 	_val;
};

class Object: public InstanciatedObject {
public:
	Object(std::string name, llvm::Type *t, Kontext& k, KCallArgList list)
		: InstanciatedObject(std::move(name), t, k)
		{
			list.emplace(list.cbegin(), KCallArg("ctor", nullptr));

            KFuncCall(std::make_shared<KIdentifier>(std::string(_name)), std::move(list)).codeGen(k);
		}
	virtual ~Object() {}

public:
	virtual llvm::Value * getStructElem(Kontext &kontext, std::string const &name) override { //TODO: do stuff here
		return kontext.getStructElem(this->get(kontext), name); // TODO: this is wrong
	}
	virtual void setStructElem(Kontext &kontext, std::string const &name, llvm::Value *val) override { //TODO: do stuff here
		auto field = this->getStructElem(kontext, name);
		new llvm::StoreInst(val, field, false, kontext.currentBlock());
	}
	virtual llvm::Value *get(Kontext &) const override { return _alloc; }
	virtual llvm::Value *load(Kontext &kontext) const override {
		return new llvm::LoadInst(_alloc, "", false, kontext.currentBlock());
	}

};

class Pointer: public InstanciatedObject {
public:
	Pointer(std::string name, llvm::Type *t, Kontext& k)
		: InstanciatedObject(std::move(name), t, k) {}

	virtual llvm::Value * getStructElem(Kontext &kontext, std::string const &name) override {
		if (_val->getType()->getPointerElementType()->isStructTy()) {
			return kontext.getStructElem(new llvm::LoadInst(_alloc, "", false, kontext.currentBlock()), name);
		}
	}

	virtual void setStructElem(Kontext &kontext, std::string const &name, llvm::Value *val) override { //TODO: do stuff here
		if (_val->getType()->getPointerElementType()->isStructTy()) {
			auto field = this->getStructElem(kontext, name);
			new llvm::StoreInst(val, field, false, kontext.currentBlock());
		}
	}

	virtual llvm::Value *get(Kontext &kontext) const override { 
		return new llvm::LoadInst(_alloc, "", false, kontext.currentBlock());
	}

	virtual llvm::Value *load(Kontext &kontext) const override {
		return new llvm::LoadInst(get(kontext), "", false, kontext.currentBlock());
	}
};