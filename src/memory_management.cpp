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
#include <llvm/Analysis/RegionInfo.h>
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
        analysisUsage.addRequired<RegionInfoPass>();
        analysisUsage.addPreserved<RegionInfoPass>();
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

            RegionInfo &RI = getAnalysis<RegionInfoPass>(F).getRegionInfo();

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

                    // Find the region that encompasses all the instructions
                    SmallVector<BasicBlock*, 8> blocks;
                    for (Instruction *const I : decendents) {
                        BasicBlock *const block = I->getParent();
                        if (find(blocks.begin(), blocks.end(), block) == blocks.end())
                            blocks.push_back(block);
                    }
                    Region *const region = RI.getCommonRegion(blocks);
                    assert(region);

                    // Figure out where to insert the release call(s)
                    vector<Instruction*> insertPoints;
                    if (BasicBlock *const exitBlock = region->getExit()) {
                        insertPoints.push_back(exitBlock->getFirstNonPHI());
                    } else {
                        // It's a top level region, scan for return instructions
                        for (BasicBlock *const BB : region->blocks()) {
                            TerminatorInst *const terminator = BB->getTerminator();
                            if (isa<ReturnInst>(terminator))
                                insertPoints.push_back(terminator);
                        }
                    }

                    // Get or insert "likely_release_mat"
                    Function *likelyRelease = M.getFunction("likely_release_mat");
                    if (!likelyRelease) {
                        FunctionType *const functionType = FunctionType::get(Type::getVoidTy(M.getContext()), Type::getInt8PtrTy(M.getContext()), false);
                        likelyRelease = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_release_mat", &M);
                        likelyRelease->setCallingConv(CallingConv::C);
                        likelyRelease->setDoesNotAlias(1);
                        likelyRelease->setDoesNotCapture(1);
                    }

                    for (Instruction *const insertPoint : insertPoints) {
                        IRBuilder<> builder(insertPoint);
                        builder.CreateCall(likelyRelease, builder.CreatePointerCast(call, Type::getInt8PtrTy(M.getContext())));
                    }

                    modified = true;
                }

            // Scan the function looking for return values that are arguments
            for (BasicBlock &BB : F)
                for (Instruction &I : BB) {
                    ReturnInst *const returnInst = dyn_cast<ReturnInst>(&I);
                    if (!returnInst)
                        continue;

                    Value *const returnValue = returnInst->getReturnValue();
                    if (!returnValue) // ret void
                        continue;

                    PointerType *const pointerType = dyn_cast<PointerType>(returnValue->getType());
                    if (!pointerType)
                        continue;

                    Value *origin = returnValue;
                    while (isa<LoadInst>(origin) || isa<CastInst>(origin)) {
                        if (isa<LoadInst>(origin))
                            origin = cast<LoadInst>(origin)->getPointerOperand();
                        else
                            origin = cast<CastInst>(origin)->getOperand(0);
                    }

                    if (!isa<Argument>(origin))
                        continue;

                    // Get or insert "likely_retain_mat"
                    Function *likelyRetain = M.getFunction("likely_retain_mat");
                    if (!likelyRetain) {
                        FunctionType *const functionType = FunctionType::get(Type::getInt8PtrTy(M.getContext()), Type::getInt8PtrTy(M.getContext()), false);
                        likelyRetain = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_retain_mat", &M);
                        likelyRetain->setCallingConv(CallingConv::C);
                        likelyRetain->setDoesNotAlias(1);
                        likelyRetain->setDoesNotCapture(1);
                    }

                    IRBuilder<> builder(returnInst);
                    Value *const retainedReturnValue = builder.CreatePointerCast(builder.CreateCall(likelyRetain, builder.CreatePointerCast(returnValue, Type::getInt8PtrTy(M.getContext()))), returnValue->getType());
                    returnInst->replaceUsesOfWith(returnValue, retainedReturnValue);

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
