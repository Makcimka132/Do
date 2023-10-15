/*
This Source Code Form is subject to the terms of the Mozilla
Public License, v. 2.0. If a copy of the MPL was not distributed
with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../../include/parser/nodes/NodeUnary.hpp"
#include "../../include/utils.hpp"
#include "../../include/parser/nodes/NodeBool.hpp"
#include "../../include/parser/nodes/NodeGet.hpp"
#include "../../include/parser/nodes/NodeIndex.hpp"
#include "../../include/parser/nodes/NodeIden.hpp"
#include "../../include/parser/nodes/NodeCall.hpp"
#include "../../include/parser/nodes/NodeBinary.hpp"
#include "../../include/parser/nodes/NodeStruct.hpp"
#include "../../include/parser/nodes/NodeFunc.hpp"
#include "../../include/parser/nodes/NodeVar.hpp"
#include "../../include/parser/nodes/NodeInt.hpp"
#include "../../include/parser/nodes/NodeFloat.hpp"
#include "../../include/parser/nodes/NodeBuiltin.hpp"
#include "../../include/parser/ast.hpp"

NodeUnary::NodeUnary(long loc, char type, Node* base) {
    this->loc = loc;
    this->type = type;
    this->base = base;
}

Type* NodeUnary::getType() {
    switch(this->type) {
        case TokType::GetPtr: return new TypePointer(this->base->getType());
        case TokType::Minus: case TokType::Ne: return this->base->getType();
        case TokType::Destructor: return new TypeVoid();
        case TokType::Multiply:
            Type* ty = this->base->getType();
            if(instanceof<TypePointer>(ty)) return ((TypePointer*)ty)->instance;
            if(instanceof<TypeArray>(ty)) return ((TypeArray*)ty)->element;
            return nullptr;
    }
    return nullptr;
}

LLVMValueRef NodeUnary::generatePtr() {
    if(instanceof<NodeIden>(this->base)) ((NodeIden*)this->base)->isMustBePtr = true;
    if(instanceof<NodeIndex>(this->base)) ((NodeIndex*)this->base)->isMustBePtr = true;
    if(instanceof<NodeGet>(this->base)) ((NodeGet*)this->base)->isMustBePtr = true;
    return this->base->generate();
}

void NodeUnary::check() {
    bool oldCheck = this->isChecked;
    this->isChecked = true;
    if(!oldCheck) this->base->check();
}

LLVMValueRef NodeUnary::generateConst() {
    if(this->type == TokType::Minus) {
        LLVMValueRef bs = this->base->generate();
        return (LLVMGetTypeKind(LLVMTypeOf(bs)) == LLVMIntegerTypeKind) ? LLVMConstNeg(bs) : LLVMConstFNeg(bs);
    }
    return nullptr;
}

LLVMValueRef NodeUnary::generate() {
    if(this->type == TokType::Minus) {
        LLVMValueRef bs = this->base->generate();
        return (LLVMGetTypeKind(LLVMTypeOf(bs)) == LLVMIntegerTypeKind) ? LLVMBuildNeg(generator->builder, bs, "NodeUnary_neg") : LLVMBuildFNeg(generator->builder, bs, "NodeUnary_fneg");
    }
    if(this->type == TokType::GetPtr) {
        LLVMValueRef val;
        if(instanceof<NodeGet>(this->base)) {
            ((NodeGet*)this->base)->isMustBePtr = true;
            val = this->base->generate();
        }
        else if(instanceof<NodeCall>(this->base)) {
            NodeCall* call = (NodeCall*)this->base;
            auto gcall = call->generate();
            auto temp = LLVMBuildAlloca(generator->builder, LLVMTypeOf(gcall), "NodeUnary_temp");
            LLVMBuildStore(generator->builder, gcall, temp);
            val = temp;
        }
        else if(instanceof<NodeBinary>(this->base)) val = generator->byIndex(this->base->generate(), std::vector<LLVMValueRef>({LLVMConstInt(LLVMInt32TypeInContext(generator->context), 0, false)}));
        /*else if(NodeSlice sl = base.instanceof!NodeSlice) {
            sl.isMustBePtr = true;
            val = sl.generate();
        }*/
        else if(instanceof<NodeIden>(this->base)) {
            NodeIden* id = ((NodeIden*)this->base);
            if(instanceof<TypeArray>(currScope->getVar(id->name, this->loc)->type)) {
                LLVMValueRef ptr = LLVMBuildInBoundsGEP(generator->builder, currScope->getWithoutLoad(id->name),
                    std::vector<LLVMValueRef>({LLVMConstInt(LLVMInt32Type(),0,false),LLVMConstInt(LLVMInt32Type(),0,false)}).data(),
                2, "NodeUnary_ingep");
                val = ptr;
            }
            else val = currScope->getWithoutLoad(id->name);
        }
        else if(instanceof<NodeIndex>(this->base)) {
            ((NodeIndex*)this->base)->isMustBePtr = true;
            val = this->base->generate();
        }
        if(LLVMGetTypeKind(LLVMTypeOf(val)) == LLVMPointerTypeKind && LLVMGetTypeKind(LLVMGetElementType(LLVMTypeOf(val))) == LLVMArrayTypeKind) {
            return LLVMBuildPointerCast(
                generator->builder,
                val,
                LLVMPointerType(LLVMGetElementType(LLVMGetElementType(LLVMTypeOf(val))),0),
                "NodeUnary_bitcast"
            );
        }
        return val;
    }
    if(this->type == TokType::Ne) return LLVMBuildNot(generator->builder, this->base->generate(), "NodeUnary_not");
    if(this->type == TokType::Multiply) {
        LLVMValueRef _base = this->base->generate();
        /*if(generator->settings.runtimeChecks) {
            LLVMValueRef isNull = LLVMBuildICmp(
                Generator.Builder,
                LLVMIntNE,
                LLVMBuildPtrToInt(Generator.Builder,_base,LLVMInt32TypeInContext(Generator.Context),toStringz("ptoi_")),
                LLVMBuildPtrToInt(Generator.Builder,new NodeNull().generate(),LLVMInt32TypeInContext(Generator.Context),toStringz("ptoi_")),
                toStringz("assert(p==null)_")
            );
            if(NeededFunctions["assert"].into(Generator.Functions)) LLVMBuildCall(Generator.Builder,Generator.Functions[NeededFunctions["assert"]],[isNull,new NodeString("Runtime error in '"~Generator.file~"' file on "~to!string(loc)~" line: attempt to use a null pointer in ptoi!\n",false).generate()].ptr,2,toStringz(""));
        }*/
        return LLVMBuildLoad(generator->builder, _base, "NodeUnary_multiply_load");
    }
    if(this->type == TokType::Destructor) {
        LLVMValueRef val2 = this->generatePtr();    
        if(LLVMGetTypeKind(LLVMTypeOf(val2)) != LLVMPointerTypeKind
        && LLVMGetTypeKind(LLVMTypeOf(val2)) != LLVMStructTypeKind) generator->error("the attempt to call the destructor is not in the structure!", this->loc); 
        if(LLVMGetTypeKind(LLVMTypeOf(val2)) == LLVMPointerTypeKind) {
            if(LLVMGetTypeKind(LLVMGetElementType(LLVMTypeOf(val2))) == LLVMPointerTypeKind) {
                if(LLVMGetTypeKind(LLVMGetElementType(LLVMGetElementType(LLVMTypeOf(val2)))) == LLVMStructTypeKind) val2 = LLVMBuildLoad(generator->builder, val2, "NodeCall_destructor_load");
            }
        }   
        std::string struc = std::string(LLVMGetStructName(LLVMGetElementType(LLVMTypeOf(val2))));   
        if(AST::structTable[struc]->destructor == nullptr) {
            if(instanceof<NodeIden>(this->base)) {
                NodeIden* id = (NodeIden*)this->base;
                if(currScope->localVars.find(id->name) == currScope->localVars.end() && !instanceof<TypeStruct>(currScope->localVars[id->name]->type)) {
                    return (new NodeCall(this->loc, new NodeIden("std::free", this->loc), {this->base}))->generate();
                }
                return nullptr;
            }
            return (new NodeCall(this->loc, new NodeIden("std::free", this->loc), {base}))->generate();
        }
        return (new NodeCall(this->loc, new NodeIden(AST::structTable[struc]->destructor->name, this->loc), {this->base}))->generate();
    }
    std::cout << "There! Operator " << (int)this->type << std::endl;
    return nullptr;
}

Node* NodeUnary::comptime() {
    //if(instanceof<NodeBuiltin>(this->base)) {
    //    std::cout << generator->file << ", " << ((NodeBuiltin*)this->base)->loc << ", " << ((NodeBuiltin*)this->base)->name << std::endl;
    //}
    switch(this->type) {
        case TokType::Minus:
            if(instanceof<NodeInt>(this->base)) return new NodeInt(-((NodeInt*)this->base)->value);
            return new NodeFloat(-((NodeFloat*)this->base)->value);
        case TokType::Ne: return new NodeBool(!(((NodeBool*)this->base->comptime()))->value);
        default: break;
    }
    return nullptr;
}

Node* NodeUnary::copy() {return new NodeUnary(this->loc, this->type, this->base->copy());}