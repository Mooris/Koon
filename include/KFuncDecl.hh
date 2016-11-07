#pragma once

#include "Nodes.hh"
#include "KIdentifier.hh"

class KFuncDecl: public KObjField {
    friend KFuncCall;
public:
    KFuncDecl() {};
    KFuncDecl(KIdentifier &&, KArgList &&, KBlock &&, Kontext *);
    virtual ~KFuncDecl() {}

    virtual ValuePtr codeGen(Kontext &);
    virtual bool isVariable() const { return false; };
    virtual void remangle(const KObject &);
    std::string getName() const { return _args[0].getCallName(); }

private:
	Kontext 	*_k;
    llvm::Function  *me;
    KIdentifier     _type;
    KArgList        _args;
    KBlock          _block;
};