#pragma once

class KIdentifier: public KExpression {
public:
    KIdentifier() : _name("anon") {}
    KIdentifier(std::string &&name): _name(std::move(name)) {}

    inline std::string getName() const { return _name; }
    virtual ValuePtr codeGen(Kontext &);
    virtual llvm::Type *getType(Kontext &);

    virtual inline bool isAssignable() { return true; }

private:
    std::string _name;
};