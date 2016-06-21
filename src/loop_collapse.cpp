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

#include <llvm/Analysis/LoopPass.h>
#include <llvm/IR/IRBuilder.h>

using namespace llvm;
using namespace std;

namespace {

/*
 * // Collapse nested loops of the form:
 * const int y, // Outer loop iterations
 *           x, // Inner loop iterations
 *           t, // Outer offset, or zero
 *           s, // Inner offset, or zero
 *           u, // Scale, or one
 * for (int i=0; i<y; i++)
 *   for (int j=0; j<x; j++)
 *     f((t + i) * x * u + j * u + s)
 *
 * // Which after LICM looks like:
 * const int a = x * u;
 * for (int i=0; i<y; i++) {
 *   const int b = i + t;
 *   const int c = a * b;
 *   const int d = c + s
 *   for (int j=0; j<x; j++) {
 *     const int e = j * u;
 *     const int f = d + e;
 *     g(f);
 *   }
 * }
 *
 * // To:
 * for (int k=0; k<z; k++)
 *   g((v + k) * u + s)
 *
 * // Where:
 * v := t * x
 * k := i * x + j
 * z := x * y
 */
struct LoopCollapse : public LoopPass
{
    static char ID;
    LoopCollapse() : LoopPass(ID) {}

    struct LoopAnalysis
    {
        BasicBlock  *preheader  = NULL; // Loop preheader block
        BasicBlock  *header     = NULL; // Loop header block
        BasicBlock  *latchBlock = NULL; // Latch block
        BasicBlock  *exitBlock  = NULL; // Unique exit block
        PHINode     *IV         = NULL; // Induction variable
        BranchInst  *latch      = NULL; // Loop latch
        ICmpInst    *latchCmp   = NULL; // Latch comparison instruction
        Value       *exitVal    = NULL; // Loop invariant exit value
        AddOperator *increment  = NULL; // Induction variable increment instruction

        LoopAnalysis(Loop *const loop)
        {
            preheader  = loop->getLoopPreheader();
            header     = loop->getHeader();
            latchBlock = loop->getLoopLatch();
            if (loop->hasDedicatedExits())
                exitBlock = loop->getUniqueExitBlock();
            if (!preheader || !latchBlock || !exitBlock)
                return;

            latch = dyn_cast<BranchInst>(latchBlock->getTerminator());
            if (!latch)
                return;

            latchCmp = dyn_cast<ICmpInst>(latch->getOperand(0));
            if (!latchCmp)
                return;

            if (loop->isLoopInvariant(latchCmp->getOperand(0)))
                exitVal = latchCmp->getOperand(0);
            else if (loop->isLoopInvariant(latchCmp->getOperand(1)))
                exitVal = latchCmp->getOperand(1);
            else
                return;

            increment = dyn_cast<AddOperator>(latchCmp->getOperand(0) != exitVal ? latchCmp->getOperand(0)
                                                                                 : latchCmp->getOperand(1));
            if (!increment)
                return;

            if      (isOne(increment->getOperand(0))) IV = dyn_cast<PHINode>(increment->getOperand(1));
            else if (isOne(increment->getOperand(1))) IV = dyn_cast<PHINode>(increment->getOperand(0));
        }

        // For the analysis to be considered valid, all values must be non-null
        // and the latch instructions must not be used for other purposes.
        operator bool() const
        {
            return preheader && header && latchBlock && exitBlock && IV && latchCmp && exitVal && increment &&
                   increment->hasNUses(2) && latchCmp->hasOneUse();
        }
    };

    static bool isOne(Value *value)
    {
        if (ConstantInt *const constantInt = dyn_cast<ConstantInt>(value))
            return constantInt->isOne();
        return false;
    }

    bool runOnLoop(Loop *parentLoop, LPPassManager &) override
    {
        // We can only collapse single subloops
        if (parentLoop->getSubLoops().size() != 1)
            return false;

        // We will attempt to collapse `child` into `parent`
        Loop *const childLoop = parentLoop->getSubLoops().front();

        // Retrieve important values associated with the loops and ensure they
        // conform to our strict requirements.
        const LoopAnalysis parent(parentLoop);
        const LoopAnalysis child(childLoop);
        if (!parent || !child)
            return false;

        // The loops must have canonical induction variables of the same type
        if (parent.IV->getType() != child.IV->getType())
            return false;

        // See the comment above LoopCollapse for the definition of these member variables
        struct CollapsiblePattern
        {
            Value *t, *s, *u, *f;
            User *jUser;
            CollapsiblePattern(Value *const t, Value *const s, Value *const u, Value *const f, User *const jUser)
                : t(t), s(s), u(u), f(f), jUser(jUser) {}
        };

        // To be collapsible, we must be able to pattern match all uses of `i` (parent.CIV)
        vector<CollapsiblePattern> collapsiblePatterns;
        for (User *const maybeB : parent.IV->users()) {
            if (maybeB == parent.increment)
                continue;

            // Match `b = i + t`
            Value *b = NULL;
            Value *t = NULL;
            vector<User*> bUsers;
            if (AddOperator *const maybeIt = dyn_cast<AddOperator>(maybeB)) {
                Value *maybeT = NULL;
                if      (maybeIt->getOperand(0) == parent.IV) maybeT = maybeIt->getOperand(1);
                else if (maybeIt->getOperand(1) == parent.IV) maybeT = maybeIt->getOperand(0);
                if (maybeT && parentLoop->isLoopInvariant(maybeT)) {
                    b = maybeB;
                    t = maybeT;
                    for (User *const user : maybeIt->users())
                        bUsers.push_back(user);
                }
            }

            // Failed to match `b = i + t`, proceed under the assumption that `t = 0` and therefore `b = i`
            if (!b) {
                b = parent.IV;
                bUsers.push_back(maybeB);
            }

            for (User *const bUser : bUsers) {
                // Match `c = a * b`
                MulOperator *const c = dyn_cast<MulOperator>(bUser);
                if (!c)
                    return false; // Failed to match `c = _ * _`

                // Match `a = x * u`
                Value *a = NULL;
                Value *u = NULL;
                {
                    Value *const notB = c->getOperand(0) != b ? c->getOperand(0) : c->getOperand(1);
                    if (MulOperator *const maybeA = dyn_cast<MulOperator>(notB)) {
                        Value *maybeU = NULL;
                        if      (maybeA->getOperand(0) == child.exitVal) maybeU = maybeA->getOperand(1);
                        else if (maybeA->getOperand(1) == child.exitVal) maybeU = maybeA->getOperand(0);
                        if (maybeU && parentLoop->isLoopInvariant(maybeU)) {
                            a = maybeA;
                            u = maybeU;
                        }
                    } else if (ConstantInt *const maybeConstantA = dyn_cast<ConstantInt>(notB)) {
                        if (ConstantInt *const constantX = dyn_cast<ConstantInt>(child.exitVal)) {
                            const int64_t sa = maybeConstantA->getSExtValue();
                            const int64_t sx = constantX->getSExtValue();
                            if (sa == sx) {
                                a = maybeConstantA;
                            } else if (sa % sx == 0) {
                                a = maybeConstantA;
                                u = ConstantInt::get(maybeConstantA->getType(), sa / sx);
                            }
                        }
                    }
                }

                // Failed to match `a = x * u`, proceed under the assumption that `u = 1` and therefore `a = x`
                if (!a)
                    a = child.exitVal;

                if (!(((c->getOperand(0) == a) && (c->getOperand(1) == b)) ||
                      ((c->getOperand(1) == a) && (c->getOperand(0) == b))))
                    return false; // Failed to match `c = a * b`

                for (User *const cUser : c->users()) {
                    // Match `d = c + s`
                    Value *d = NULL;
                    Value *s = NULL;
                    vector<User*> dUsers;
                    if (AddOperator *const maybeD = dyn_cast<AddOperator>(cUser)) {
                        Value *const maybeS = maybeD->getOperand(0) != c ? maybeD->getOperand(0)
                                                                         : maybeD->getOperand(1);
                        if (parentLoop->isLoopInvariant(maybeS)) {
                            d = maybeD;
                            s = maybeS;
                            for (User *const user : d->users())
                                dUsers.push_back(user);
                        }
                    }

                    // Failed to match `d = c + s`, proceed under the assumption that `s = 0` and therefore `d = c`
                    if (!d) {
                        d = c;
                        dUsers.push_back(cUser);
                    }

                    for (User *const dUser : dUsers) {
                        // Match `f = d + e`
                        AddOperator *const f = dyn_cast<AddOperator>(dUser);
                        if (!f)
                            return false; // Failed to match `f = _ + _`

                        // Match `e = j * u`
                        Value *const e = f->getOperand(0) != d ? f->getOperand(0)
                                                               : f->getOperand(1);
                        if (u) {
                            MulOperator *const ju = dyn_cast<MulOperator>(e);
                            if (!ju)
                                return false; // Failed to match `e = _ * _`
                            if (!(((ju->getOperand(0) == child.IV) && (ju->getOperand(1) == u)) ||
                                  ((ju->getOperand(1) == child.IV) && (ju->getOperand(0) == u))))
                                return false; // Failed to match `e = j * u`
                        } else {
                            if (e != child.IV)
                                return false; // Failed to match `e = j` when `u = 1`
                        }

                        collapsiblePatterns.push_back(CollapsiblePattern(t, s, u, f, cast<User>(u ? e : f)));
                    }
                }
            }
        }

        // We must have pattern matched all uses of child.CIV
        for (User *const user : child.IV->users()) {
            if (user == child.increment)
                continue;

            bool found = false;
            for (const CollapsiblePattern &cp : collapsiblePatterns)
                if (cp.jUser == user) {
                    found = true;
                    break;
                }

            if (!found)
                return false;
        }

        // Hoist the child's exit value outside of the parent loop
        {
            bool changed;
            if (!parentLoop->makeLoopInvariant(child.exitVal, changed))
                return changed;
        }

        // At this point we have proven that child is collapsible into parent,
        // so lets do it!

        // Promote the Loop ID
        if (MDNode *const loopID = childLoop->getLoopID())
            parentLoop->setLoopID(loopID);

        // Scale parent.exitVal by child.exitVal
        IRBuilder<> builder(parent.preheader->getTerminator());
        Value *const z = builder.CreateMul(parent.exitVal, child.exitVal, "", true, true);
        parent.latchCmp->replaceUsesOfWith(parent.exitVal, z);

        // Replace each collapsible pattern
        for (const CollapsiblePattern &cp : collapsiblePatterns) {
            // Construct the new use right before the current use
            builder.SetInsertPoint(cast<Instruction>(cp.f));

            Value *replace = parent.IV;

            if (cp.t) {
                Value *v = NULL;

                // As a special case, try to pattern match `t = h * y`
                // In which case reorder:
                //   (* (* h y) x)
                // To:
                //   (* h (* y x))
                // Which equals:
                //   (* h z)
                // And facilitates collapsing additional nested loops if `s` is the grandparent loop's CIV.
                if (MulOperator *const hy = dyn_cast<MulOperator>(cp.t))
                    if (hy->hasOneUse() && ((hy->getOperand(0) == parent.exitVal) || (hy->getOperand(1) == parent.exitVal))) {
                        if (Instruction *const zInst = dyn_cast<Instruction>(z))
                            zInst->moveBefore(cast<Instruction>(hy));
                        hy->replaceUsesOfWith(parent.exitVal, z);
                        v = hy;
                    }

                if (!v)
                    v = builder.CreateMul(cp.t, child.exitVal, "", true, true);
                replace = builder.CreateAdd(v, replace, "", true, true);
            }

            if (cp.u)
                replace = builder.CreateMul(cp.u, replace, "", true, true);

            if (cp.s)
                replace = builder.CreateAdd(cp.s, replace, "", true, true);

            cp.f->replaceAllUsesWith(replace);
        }

        // Remove phi nodes references to the latch from the header
        for (Instruction &instruction : *child.header)
            if (PHINode *const phi = dyn_cast<PHINode>(&instruction))
                phi->removeIncomingValue(child.latchBlock);

        // Replace the child's latch with an unconditional branch
        builder.SetInsertPoint(child.latch);
        builder.CreateBr(child.exitBlock);
        child.latch->eraseFromParent();

        // Remove the postcondition and increment
        assert(child.latchCmp->hasNUses(0));
        child.latchCmp->eraseFromParent();
        assert(child.increment->hasNUses(0));
        cast<Instruction>(child.increment)->eraseFromParent();

        return true;
    }
};
char LoopCollapse::ID = 0;

} // namespace (anonymous)

LoopPass *createLoopCollapsePass()
{
    return new LoopCollapse();
}
