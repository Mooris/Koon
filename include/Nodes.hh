#pragma once

#include <string>
#include <vector>
#include <memory>

class KStatement;
class KFuncDecl;
class KArg;
class KCallArg;

using StatementList = std::vector<std::shared_ptr<KStatement>>;
using KFuncList = std::vector<KFuncDecl>;
using KArgList = std::vector<KArg>;
using KCallArgList = std::vector<KCallArg>;

class Kontext;
namespace llvm { class Value; class Type; class Module; }

using ValuePtr = llvm::Value *;

class KNode {
public:
    virtual ~KNode() {}
    virtual ValuePtr codeGen(Kontext &) = 0;
};

class KExpression: public KNode {
public:
    virtual llvm::Type *getType(Kontext &) = 0;
};

class KStatement: public KNode {};


class KInt: public KExpression {
public:
    KInt(int i)
        : _inner(i) {}
    virtual ~KInt() {}

    virtual ValuePtr codeGen(Kontext &);
    virtual llvm::Type *getType(Kontext &);

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

class KIdentifier: public KExpression {
public:
    KIdentifier() : _name("anon") {}
    KIdentifier(std::string &&name): _name(std::move(name)) {}

    inline std::string getName() const { return _name; }
    virtual ValuePtr codeGen(Kontext &);
    virtual llvm::Type *getType(Kontext &);

private:
    std::string _name;
};

class KAssignment: public KExpression {
public:
    KAssignment(KIdentifier lhs, std::shared_ptr<KExpression> rhs)
        : _lhs(std::move(lhs)), _rhs(std::move(rhs)) {}

    virtual ValuePtr codeGen(Kontext &);
    virtual llvm::Type* getType(Kontext &);

private:
    KIdentifier                     _lhs;
    std::shared_ptr<KExpression>    _rhs;
};

class KVarDecl: public KStatement {
public:
    KVarDecl() : _name("broken"), _type(nullptr), _expr(nullptr) {}
    KVarDecl(KIdentifier &&name, std::shared_ptr<KExpression> &&expr)
        :   _name(std::move(name)),
            _type(nullptr),
            _expr(std::move(expr)) {}

    KVarDecl(KIdentifier &&name, std::string &&te)
        :   _name(std::move(name)),
            _type(std::make_shared<std::string>(std::move(te))),
            _expr(nullptr) {}

    KVarDecl(KIdentifier &&name, std::string &&type, std::shared_ptr<KExpression> &&expr)
        :   _name(std::move(name)),
            _type(std::make_shared<std::string>(std::move(type))),
            _expr(std::move(expr)) {}

    virtual ~KVarDecl() {}

public:
    virtual ValuePtr codeGen(Kontext &);

private:
    KIdentifier                     _name;
    std::shared_ptr<std::string>    _type;
    std::shared_ptr<KExpression>    _expr;
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

class KFuncDecl: public KStatement {
public:
    KFuncDecl() {};
    KFuncDecl(KIdentifier &&, KArgList &&, KBlock &&, llvm::Module*);
    virtual ~KFuncDecl() {}

    virtual ValuePtr codeGen(Kontext &);

private:
    KIdentifier     _type;
    KArgList        _args;
    KBlock          _block;
};

class KCallArg: public KNode {
public:
    KCallArg() : _name("Broken"), _expr(nullptr) {}
    KCallArg(KIdentifier &&id, std::shared_ptr<KExpression> &&expr)
        : _name(std::move(id)), _expr(std::move(expr)) {}

public:
    inline bool haveExpr() const { return _expr != nullptr; }
    inline std::string getName() const { return _name.getName(); }
    virtual ValuePtr codeGen(Kontext&);

private:
    KIdentifier     _name;
    std::shared_ptr<KExpression> _expr;
};

class KFuncCall: public KExpression {
public:
    KFuncCall(KIdentifier &&object, KCallArgList &&args)
        : _object(std::move(object)), _args(std::move(args)) {}
public:
    virtual ValuePtr codeGen(Kontext&);
    virtual llvm::Type* getType(Kontext&);

private:
    KIdentifier         _object;
    KCallArgList        _args;
};
