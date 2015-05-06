/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright 2015 Joshua C. Klontz                                           *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License");           *
 * you may not use this file except in compliance with the License.          *
 * You may obtain a copy of the License at                                   *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 * Unless required by applicable law or agreed to in writing, software       *
 * distributed under the License is distributed on an "AS IS" BASIS,         *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
 * See the License for the specific language governing permissions and       *
 * limitations under the License.                                            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <llvm/Pass.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <iostream>
#include <map>

using namespace llvm;
using namespace std;

namespace {

struct AxisSubstitution : public ModulePass
{
    static char ID;
    AxisSubstitution() : ModulePass(ID) {}

private:
    void getAnalysisUsage(AnalysisUsage &analysisUsage) const override
    {
        analysisUsage.setPreservesCFG();
    }

    static void recursiveCastedOrigins(Value *const value, CallInst *const call, map<Value*, CallInst*> &origins)
    {
        for (User *const user : value->users())
            if (CastInst *const cast = dyn_cast<CastInst>(user)) {
                origins.insert(pair<Value*, CallInst*>(cast, call));
                recursiveCastedOrigins(cast, call, origins);
            }
    }

    static map<Value*, CallInst*> getOrigins(Function &F)
    {
        map<Value* /* likely_mat */, CallInst* /* likely_new() */> origins;
        for (BasicBlock &B : F)
            for (Instruction &I : B)
                if (CallInst *const call = dyn_cast<CallInst>(&I))
                    if (call->getCalledFunction()->getName() == "likely_new") {
                        origins.insert(pair<Value*, CallInst*>(call, call));
                        recursiveCastedOrigins(call, call, origins);
                    }
        return origins;
    }

    // Returns -1 on failure to pattern match
    static int getStructGEPIndex(Value *const value)
    {
        GetElementPtrInst *const GEP = dyn_cast<GetElementPtrInst>(value);
        if (!GEP)
            return -1;

        // Special case optimized likely_matrix::data GEP
        if (GEP->getNumOperands() < 3)
            return -1;

        ConstantInt *const index = dyn_cast<ConstantInt>(GEP->getOperand(2));
        if (!index)
            return -1;

        return int(index->getZExtValue());
    }

    static int getMatrixGEPIndex(Value *const value)
    {
        const int i = getStructGEPIndex(value);
        assert(i != 0); // We should never reference likely_matrix::ref_count
        return i;
    }

    bool runOnModule(Module &M) override
    {
        bool modified = false;

        for (Function &F : M.functions()) {
            // Local origins - Replace all axis loads of matricies returned by
            // likely_new with the corresponding value passed to likely_new.
            for (const pair<Value*, CallInst*> &origin : getOrigins(F))
                for (User *const user : origin.first->users()) {
                    const int i = getMatrixGEPIndex(user);
                    if (i < 0)
                        continue; // It's not a matrix GEP

                    for (User *const matrixGEPuser : user->users()) {
                        LoadInst *const load = dyn_cast<LoadInst>(matrixGEPuser);
                        if (!load)
                            continue;

                        load->replaceAllUsesWith(origin.second->getOperand(i - 1)); // We know that the origin dominates the load
                        modified = true;
                    }
                }

            // Remote origins - Trace the origin of axis values through
            // likely_fork and back again to simplify loads when possible.
            if (!F.hasOneUse())
                continue; // It can't be a thunk

            Constant *const thunkPointer = dyn_cast<Constant>(*F.users().begin());
            if (!thunkPointer || !thunkPointer->getType()->isPointerTy())
                continue; // It can't be a thunk

            if (!thunkPointer->hasOneUse())
                continue; // It can't be a thunk

            CallInst *const callInst = dyn_cast<CallInst>(*thunkPointer->users().begin());
            if (!callInst)
                continue;

            Function *const function = callInst->getCalledFunction();
            if (!function || (function->getName() != "likely_fork"))
                continue;

            // At this point we have identified both the thunk and it's caller
            AllocaInst *const allocaInst = cast<AllocaInst>(cast<CastInst>(callInst->getOperand(1))->getOperand(0));
            const map<Value*, CallInst*> origins = getOrigins(*allocaInst->getParent()->getParent());
            const DominatorTree dominatorTree = DominatorTreeAnalysis().run(F);
            const int numArgs = cast<StructType>(allocaInst->getType()->getElementType())->getNumElements();

            // Populate caller struct arguments
            vector<Value*> callerArgs(numArgs, NULL);
            for (User *const allocaInstUser : allocaInst->users()) {
                const int i = getStructGEPIndex(allocaInstUser);
                if (i < 0)
                    continue;

                for (User *const gepUser : allocaInstUser->users()) {
                    StoreInst *const storeInst = dyn_cast<StoreInst>(gepUser);
                    if (!storeInst)
                        continue;
                    callerArgs[i] = storeInst->getValueOperand();
                }
            }

            // Populate callee struct arguments
            vector<LoadInst*> calleeArgs(numArgs, NULL);
            for (User *const argUser : F.args().begin()->users()) {
                const int i = getStructGEPIndex(argUser);
                if (i < 0)
                    continue;

                for (User *const gepUser : argUser->users()) {
                    LoadInst *const loadInst = dyn_cast<LoadInst>(gepUser);
                    if (!loadInst)
                        continue;
                    calleeArgs[i] = loadInst;
                }
            }

            // Iterate over callee args
            for (int i=0; i<numArgs; i++) {
                const auto &origin = origins.find(callerArgs[i]);
                if (origin == origins.end())
                    continue; // We have no origin information for the corresponding argument in the caller,
                              // and thus no opportunity to optimize.

                Value *const caleeArg = calleeArgs[i];
                if (!caleeArg)
                    continue;

                for (User *const calleeArgUser : caleeArg->users()) {
                    const int matrixGEPIndex = getMatrixGEPIndex(calleeArgUser);
                    if (matrixGEPIndex < 0)
                        continue;

                    for (User *const gepUser : calleeArgUser->users()) {
                        LoadInst *const replaceable = dyn_cast<LoadInst>(gepUser);
                        if (!replaceable)
                            continue;

                        // At this point we've identified a load that we may be able to replace if we can trace its origin
                        LoadInst *const originLoad = dyn_cast<LoadInst>(origin->second->getOperand(matrixGEPIndex-1));
                        if (!originLoad)
                            continue;

                        GetElementPtrInst *const originGEP = dyn_cast<GetElementPtrInst>(originLoad->getPointerOperand());
                        if (!originGEP)
                            continue;

                        const int originStructGEPIndex = getMatrixGEPIndex(originGEP);
                        if (originStructGEPIndex < 0)
                            continue;

                        Value *const originMatrix = originGEP->getPointerOperand();
                        const size_t originIndex = find(callerArgs.begin(), callerArgs.end(), originMatrix) - callerArgs.begin();
                        if (originIndex >= callerArgs.size())
                            continue;

                        Value *const tracedOrigin = calleeArgs[originIndex];
                        if (!tracedOrigin)
                            continue;

                        // At this point we know which matrix and struct GEP index are equivalent to the replaceable load,
                        // let's look for it.
                        for (User *const tracedOriginUser : tracedOrigin->users()) {
                            GetElementPtrInst *const tracedOriginGEP = dyn_cast<GetElementPtrInst>(tracedOriginUser);
                            if (!tracedOriginGEP)
                                continue;

                            const int index = getMatrixGEPIndex(tracedOriginGEP);
                            if (index != originStructGEPIndex)
                                continue; // Not the index we are looking for

                            for (User *const tracedOriginGEPuser : tracedOriginGEP->users()) {
                                LoadInst *const replace = dyn_cast<LoadInst>(tracedOriginGEPuser);
                                if (!replace)
                                    continue;

                                // At this point we've found the replacement,
                                // make sure to move instructions accordingly
                                if (dominatorTree.dominates(replaceable, replace)) {
                                    replace->moveBefore(replaceable);
                                    tracedOriginGEP->moveBefore(replace);
                                }
                                replaceable->replaceAllUsesWith(replace);
                                modified = true;
                                break;
                            }

                            if (replaceable->hasNUses(0))
                                break; // The replacement occurred
                        }
                    }
                }
            }
        }

        return modified;
    }
};
char AxisSubstitution::ID = 0;

} // namespace (anonymous)

ModulePass *createAxisSubstitutionPass()
{
    return new AxisSubstitution();
}
