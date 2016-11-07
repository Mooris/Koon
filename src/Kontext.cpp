#include "Kontext.hh"
#include "InstanciatedObject.hh"

void Kontext::createBuiltins() {
	createInteger();
}

void Kontext::createInteger() {
	std::vector<llvm::Type*> types(1, llvm::Type::getInt32Ty(llvm::getGlobalContext()));
	auto leType = llvm::StructType::create(llvm::getGlobalContext(), makeArrayRef(types), "Integer", false);

    std::vector<KObjectAttr> attributes(1, KObjectAttr("innerInt", nullptr));
    auto o = KObject("Integer", leType, std::move(attributes));
    this->addType("Integer", o);

    this->setObject("Integer");

    std::vector<llvm::Type *> argTypes;

    argTypes.emplace_back(leType->getPointerTo());
    argTypes.emplace_back(llvm::Type::getInt32Ty(llvm::getGlobalContext()));

    auto fType = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm::getGlobalContext()), makeArrayRef(argTypes), false);
    auto func = llvm::Function::Create(fType,
            llvm::GlobalValue::ExternalLinkage,
            "Integer_Integer",
            this->module());

    auto bBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(),
                    "entry",
                    func,
                    nullptr);

    this->pushBlock(bBlock);

	auto argIter = func->arg_begin();    

    auto obj = InstanciatedObject::Create("self", &(*argIter), *this, KCallArgList());
    (*argIter).setName("self");
    obj->store(*this);
	argIter++;

	(*argIter).setName("leValue");


    auto leBlock = KBlock();
    auto decl = std::make_shared<KVarDecl>("innerInt", llvm::Type::getInt32Ty(llvm::getGlobalContext()), &(*argIter));
    decl->setInObj();
    leBlock.emplaceStatement(std::move(decl));

    leBlock.codeGen(*this);

    llvm::ReturnInst::Create(llvm::getGlobalContext(),
                _blocks.top()->returnValue,
                bBlock);

    this->popBlock();

    this->createIntegerAdd();

	this->popObject();
}

void Kontext::createIntegerAdd() {
	std::vector<llvm::Type *> argTypes;

    argTypes.emplace_back(this->_types["Integer"].type()->getPointerTo());
    argTypes.emplace_back(this->_types["Integer"].type()->getPointerTo());

    auto fType = llvm::FunctionType::get(this->_types["Integer"].type(), makeArrayRef(argTypes), false);
    auto func = llvm::Function::Create(fType,
            llvm::GlobalValue::ExternalLinkage,
            "Integer_operator+",
            this->module());

    auto bBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(),
                    "entry",
                    func,
                    nullptr);

    this->pushBlock(bBlock);

	auto argIter = func->arg_begin();    

    auto obj = InstanciatedObject::Create("self", &(*argIter), *this, KCallArgList());
    (*argIter).setName("self");
    obj->store(*this);
	argIter++;

    auto rhs = InstanciatedObject::Create("rhs", &(*argIter), *this, KCallArgList());
    (*argIter).setName("rhs");
    rhs->store(*this);

    auto binOp = llvm::BinaryOperator::Create(	llvm::Instruction::Add,
    											new llvm::LoadInst(obj->getStructElem(*this, "innerInt"), "", false, this->currentBlock()),
    											new llvm::LoadInst(rhs->getStructElem(*this, "innerInt"), "", false, this->currentBlock()),
    											"",	this->_blocks.top()->block);

    auto type = this->type_of("Integer");
    auto object = InstanciatedObject::Create("temp", type, *this, KCallArgList(1, KCallArg("Integer", binOp)));

    llvm::ReturnInst::Create(llvm::getGlobalContext(),
                new llvm::LoadInst(object->get(*this), "", false, this->currentBlock()),
                bBlock);

    this->popBlock();
}