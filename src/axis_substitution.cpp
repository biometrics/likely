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
    static int getStructGEPIndex(GetElementPtrInst *const GEP)
    {
        // Special case optimized likely_matrix::data GEP
        if (GEP->getNumOperands() < 3)
            return -1;

        ConstantInt *const index = dyn_cast<ConstantInt>(GEP->getOperand(2));
        if (!index)
            return -1;

        return int(index->getZExtValue());
    }

    bool runOnModule(Module &M) override
    {
        bool modified = false;

        for (Function &F : M.functions()) {
            // Local origins - replace all axis loads of matricies returned by likely_new with the
            // corresponding value passed to likely_new.
            for (const pair<Value*, CallInst*> &origin : getOrigins(F))
                for (User *const user : origin.first->users()) {
                    GetElementPtrInst *const GEP = dyn_cast<GetElementPtrInst>(user);
                    if (!GEP)
                        continue;

                    const int i = getStructGEPIndex(GEP);
                    if (i < 0)
                        continue;
                    assert(i >= 1); // We should never reference likely_matrix::ref_count

                    for (User *const user : GEP->users()) {
                        LoadInst *const load = dyn_cast<LoadInst>(user);
                        if (!load)
                            continue;

                        load->replaceAllUsesWith(origin.second->getOperand(i - 1));
                        modified = true;
                    }
                }

            // Remote origins - trace the origin of axis loads through likely_fork to simplify when possible.
            if (!F.hasNUses(1))
                continue;

            for (User *const bitcastUser : F.users().begin()->users()) {
                CallInst *const callInst = dyn_cast<CallInst>(bitcastUser);
                if (!callInst)
                    continue;

                Function *const function = callInst->getCalledFunction();
                if (!function || (function->getName() != "likely_fork"))
                    continue;

                DominatorTree dominatorTree = DominatorTreeAnalysis().run(F);

                AllocaInst *const allocaInst = cast<AllocaInst>(cast<CastInst>(callInst->getOperand(1))->getOperand(0));
                const map<Value*, CallInst*> origins = getOrigins(*allocaInst->getParent()->getParent());

                vector<Value*> remoteThunk(cast<StructType>(allocaInst->getType()->getElementType())->getNumElements(), NULL);
                for (User *const remoteThunkUser : allocaInst->users()) {
                    GetElementPtrInst *const GEP = dyn_cast<GetElementPtrInst>(remoteThunkUser);
                    if (!GEP)
                        continue;

                    const int i = getStructGEPIndex(GEP);
                    if (i < 0)
                        continue;

                    for (User *const gepUser : GEP->users()) {
                        StoreInst *const storeInst = dyn_cast<StoreInst>(gepUser);
                        if (!storeInst)
                            continue;
                        remoteThunk[i] = storeInst->getValueOperand();
                    }
                }

                vector<Value*> localThunk(remoteThunk.size(), NULL);
                for (User *const localThunkUser : F.args().begin()->users()) {
                    GetElementPtrInst *const GEP = dyn_cast<GetElementPtrInst>(localThunkUser);
                    if (!GEP)
                        continue;

                    const int i = getStructGEPIndex(GEP);
                    if (i < 0)
                        continue;

                    for (User *const gepUser : GEP->users()) {
                        LoadInst *const loadInst = dyn_cast<LoadInst>(gepUser);
                        if (!loadInst)
                            continue;
                        localThunk[i] = loadInst;
                    }
                }

                // Iterate over the thunk users in the callee
                for (User *const thunkUser : F.args().begin()->users()) {
                    GetElementPtrInst *const GEP = dyn_cast<GetElementPtrInst>(thunkUser);
                    if (!GEP)
                        continue;

                    const int i = getStructGEPIndex(GEP);
                    if (i < 0)
                        continue;

                    const auto &origin = origins.find(remoteThunk[i]);
                    if (origin == origins.end())
                        continue;

                    for (User *const gepUser : GEP->users()) {
                        LoadInst *const thunkElement = dyn_cast<LoadInst>(gepUser);
                        if (!thunkElement)
                            continue;

                        for (User *const thunkElementUser : thunkElement->users()) {
                            GetElementPtrInst *const GEP = dyn_cast<GetElementPtrInst>(thunkElementUser);
                            if (!GEP)
                                continue;

                            const int j = getStructGEPIndex(GEP);
                            if (j < 0)
                                continue;
                            assert(j >= 1); // We should never reference likely_matrix::ref_count

                            for (User *const gepUser : GEP->users()) {
                                LoadInst *const axisLoad = dyn_cast<LoadInst>(gepUser);
                                if (!axisLoad)
                                    continue;

                                // We found a load that we may be able to replace if we can trace its origin
                                LoadInst *const originLoad = dyn_cast<LoadInst>(origin->second->getOperand(j-1));
                                if (!originLoad)
                                    continue;

                                GetElementPtrInst *const originGEP = dyn_cast<GetElementPtrInst>(originLoad->getPointerOperand());
                                if (!originGEP)
                                    continue;

                                const int k = getStructGEPIndex(originGEP);
                                if (k < 0)
                                    continue;
                                assert(k >= 1); // We should never reference likely_matrix::ref_count

                                Value *const originMat = originGEP->getPointerOperand();
                                const size_t originIndex = find(remoteThunk.begin(), remoteThunk.end(), originMat) - remoteThunk.begin();
                                if (originIndex >= remoteThunk.size())
                                    continue;

                                Value *const localOrigin = localThunk[originIndex];
                                if (!localOrigin)
                                    continue;

                                // See if we can find a load for the same axis
                                bool success = false;
                                for (User *const localOriginUser : localOrigin->users()) {
                                    GetElementPtrInst *const localOriginGEP = dyn_cast<GetElementPtrInst>(localOriginUser);
                                    if (!localOriginGEP)
                                        continue;

                                    const int l = getStructGEPIndex(localOriginGEP);
                                    if (l < 0)
                                        continue;
                                    assert(l >= 1); // We should never reference likely_matrix::ref_count

                                    if (k != l)
                                        continue;

                                    for (User *const localOriginGEPuser : localOriginGEP->users()) {
                                        LoadInst *const replace = dyn_cast<LoadInst>(localOriginGEPuser);
                                        if (!replace)
                                            continue;
                                        if (dominatorTree.dominates(axisLoad, replace)) {
                                            replace->moveBefore(axisLoad);
                                            localOriginGEP->moveBefore(replace);
                                        }
                                        axisLoad->replaceAllUsesWith(replace);
                                        modified = true;
                                        success = true;
                                        break;
                                    }

                                    if (success)
                                        break;
                                }
                            }
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
