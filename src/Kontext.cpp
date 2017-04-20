#include "Kontext.hh"
#include "InstanciatedObject.hh"

void Kontext::createBuiltins() {
    std::vector<llvm::Type*> printf_arg_types;
    printf_arg_types.push_back(llvm::Type::getInt8PtrTy(this->getContext())); //char*

    llvm::FunctionType* printf_type =
        llvm::FunctionType::get(
            llvm::Type::getInt32Ty(this->getContext()), printf_arg_types, true);

    llvm::Function *func = llvm::Function::Create(
                printf_type, llvm::Function::ExternalLinkage,
                llvm::Twine("printf"),
                this->_module
           );
    func->setCallingConv(llvm::CallingConv::C);
    
	createInteger();
}

void Kontext::createInteger() {
	std::vector<llvm::Type*> types(1, llvm::Type::getInt32Ty(this->getContext()));
	auto leType = llvm::StructType::create(this->getContext(), makeArrayRef(types), "Integer", false);

    std::vector<KObjectAttr> attributes(1, KObjectAttr("innerInt", nullptr));
    auto o = KObject("Integer", leType, std::move(attributes));
    this->addType("Integer", o);

    this->setObject("Integer");

    std::vector<llvm::Type *> argTypes;

    argTypes.emplace_back(leType->getPointerTo());
    argTypes.emplace_back(llvm::Type::getInt32Ty(this->getContext()));

    auto fType = llvm::FunctionType::get(llvm::Type::getVoidTy(this->getContext()), makeArrayRef(argTypes), false);
    auto func = llvm::Function::Create(fType,
            llvm::GlobalValue::ExternalLinkage,
            "_KN7IntegerC1E",
            this->module());

    auto bBlock = llvm::BasicBlock::Create(this->getContext(),
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
    auto decl = std::make_shared<KVarDecl>("innerInt", llvm::Type::getInt32Ty(this->getContext()), &(*argIter));
    decl->setInObj();
    leBlock.emplaceStatement(std::move(decl));

    leBlock.codeGen(*this);

    llvm::ReturnInst::Create(this->getContext(),
                _blocks.top()->returnValue,
                bBlock);

    this->popBlock();

    this->createIntegerAdd();
    this->createIntegerPrint();

	this->popObject();
}

void Kontext::createIntegerAdd() {
	std::vector<llvm::Type *> argTypes;

    argTypes.emplace_back(this->_types["Integer"].type()->getPointerTo());
    argTypes.emplace_back(this->_types["Integer"].type()->getPointerTo());

    auto fType = llvm::FunctionType::get(this->_types["Integer"].type(), makeArrayRef(argTypes), false);
    auto func = llvm::Function::Create(fType,
            llvm::GlobalValue::ExternalLinkage,
            "_KN7IntegerplE",
            this->module());

    auto bBlock = llvm::BasicBlock::Create(this->getContext(),
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

    llvm::ReturnInst::Create(this->getContext(),
                new llvm::LoadInst(object->get(*this), "", false, this->currentBlock()),
                bBlock);

    this->popBlock();
}

void Kontext::createIntegerPrint() {
    std::vector<llvm::Type *> argTypes;

    argTypes.emplace_back(this->_types["Integer"].type()->getPointerTo());

    auto fType = llvm::FunctionType::get(llvm::Type::getVoidTy(this->getContext()), makeArrayRef(argTypes), false);
    auto func = llvm::Function::Create(fType,
            llvm::GlobalValue::ExternalLinkage,
            "_KN7Integer5printE",
            this->module());

    auto bBlock = llvm::BasicBlock::Create(this->getContext(),
                    "entry",
                    func,
                    nullptr);

    this->pushBlock(bBlock);

    auto argIter = func->arg_begin();

    auto obj = InstanciatedObject::Create("self", &(*argIter), *this, KCallArgList());
    (*argIter).setName("self");
    obj->store(*this);

    const char *constValue = "%d\n";
    llvm::Constant *format_const = llvm::ConstantDataArray::getString(this->getContext(), constValue);
    llvm::GlobalVariable *var =
        new llvm::GlobalVariable(
            *_module, llvm::ArrayType::get(llvm::IntegerType::get(this->getContext(), 8), strlen(constValue)+1),
            true, llvm::GlobalValue::PrivateLinkage, format_const, ".str");
    llvm::Constant *zero =
        llvm::Constant::getNullValue(llvm::IntegerType::getInt32Ty(this->getContext()));

    std::vector<llvm::Constant*> indices;
    indices.push_back(zero);
    indices.push_back(zero);
    llvm::Constant *var_ref = llvm::ConstantExpr::getGetElementPtr(
    llvm::ArrayType::get(llvm::IntegerType::get(this->getContext(), 8), 4),
        var, indices);

    std::vector<llvm::Value*> args;
    args.push_back(var_ref);
    args.push_back(new llvm::LoadInst(obj->getStructElem(*this, "innerInt"), "", false, this->currentBlock()));

    auto call = llvm::CallInst::Create(_module->getFunction("printf"), llvm::makeArrayRef(args), "", bBlock);
    llvm::ReturnInst::Create(this->getContext(), bBlock);
    this->popBlock();
}