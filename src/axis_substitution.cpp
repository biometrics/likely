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

    bool runOnModule(Module &M) override
    {
        bool modified = false;

        for (Function &F : M.functions()) {
            map<Value* /* likely_mat */, CallInst* /* likely_new() */> origins;
            for (BasicBlock &B : F)
                for (Instruction &I : B)
                    if (CallInst *const call = dyn_cast<CallInst>(&I))
                        if (call->getCalledFunction()->getName() == "likely_new") {
                            origins.insert(pair<Value*, CallInst*>(call, call));
                            recursiveCastedOrigins(call, call, origins);
                        }

            for (const pair<Value*, CallInst*> &origin : origins)
                for (User *const user : origin.first->users())
                    if (GetElementPtrInst *const GEP = dyn_cast<GetElementPtrInst>(user)) {
                        // Special case optimized likely_matrix::data GEP
                        if (GEP->getNumOperands() < 3)
                            continue;

                        if (ConstantInt *const index = cast<ConstantInt>(GEP->getOperand(2))) {
                            const uint64_t i = index->getZExtValue();
                            assert(i >= 1); // We should never reference likely_matrix::ref_count

                            for (User *const user : GEP->users())
                                if (LoadInst *const load = dyn_cast<LoadInst>(user)) {
                                    load->replaceAllUsesWith(origin.second->getOperand(i - 1));
                                    modified = true;
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
