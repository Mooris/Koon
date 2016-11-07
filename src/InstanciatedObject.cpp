#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include "InstanciatedObject.hh"

InstanciatedObject* InstanciatedObject::Create(	std::string const &name,
 												llvm::Type *t,
 												Kontext &kontext,
 												KCallArgList &&list){
	InstanciatedObject *object = nullptr;

	switch (t->getTypeID()) {
		case llvm::Type::VoidTyID:
			std::cout << name << " is void" << std::endl;
		break;
		case llvm::Type::PointerTyID:
			object = new Pointer(name, t, kontext);
		break;
		case llvm::Type::StructTyID:
			object = new Object(name, t, kontext, std::move(list));
		break;
		default:
			std::cout << name << " is unknown, typeID: " << t->getTypeID() << std::endl;
			return nullptr;
		break;
	}
	if (!object) {
		// hard shit happens TODO: Throw
		return nullptr;
	}
	
	return object;
}

InstanciatedObject* InstanciatedObject::Create(	std::string const &name,
 												llvm::Value *v,
 												Kontext &kontext,
 												KCallArgList &&list)
{
	auto obj = InstanciatedObject::Create(name, v->getType(), kontext, std::move(list));
	obj->setValue(v);
	return obj;
}
