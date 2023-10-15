/*
This Source Code Form is subject to the terms of the Mozilla
Public License, v. 2.0. If a copy of the MPL was not distributed
with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../../include/parser/nodes/NodeWhile.hpp"
#include "../../include/parser/nodes/NodeRet.hpp"
#include "../../include/parser/nodes/NodeBool.hpp"
#include "../../include/parser/nodes/NodeFunc.hpp"
#include "../../include/parser/nodes/NodeIf.hpp"
#include "../../include/parser/nodes/NodeFor.hpp"
#include "../../include/utils.hpp"

NodeWhile::NodeWhile(Node* cond, Node* body, long loc, std::string funcName) {
    this->cond = cond;
    this->body = body;
    this->loc = loc;
    this->funcName = funcName;
}

Type* NodeWhile::getType() {return new TypeVoid();}
Node* NodeWhile::comptime() {return nullptr;}
Node* NodeWhile::copy() {return new NodeWhile(this->cond->copy(), this->body->copy(), this->loc, this->funcName);}

void NodeWhile::check() {
    bool oldCheck = this->isChecked;
    this->isChecked = true;

    if(this->body != nullptr && !oldCheck) {
        if(instanceof<NodeRet>(this->body)) ((NodeRet*)this->body)->parent = this->funcName;
        else if(instanceof<NodeBlock>(this->body)) {
            NodeBlock* nb = (NodeBlock*)this->body;
            for(int i=0; i<nb->nodes.size(); i++) {
                if(instanceof<NodeRet>(nb->nodes[i])) ((NodeRet*)nb->nodes[i])->parent = this->funcName;
                else if(instanceof<NodeIf>(nb->nodes[i])) ((NodeIf*)nb->nodes[i])->funcName = this->funcName;
                else if(instanceof<NodeWhile>(nb->nodes[i])) ((NodeWhile*)nb->nodes[i])->funcName = this->funcName;
                else if(instanceof<NodeFor>(nb->nodes[i])) ((NodeFor*)nb->nodes[i])->funcName = this->funcName;
            }
        }
    }
}

LLVMValueRef NodeWhile::generate() {
    LLVMBasicBlockRef condBlock = LLVMAppendBasicBlock(generator->functions[currScope->funcName], "cond");
    LLVMBasicBlockRef whileBlock = LLVMAppendBasicBlock(generator->functions[currScope->funcName], "while");
    currScope->blockExit = LLVMAppendBasicBlock(generator->functions[currScope->funcName], "exit");

    LLVMBuildBr(generator->builder, condBlock);
    LLVMPositionBuilderAtEnd(generator->builder, condBlock);
    LLVMBuildCondBr(generator->builder, this->cond->generate(), whileBlock, currScope->blockExit);
    LLVMPositionBuilderAtEnd(generator->builder, whileBlock);

    int selfNumber = generator->activeLoops.size();
    generator->activeLoops[selfNumber] = Loop{.isActive = true, .start = condBlock, .end = currScope->blockExit, .hasEnd = false, .isIf = false};

    generator->currBB = whileBlock;
    this->body->generate();
    if(!generator->activeLoops[selfNumber].hasEnd) LLVMBuildBr(generator->builder, condBlock);

    LLVMPositionBuilderAtEnd(generator->builder, generator->activeLoops[selfNumber].end);
    generator->activeLoops.erase(selfNumber);
    return nullptr;
}