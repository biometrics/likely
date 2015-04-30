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
#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/Analysis/CodeMetrics.h>

using namespace llvm;

namespace {

struct AssumptionSubstitution : public FunctionPass
{
    static char ID;
    AssumptionSubstitution() : FunctionPass(ID) {}

    void getAnalysisUsage(AnalysisUsage &analysisUsage) const override
    {
        analysisUsage.setPreservesCFG();
        analysisUsage.addPreserved<AssumptionCacheTracker>();
        analysisUsage.addRequired<AssumptionCacheTracker>();
    }

    bool runOnFunction(Function &F) override
    {
        AssumptionCache &AC = getAnalysis<AssumptionCacheTracker>().getAssumptionCache(F);

        SmallPtrSet<const Value*, 1> EphValues;
        CodeMetrics::collectEphemeralValues(&F, &AC, EphValues);

        bool modified = false;
        for (const auto &assumption : AC.assumptions())
            if (CallInst *const callInst = dyn_cast_or_null<CallInst>((Value*)assumption))
                if (CmpInst *const cmpInst = dyn_cast<CmpInst>(callInst->getOperand(0)))
                    if (cmpInst->getPredicate() == CmpInst::ICMP_EQ) {
                        Value *const find    = cmpInst->getOperand(0);
                        Value *const replace = cmpInst->getOperand(1);
                        for (User *const user : find->users())
                            if (!EphValues.count(user)) {
                                user->replaceUsesOfWith(find, replace);
                                modified = true;
                            }
                    }

        return modified;
    }
};
char AssumptionSubstitution::ID = 0;

} // namespace (anonymous)

FunctionPass *createAssumptionSubstitutionPass()
{
    return new AssumptionSubstitution();
}
