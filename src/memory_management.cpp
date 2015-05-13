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
#include <llvm/Analysis/PostDominators.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <set>

using namespace llvm;
using namespace std;

namespace {

struct MemoryManagement : public ModulePass
{
    static char ID;
    MemoryManagement() : ModulePass(ID) {}

    void getAnalysisUsage(AnalysisUsage &analysisUsage) const override
    {
        analysisUsage.setPreservesCFG();
        analysisUsage.addRequired<PostDominatorTree>();
        analysisUsage.addPreserved<PostDominatorTree>();
    }

    static void recursiveCollectDecendents(Instruction *const I, set<Instruction*> &decendents)
    {
        decendents.insert(I);

        // Once the matrix element has been loaded we don't need to follow it
        if (isa<LoadInst>(I) || isa<PtrToIntInst>(I))
            return;

        for (User *const U : I->users()) {
            // Something has gone wrong if the user isn't an instruction
            Instruction *const J = cast<Instruction>(U);
            recursiveCollectDecendents(J, decendents);
        }
    }

    bool runOnModule(Module &M) override
    {
        bool modified = false;

        for (Function &F : M) {
            if (F.isDeclaration())
                continue;

            PostDominatorTree &PDT = getAnalysis<PostDominatorTree>(F);

            // Scan the function looking for calls to "likely_new"
            for (BasicBlock &BB : F)
                for (Instruction &I : BB) {
                    CallInst *const call = dyn_cast<CallInst>(&I);
                    if (!call)
                        continue;
                    if (call->getCalledFunction()->getName() != "likely_new")
                        continue;
                    set<Instruction*> decendents;
                    recursiveCollectDecendents(call, decendents);

                    bool isRetVal = false;
                    for (Instruction *const I : decendents)
                        if (isa<ReturnInst>(I)) {
                            isRetVal = true;
                            break;
                        }
                    if (isRetVal)
                        continue;

                    // At this point we've identified a matrix that we need to release,
                    // now we need to figure out where to put the call to "likely_release".
                    BasicBlock *postDominator = &F.getEntryBlock();
                    for (Instruction *const I : decendents)
                        postDominator = PDT.findNearestCommonDominator(postDominator, I->getParent());

                    Function *likelyRelease = M.getFunction("likely_release_mat");
                    if (!likelyRelease) {
                        FunctionType *const functionType = FunctionType::get(Type::getVoidTy(M.getContext()), Type::getInt8PtrTy(M.getContext()), false);
                        likelyRelease = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_release_mat", &M);
                        likelyRelease->setCallingConv(CallingConv::C);
                        likelyRelease->setDoesNotAlias(1);
                        likelyRelease->setDoesNotCapture(1);
                    }

                    IRBuilder<> builder(postDominator->getTerminator());
                    builder.CreateCall(likelyRelease, builder.CreatePointerCast(call, Type::getInt8PtrTy(M.getContext())));
                    modified = true;
                }
        }

        return modified;
    }
};
char MemoryManagement::ID = 0;

} // namespace (anonymous)

ModulePass *createMemoryManagementPass()
{
    return new MemoryManagement();
}
