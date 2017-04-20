#pragma once

#include <string>
#include <vector>
#include <memory>

class KStatement;
class KFuncDecl;
class KArg;
class KCallArg;
class KObjField;

using StatementList = std::vector<std::shared_ptr<KStatement>>;
using KTopLevel = std::vector<std::shared_ptr<KStatement>>;
using KObjFieldList = std::vector<std::shared_ptr<KObjField>>;
using KArgList = std::vector<KArg>;
using KCallArgList = std::vector<KCallArg>;

class Kontext;
namespace llvm { class Value; class Type; class Module; class Function; }

using ValuePtr = llvm::Value *;

class KNode {
public:
    virtual ~KNode() {}
    virtual ValuePtr codeGen(Kontext &) = 0;
};

class KExpression: public KNode {
public:
    virtual llvm::Type *getType(Kontext &) = 0;
    virtual bool isAssignable() = 0;
};

class KStatement: public KNode {};


class KInt: public KExpression {
public:
    KInt(int i)
        : _inner(i) {}
    virtual ~KInt() {}

    virtual ValuePtr codeGen(Kontext &);
    virtual llvm::Type *getType(Kontext &);

    virtual inline bool isAssignable() { return false; }
private:
    int _inner;
};

class KReturnStatement: public KStatement {
public:
    KReturnStatement()
        : _expr(nullptr) {}
    KReturnStatement(std::shared_ptr<KExpression> &&expr)
        : _expr(std::move(expr)) {}
    virtual ~KReturnStatement() {}

    virtual ValuePtr    codeGen(Kontext &);

private:
    std::shared_ptr<KExpression>     _expr;
};

class KExpressionStatement: public KStatement {
public:
    KExpressionStatement(std::shared_ptr<KExpression> &&expr)
        : _expr(std::move(expr)) {}
    virtual ~KExpressionStatement() {}

    virtual ValuePtr codeGen(Kontext &);
private:
    std::shared_ptr<KExpression>     _expr;
};

#include "KIdentifier.hh"

class KOperator: public KExpression {
    template <class E>
    static constexpr auto ce(E &&e) noexcept {
        return static_cast<std::underlying_type_t<E>>(std::forward<E>(e));
    }
public:
    virtual ~KOperator() {}

    /* For bison */
    virtual ValuePtr codeGen(Kontext &) { return nullptr; }
    virtual llvm::Type* getType(Kontext &) { return nullptr; }

    virtual inline bool isAssignable() { return false; }
};

class KBinaryOperator: public KOperator {
public:
    KBinaryOperator(std::shared_ptr<KExpression> lhs,
                    std::shared_ptr<KExpression> rhs,
                    std::string opType)
        : _lhs(std::move(lhs)), _rhs(std::move(rhs)), _opType(opType) {}
    virtual ~KBinaryOperator() {}

public:
    virtual ValuePtr codeGen(Kontext &);
    virtual llvm::Type* getType(Kontext &);

    virtual bool isAssignable();

private:
    std::shared_ptr<KExpression>    _lhs;
    std::shared_ptr<KExpression>    _rhs;
    std::string                     _opType;
};

class KBlock: public KNode {
public:
    KBlock() {}
    virtual ~KBlock() {};

    inline  void emplaceStatement(std::shared_ptr<KStatement> &&stmt)
        { this->_stmts.push_back(std::move(stmt)); }

    virtual ValuePtr codeGen(Kontext &);

private:
    StatementList   _stmts;
};

class KArg: public KNode {
    friend class KFuncDecl;

public:
    KArg() {}
    KArg(KIdentifier &&name, KIdentifier &&type)
        : _name(std::move(name)), _type(std::move(type)), _callname(_name) {}
    KArg(KIdentifier &&name, KIdentifier &&type, KIdentifier &&callname)
        : _name(std::move(name)), _type(std::move(type)), _callname(std::move(callname)) {}

    virtual ~KArg() {}
    virtual ValuePtr codeGen(Kontext &);

    inline std::string getName() const { return _name.getName(); }
    inline std::string getType() const { return _type.getName(); }
    inline std::string getCallName() const { return _callname.getName(); }

private:
    KIdentifier _name;
    KIdentifier _type;
    KIdentifier _callname;
};

class KCallArg: public KNode { /* TODO: May have undefined behavior */
public:
    KCallArg() : _name("Broken"), _expr(nullptr), _value(nullptr) {}
    KCallArg(KIdentifier &&id, std::shared_ptr<KExpression> &&expr)
        : _name(std::move(id)), _expr(std::move(expr)), _value(nullptr) {}
    KCallArg(std::string name, llvm::Value *value)
        : _name(std::move(name)), _expr(nullptr), _value(std::move(value)) {}
public:
    inline bool haveExpr() const { return (_expr != nullptr) || (_value != nullptr); }
    inline std::string getName() const { return _name.getName(); }
    virtual ValuePtr codeGen(Kontext&);

private:
    KIdentifier     _name;
    std::shared_ptr<KExpression> _expr;
    llvm::Value*    _value;
};

class KFuncCall: public KExpression {
public:
    KFuncCall(std::shared_ptr<KExpression> object, KCallArgList &&args)
        : _object(std::move(object)), _args(std::move(args)) {}
public:
    virtual ValuePtr codeGen(Kontext&);
    virtual llvm::Type* getType(Kontext&);

    virtual bool isAssignable();

private:
    std::shared_ptr<KExpression>    _object;
    KCallArgList                    _args;
};
class KObject;
class KObjField: public KStatement {
public:
    virtual bool isVariable() const = 0;
    virtual void remangle(const KObject &) = 0;
    virtual std::string getName() const = 0;
    virtual std::shared_ptr<KExpression> getExpr() const {}
    virtual llvm::Type *getType(Kontext &) { return nullptr; }
    inline void setInObj() { this->_inObject = true; }

protected:
    bool _inObject = false;
};

class KVarDecl: public KObjField {
public:
    KVarDecl() : _name("broken"), _type(nullptr), _expr(nullptr) {}
    KVarDecl(std::string &&name, std::shared_ptr<KExpression> &&expr)
        :   _name(std::move(name)),
            _type(nullptr),
            _expr(std::move(expr)),
            _ltype(nullptr),
            _lvalue(nullptr) {}

    KVarDecl(std::string &&name, std::string &&te)
        :   _name(std::move(name)),
            _type(std::make_shared<std::string>(std::move(te))),
            _expr(nullptr),
            _ltype(nullptr),
            _lvalue(nullptr) {}

    KVarDecl(std::string &&name, std::string &&type, std::shared_ptr<KExpression> &&expr)
        :   _name(std::move(name)),
            _type(std::make_shared<std::string>(std::move(type))),
            _expr(std::move(expr)),
            _ltype(nullptr),
            _lvalue(nullptr) {}

    KVarDecl(std::string name, llvm::Type *type, llvm::Value *v)
        :   _name(std::move(name)),
            _type(nullptr),
            _expr(nullptr),
            _ltype(type),
            _lvalue(v) {}

    virtual ~KVarDecl() {}

public:
    virtual ValuePtr codeGen(Kontext &);
    virtual inline bool isVariable() const { return true; }
    virtual inline std::shared_ptr<KExpression> getExpr() const { return _expr; }
    virtual std::string getName() const { return _name; }
    virtual void remangle(const KObject &) {}
    virtual inline llvm::Type *getType(Kontext &);

private:
    std::string                     _name;
    std::shared_ptr<std::string>    _type;
    std::shared_ptr<KExpression>    _expr;

    llvm::Type*                     _ltype;
    llvm::Value*                    _lvalue;
};

#include "KFuncDecl.hh"

class KObjectDecl: public KObjField {
public:
    KObjectDecl() : _name("Broken") {}
    KObjectDecl(std::string &&, KObjFieldList &&, Kontext *);
    virtual ~KObjectDecl() {}

public:
    virtual ValuePtr codeGen(Kontext &);
    virtual bool isVariable() const { return false; };
    virtual void remangle(const KObject &);
    virtual std::string getName() const { return _name; }

private:
    std::string _name;
    KObjFieldList   _stmts;
    std::shared_ptr<KFuncDecl> _ctor = nullptr;
};
