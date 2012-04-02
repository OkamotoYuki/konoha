/****************************************************************************
 * KONOHA COPYRIGHT, LICENSE NOTICE, AND DISCRIMER
 *
 * Copyright (c) 2011-2012 Masahiro Ide <imasahiro9 at gmail.com>
 * All rights reserved.
 * You may choose one of the following two licenses when you use konoha.
 * If you want to use the latter license, please contact us.
 *
 * (1) GNU General Public License 3.0 (with K_UNDER_GPL)
 * (2) Konoha Non-Disclosure License 1.0
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Intrinsics.h>
#include <llvm/Attributes.h>
#include <llvm/PassManager.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/UnifyFunctionExitNodes.h>
#ifdef USE_LLVM_3_1
#include <llvm/Transforms/Vectorize.h>
#endif
#include <llvm/Analysis/Verifier.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/DomPrinter.h>
#include <llvm/Analysis/RegionPass.h>
#include <llvm/Analysis/RegionPrinter.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/Lint.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/system_error.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/ADT/OwningPtr.h>
#include <llvm/ADT/Triple.h>

#include <iostream>

#undef HAVE_SYS_TYPES_H
#undef HAVE_SYS_STAT_H
#undef HAVE_UNISTD_H
#undef HAVE_SYS_TIME_H
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_VERSION
#include <konoha2/konoha2.h>
#include <konoha2/sugar.h>

namespace konoha {
template <class T>
inline T object_cast(kRawPtr *po)
{
	return static_cast<T>(po->rawptr);
}

template <class T>
inline void convert_array(std::vector<T> &vec, kArray *a)
{
	size_t size = a->size;
	for (size_t i=0; i < size; i++) {
		T v = konoha::object_cast<T>(a->ptrs[i]);
		vec.push_back(v);
	}
}
}

using namespace llvm;

#ifdef __cplusplus
extern "C" {
#endif

#define LLVM_TODO(str) do {\
	fprintf(stderr, "(TODO: %s %d):", __func__, __LINE__);\
	fprintf(stderr, "%s\n", str);\
	abort();\
} while (0)

#define LLVM_WARN(str) do {\
	fprintf(stderr, "(WARN: %s %d):", __func__, __LINE__);\
	fprintf(stderr, "%s\n", str);\
} while (0)

#define _UNUSED_ __attribute__((unused))

#define PKG_NULVAL(T) PKG_NULVAL_##T
#define PKG_NULVAL_int    (0)
#define PKG_NULVAL_float  (0.0)
#define PKG_NULVAL_String (KNH_NULVAL(CLASS_String))
#define WRAP(ptr) ((void*)ptr)
#define Int_to(T, a)               ((T)a.ivalue)
#define DEFAPI(T) T

static void Type_init(CTX _UNUSED_, kRawPtr *po, void *conf)
{
	po->rawptr = conf;
}
static void Type_free(CTX _UNUSED_, kRawPtr *po)
{
	po->rawptr = NULL;
}

static inline kRawPtr *new_ReturnCppObject(CTX, ksfp_t *sfp, void *ptr _RIX)
{
	kObject *defobj = sfp[K_RIX].o;
	kObject *ret = new_kObject(O_ct(defobj), ptr);
	return (kRawPtr*) ret;
}

//## @Static Type Type.getVoidTy();
static KMETHOD Type_getVoidTy(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getVoidTy(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static Type Type.getLabelTy();
static KMETHOD Type_getLabelTy(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getLabelTy(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static Type Type.getFloatTy();
static KMETHOD Type_getFloatTy(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getFloatTy(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static Type Type.getDoubleTy();
static KMETHOD Type_getDoubleTy(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getDoubleTy(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static Type Type.getMetadataTy();
static KMETHOD Type_getMetadataTy(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getMetadataTy(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static Type Type.getX86FP80Ty();
static KMETHOD Type_getX86FP80Ty(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getX86_FP80Ty(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static Type Type.getFP128Ty();
static KMETHOD Type_getFP128Ty(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getFP128Ty(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static Type Type.getPPCFP128Ty();
static KMETHOD Type_getPPCFP128Ty(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getPPC_FP128Ty(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static Type Type.getX86MMXTy();
static KMETHOD Type_getX86MMXTy(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getX86_MMXTy(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static IntegerType Type.getInt1Ty();
static KMETHOD Type_getInt1Ty(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getInt1Ty(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static IntegerType Type.getInt8Ty();
static KMETHOD Type_getInt8Ty(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getInt8Ty(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static IntegerType Type.getInt16Ty();
static KMETHOD Type_getInt16Ty(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getInt16Ty(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static IntegerType Type.getInt32Ty();
static KMETHOD Type_getInt32Ty(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getInt32Ty(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static IntegerType Type.getInt64Ty();
static KMETHOD Type_getInt64Ty(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getInt64Ty(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static PointerType Type.getFloatPtrTy();
static KMETHOD Type_getFloatPtrTy(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getFloatPtrTy(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static PointerType Type.getDoublePtrTy();
static KMETHOD Type_getDoublePtrTy(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getDoublePtrTy(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static PointerType Type.getX86FP80PtrTy();
static KMETHOD Type_getX86FP80PtrTy(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getX86_FP80PtrTy(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static PointerType Type.getFP128PtrTy();
static KMETHOD Type_getFP128PtrTy(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getFP128PtrTy(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static PointerType Type.getPPCFP128PtrTy();
static KMETHOD Type_getPPCFP128PtrTy(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getPPC_FP128PtrTy(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static PointerType Type.getX86MMXPtrTy();
static KMETHOD Type_getX86MMXPtrTy(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getX86_MMXPtrTy(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static PointerType Type.getInt1PtrTy();
static KMETHOD Type_getInt1PtrTy(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getInt1PtrTy(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static PointerType Type.getInt8PtrTy();
static KMETHOD Type_getInt8PtrTy(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getInt8PtrTy(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static PointerType Type.getInt16PtrTy();
static KMETHOD Type_getInt16PtrTy(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getInt16PtrTy(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static PointerType Type.getInt32PtrTy();
static KMETHOD Type_getInt32PtrTy(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getInt32PtrTy(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static PointerType Type.getInt64PtrTy();
static KMETHOD Type_getInt64PtrTy(CTX, ksfp_t *sfp _RIX)
{
	const Type *ptr = Type::getInt64PtrTy(getGlobalContext());
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static PointerType PointerType.get(Type type);
static KMETHOD PointerType_get(CTX, ksfp_t *sfp _RIX)
{
	Type *type = konoha::object_cast<Type *>(sfp[1].p);
	const Type *ptr  = PointerType::get(type, 0);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## IRBuilder IRBuilder.new(BasicBlock bb);
static KMETHOD IRBuilder_new(CTX, ksfp_t *sfp _RIX)
{
	BasicBlock *bb = konoha::object_cast<BasicBlock *>(sfp[1].p);
	IRBuilder<> *self = new IRBuilder<>(bb);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(self) K_RIXPARAM);
	RETURN_(p);
}

//## ReturnInst IRBuilder.CreateRetVoid();
static KMETHOD IRBuilder_createRetVoid(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	ReturnInst *ptr = self->CreateRetVoid();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ReturnInst IRBuilder.CreateRet(Value V);
static KMETHOD IRBuilder_createRet(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	ReturnInst *ptr = self->CreateRet(V);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

////## ReturnInst IRBuilder.CreateAggregateRet(Value retVals, int N);
//KMETHOD IRBuilder_createAggregateRet(CTX, ksfp_t *sfp _RIX)
//{
//	LLVM_TODO("NO SUPPORT");
//	//IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
//	//Value *const retVals = konoha::object_cast<Value *const>(sfp[1].p);
//	//kint_t N = Int_to(kint_t,sfp[2]);
//	//ReturnInst *ptr = self->CreateAggregateRet(retVals, N);
//	//kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
//	//RETURN_(p);
//}

//## BranchInst IRBuilder.CreateBr(BasicBlock Dest);
static KMETHOD IRBuilder_createBr(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	BasicBlock *Dest = konoha::object_cast<BasicBlock *>(sfp[1].p);
	BranchInst *ptr = self->CreateBr(Dest);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## BranchInst IRBuilder.CreateCondBr(Value Cond, BasicBlock True, BasicBlock False);
static KMETHOD IRBuilder_createCondBr(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Cond = konoha::object_cast<Value *>(sfp[1].p);
	BasicBlock *True = konoha::object_cast<BasicBlock *>(sfp[2].p);
	BasicBlock *False = konoha::object_cast<BasicBlock *>(sfp[3].p);
	BranchInst *ptr = self->CreateCondBr(Cond, True, False);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## SwitchInst IRBuilder.CreateSwitch(Value V, BasicBlock Dest);
static KMETHOD IRBuilder_createSwitch(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	BasicBlock *Dest = konoha::object_cast<BasicBlock *>(sfp[2].p);
	SwitchInst *ptr = self->CreateSwitch(V, Dest);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## IndirectBrInst IRBuilder.CreateIndirectBr(Value Addr);
static KMETHOD IRBuilder_createIndirectBr(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Addr = konoha::object_cast<Value *>(sfp[1].p);
	IndirectBrInst *ptr = self->CreateIndirectBr(Addr);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## InvokeInst IRBuilder.CreateInvoke0(Value Callee, BasicBlock NormalDest, BasicBlock UnwindDest);
static KMETHOD IRBuilder_createInvoke0(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Callee = konoha::object_cast<Value *>(sfp[1].p);
	BasicBlock *NormalDest = konoha::object_cast<BasicBlock *>(sfp[2].p);
	BasicBlock *UnwindDest = konoha::object_cast<BasicBlock *>(sfp[3].p);
	InvokeInst *ptr = self->CreateInvoke(Callee, NormalDest, UnwindDest);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## InvokeInst IRBuilder.CreateInvoke1(Value Callee, BasicBlock NormalDest, BasicBlock UnwindDest, Value Arg1);
static KMETHOD IRBuilder_createInvoke1(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Callee = konoha::object_cast<Value *>(sfp[1].p);
	BasicBlock *NormalDest = konoha::object_cast<BasicBlock *>(sfp[2].p);
	BasicBlock *UnwindDest = konoha::object_cast<BasicBlock *>(sfp[3].p);
	Value *Arg1 = konoha::object_cast<Value *>(sfp[4].p);
	InvokeInst *ptr = self->CreateInvoke(Callee, NormalDest, UnwindDest, Arg1);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## InvokeInst IRBuilder.CreateInvoke3(Value Callee, BasicBlock NormalDest, BasicBlock UnwindDest, Value Arg1, Value Arg2, Value Arg3);
static KMETHOD IRBuilder_createInvoke3(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Callee = konoha::object_cast<Value *>(sfp[1].p);
	BasicBlock *NormalDest = konoha::object_cast<BasicBlock *>(sfp[2].p);
	BasicBlock *UnwindDest = konoha::object_cast<BasicBlock *>(sfp[3].p);
	Value *Arg1 = konoha::object_cast<Value *>(sfp[4].p);
	Value *Arg2 = konoha::object_cast<Value *>(sfp[5].p);
	Value *Arg3 = konoha::object_cast<Value *>(sfp[6].p);
	InvokeInst *ptr = self->CreateInvoke3(Callee, NormalDest, UnwindDest, Arg1, Arg2, Arg3);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

////## InvokeInst IRBuilder.CreateInvoke(Value Callee, BasicBlock NormalDest, BasicBlock UnwindDest, ArrayRef<Value> Args);
//KMETHOD IRBuilder_createInvoke(CTX, ksfp_t *sfp _RIX)
//{
//	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
//	Value *Callee = konoha::object_cast<Value *>(sfp[1].p);
//	BasicBlock *NormalDest = konoha::object_cast<BasicBlock *>(sfp[2].p);
//	BasicBlock *UnwindDest = konoha::object_cast<BasicBlock *>(sfp[3].p);
//	kArray *Args = (sfp[4].a);
//	std::vector<Value*> List;
//	konoha::convert_array(List, Args);
//	InvokeInst *ptr = self->CreateInvoke(Callee, NormalDest, UnwindDest, List.begin(), List.end());
//	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
//	RETURN_(p);
//}

////## ResumeInst IRBuilder.CreateResume(Value Exn);
//KMETHOD IRBuilder_createResume(CTX, ksfp_t *sfp _RIX)
//{
//	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
//	Value *Exn = konoha::object_cast<Value *>(sfp[1].p);
//	ResumeInst *ptr = self->CreateResume(Exn);
//	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
//	RETURN_(p);
//}
//
//## UnreachableInst IRBuilder.CreateUnreachable();
static KMETHOD IRBuilder_createUnreachable(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	UnreachableInst *ptr = self->CreateUnreachable();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateAdd(Value LHS, Value RHS);
static KMETHOD IRBuilder_createAdd(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateAdd(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateNSWAdd(Value LHS, Value RHS);
static KMETHOD IRBuilder_createNSWAdd(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateNSWAdd(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateNUWAdd(Value LHS, Value RHS);
static KMETHOD IRBuilder_createNUWAdd(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateNUWAdd(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFAdd(Value LHS, Value RHS);
static KMETHOD IRBuilder_createFAdd(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateFAdd(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateSub(Value LHS, Value RHS);
static KMETHOD IRBuilder_createSub(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateSub(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateNSWSub(Value LHS, Value RHS);
static KMETHOD IRBuilder_createNSWSub(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateNSWSub(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateNUWSub(Value LHS, Value RHS);
static KMETHOD IRBuilder_createNUWSub(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateNUWSub(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFSub(Value LHS, Value RHS);
static KMETHOD IRBuilder_createFSub(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateFSub(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateMul(Value LHS, Value RHS);
static KMETHOD IRBuilder_createMul(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateMul(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateNSWMul(Value LHS, Value RHS);
static KMETHOD IRBuilder_createNSWMul(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateNSWMul(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateNUWMul(Value LHS, Value RHS);
static KMETHOD IRBuilder_createNUWMul(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateNUWMul(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFMul(Value LHS, Value RHS);
static KMETHOD IRBuilder_createFMul(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateFMul(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateUDiv(Value LHS, Value RHS);
static KMETHOD IRBuilder_createUDiv(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateUDiv(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateExactUDiv(Value LHS, Value RHS);
static KMETHOD IRBuilder_createExactUDiv(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateExactUDiv(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateSDiv(Value LHS, Value RHS);
static KMETHOD IRBuilder_createSDiv(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateSDiv(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateExactSDiv(Value LHS, Value RHS);
static KMETHOD IRBuilder_createExactSDiv(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateExactSDiv(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFDiv(Value LHS, Value RHS);
static KMETHOD IRBuilder_createFDiv(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateFDiv(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateURem(Value LHS, Value RHS);
static KMETHOD IRBuilder_createURem(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateURem(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateSRem(Value LHS, Value RHS);
static KMETHOD IRBuilder_createSRem(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateSRem(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFRem(Value LHS, Value RHS);
static KMETHOD IRBuilder_createFRem(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateFRem(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateShl(Value LHS, Value RHS);
static KMETHOD IRBuilder_createShl(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateShl(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateLShr(Value LHS, Value RHS);
static KMETHOD IRBuilder_createLShr(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateLShr(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateAShr(Value LHS, Value RHS);
static KMETHOD IRBuilder_createAShr(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateAShr(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateAnd(Value LHS, Value RHS);
static KMETHOD IRBuilder_createAnd(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateAnd(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateOr(Value LHS, Value RHS);
static KMETHOD IRBuilder_createOr(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateOr(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateXor(Value LHS, Value RHS);
static KMETHOD IRBuilder_createXor(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateXor(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateNeg(Value V);
static KMETHOD IRBuilder_createNeg(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Value *ptr = self->CreateNeg(V);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateNSWNeg(Value V);
static KMETHOD IRBuilder_createNSWNeg(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Value *ptr = self->CreateNSWNeg(V);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateNUWNeg(Value V);
static KMETHOD IRBuilder_createNUWNeg(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Value *ptr = self->CreateNUWNeg(V);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFNeg(Value V);
static KMETHOD IRBuilder_createFNeg(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Value *ptr = self->CreateFNeg(V);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateNot(Value V);
static KMETHOD IRBuilder_createNot(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Value *ptr = self->CreateNot(V);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## AllocaInst IRBuilder.CreateAlloca(Type Ty, Value ArraySize);
static KMETHOD IRBuilder_createAlloca(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Type *Ty = konoha::object_cast<Type *>(sfp[1].p);
	Value *ArraySize = konoha::object_cast<Value *>(sfp[2].p);
	AllocaInst *ptr = self->CreateAlloca(Ty, ArraySize);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## LoadInst IRBuilder.CreateLoad(Value Ptr, boolean isVolatile);
static KMETHOD IRBuilder_createLoad(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Ptr = konoha::object_cast<Value *>(sfp[1].p);
	kbool_t isVolatile = sfp[2].bvalue;
	LoadInst *ptr = self->CreateLoad(Ptr, isVolatile);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//@Native LoadInst LoadInst.new(Value ptr);
//## LoadInst IRBuilder.CreateLoad(Value Ptr, boolean isVolatile);
static KMETHOD LoadInst_new(CTX, ksfp_t *sfp _RIX)
{
	Value *Ptr = konoha::object_cast<Value *>(sfp[1].p);
	LoadInst *ptr = new LoadInst(Ptr);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## StoreInst IRBuilder.CreateStore(Value Val, Value Ptr, boolean isVolatile);
static KMETHOD IRBuilder_createStore(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Val = konoha::object_cast<Value *>(sfp[1].p);
	Value *Ptr = konoha::object_cast<Value *>(sfp[2].p);
	kbool_t isVolatile = sfp[3].bvalue;
	StoreInst *ptr = self->CreateStore(Val, Ptr, isVolatile);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

////## FenceInst IRBuilder.CreateFence(AtomicOrdering Ordering, SynchronizationScope SynchScope);
//KMETHOD IRBuilder_createFence(CTX, ksfp_t *sfp _RIX)
//{
//	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
//	AtomicOrdering *Ordering = konoha::object_cast<AtomicOrdering *>(sfp[1].p);
//	SynchronizationScope *SynchScope = konoha::object_cast<SynchronizationScope *>(sfp[2].p);
//	FenceInst *ptr = self->CreateFence(Ordering, SynchScope);
//	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
//	RETURN_(p);
//}
//
////## AtomicCmpXchgInst IRBuilder.CreateAtomicCmpXchg(Value Ptr, Value Cmp, Value New, AtomicOrdering Ordering, SynchronizationScope SynchScope);
//KMETHOD IRBuilder_createAtomicCmpXchg(CTX, ksfp_t *sfp _RIX)
//{
//	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
//	Value *Ptr = konoha::object_cast<Value *>(sfp[1].p);
//	Value *Cmp = konoha::object_cast<Value *>(sfp[2].p);
//	Value *New = konoha::object_cast<Value *>(sfp[3].p);
//	AtomicOrdering *Ordering = konoha::object_cast<AtomicOrdering *>(sfp[4].p);
//	SynchronizationScope *SynchScope = konoha::object_cast<SynchronizationScope *>(sfp[5].p);
//	AtomicCmpXchgInst *ptr = self->CreateAtomicCmpXchg(Ptr, Cmp, New, Ordering, SynchScope);
//	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
//	RETURN_(p);
//}

//## @Native AllocaInst AllocaInst.new(Type ty, Value arraySize);
static KMETHOD AllocaInst_new(CTX, ksfp_t *sfp _RIX)
{
	Type *Ty = konoha::object_cast<Type *>(sfp[1].p);
	Value *ArraySize = konoha::object_cast<Value *>(sfp[2].p);
	AllocaInst *ptr = new AllocaInst(Ty, ArraySize);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Native StoreInst StoreInst.new(Value val, Value ptr);
static KMETHOD StoreInst_new(CTX, ksfp_t *sfp _RIX)
{
	Value *Val = konoha::object_cast<Value *>(sfp[1].p);
	Value *Ptr = konoha::object_cast<Value *>(sfp[2].p);
	StoreInst *ptr = new StoreInst(Val, Ptr);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Native @Static GetElementPtrInst GetElementPtrInst.create(Value ptr, Array<Value> idxList);
static KMETHOD GetElementPtrInst_create(CTX, ksfp_t *sfp _RIX)
{
	Value *Ptr = konoha::object_cast<Value *>(sfp[1].p);
	kArray *IdxList = sfp[2].a;
	std::vector<Value*> List;
	konoha::convert_array(List, IdxList);
	GetElementPtrInst *ptr = GetElementPtrInst::Create(Ptr, List);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Native @Static GetElementPtrInst GetElementPtrInst.CreateInBounds(Value ptr, Array<Value> idxList);
static KMETHOD GetElementPtrInst_createInBounds(CTX, ksfp_t *sfp _RIX)
{
	Value *Ptr = konoha::object_cast<Value *>(sfp[1].p);
	kArray *IdxList = sfp[2].a;
	std::vector<Value*> List;
	konoha::convert_array(List, IdxList);
	GetElementPtrInst *ptr = GetElementPtrInst::CreateInBounds(Ptr, List);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateGEP(Value Ptr, ArrayRef< Value > IdxList);
static KMETHOD IRBuilder_createGEP(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Ptr = konoha::object_cast<Value *>(sfp[1].p);
	kArray *IdxList = sfp[2].a;
	std::vector<Value*> List;
	konoha::convert_array(List, IdxList);
	Value *ptr = self->CreateGEP(Ptr, List);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateInBoundsGEP(Value Ptr, ArrayRef< Value > IdxList);
static KMETHOD IRBuilder_createInBoundsGEP(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Ptr = konoha::object_cast<Value *>(sfp[1].p);
	kArray *IdxList = sfp[2].a;
	std::vector<Value*> List;
	konoha::convert_array(List, IdxList);
	Value *ptr = self->CreateInBoundsGEP(Ptr, List);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateGEP1(Value Ptr, Value Idx);
static KMETHOD IRBuilder_createGEP1(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Ptr = konoha::object_cast<Value *>(sfp[1].p);
	Value *Idx = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateGEP(Ptr, Idx);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateInBoundsGEP1(Value Ptr, Value Idx);
static KMETHOD IRBuilder_createInBoundsGEP1(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Ptr = konoha::object_cast<Value *>(sfp[1].p);
	Value *Idx = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateInBoundsGEP(Ptr, Idx);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateConstGEP1_32(Value Ptr, int Idx0);
static KMETHOD IRBuilder_createConstGEP132(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Ptr = konoha::object_cast<Value *>(sfp[1].p);
	kint_t Idx0 = Int_to(kint_t,sfp[2]);
	Value *ptr = self->CreateConstGEP1_32(Ptr, Idx0);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateConstInBoundsGEP1_32(Value Ptr, int Idx0);
static KMETHOD IRBuilder_createConstInBoundsGEP132(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Ptr = konoha::object_cast<Value *>(sfp[1].p);
	kint_t Idx0 = Int_to(kint_t,sfp[2]);
	Value *ptr = self->CreateConstInBoundsGEP1_32(Ptr, Idx0);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateConstGEP2_32(Value Ptr, int Idx0, int Idx1);
static KMETHOD IRBuilder_createConstGEP232(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Ptr = konoha::object_cast<Value *>(sfp[1].p);
	kint_t Idx0 = Int_to(kint_t,sfp[2]);
	kint_t Idx1 = Int_to(kint_t,sfp[3]);
	Value *ptr = self->CreateConstGEP2_32(Ptr, Idx0, Idx1);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateConstInBoundsGEP2_32(Value Ptr, int Idx0, int Idx1);
static KMETHOD IRBuilder_createConstInBoundsGEP232(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Ptr = konoha::object_cast<Value *>(sfp[1].p);
	kint_t Idx0 = Int_to(kint_t,sfp[2]);
	kint_t Idx1 = Int_to(kint_t,sfp[3]);
	Value *ptr = self->CreateConstInBoundsGEP2_32(Ptr, Idx0, Idx1);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateConstGEP1_64(Value Ptr, uint64_t Idx0);
static KMETHOD IRBuilder_createConstGEP164(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Ptr = konoha::object_cast<Value *>(sfp[1].p);
	kint_t Idx0 = sfp[2].ivalue;
	Value *ptr = self->CreateConstGEP1_64(Ptr, Idx0);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateConstInBoundsGEP1_64(Value Ptr, uint64_t Idx0);
static KMETHOD IRBuilder_createConstInBoundsGEP164(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Ptr = konoha::object_cast<Value *>(sfp[1].p);
	kint_t Idx0 = sfp[2].ivalue;
	Value *ptr = self->CreateConstInBoundsGEP1_64(Ptr, Idx0);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateConstGEP2_64(Value Ptr, uint64_t Idx0, uint64_t Idx1);
static KMETHOD IRBuilder_createConstGEP264(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Ptr = konoha::object_cast<Value *>(sfp[1].p);
	kint_t Idx0 = sfp[2].ivalue;
	kint_t Idx1 = sfp[3].ivalue;
	Value *ptr = self->CreateConstGEP2_64(Ptr, Idx0, Idx1);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateConstInBoundsGEP2_64(Value Ptr, uint64_t Idx0, uint64_t Idx1);
static KMETHOD IRBuilder_createConstInBoundsGEP264(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Ptr = konoha::object_cast<Value *>(sfp[1].p);
	kint_t Idx0 = sfp[2].ivalue;
	kint_t Idx1 = sfp[3].ivalue;
	Value *ptr = self->CreateConstInBoundsGEP2_64(Ptr, Idx0, Idx1);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateStructGEP(Value Ptr, int Idx);
static KMETHOD IRBuilder_createStructGEP(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Ptr = konoha::object_cast<Value *>(sfp[1].p);
	kint_t Idx = Int_to(kint_t,sfp[2]);
	Value *ptr = self->CreateStructGEP(Ptr, Idx, "gep");
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateGlobalString(StringRef Str);
static KMETHOD IRBuilder_createGlobalString(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	kString *Str = sfp[1].s;
	Value *ptr = self->CreateGlobalString(S_text(Str));
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateGlobalStringPtr(StringRef Str);
static KMETHOD IRBuilder_createGlobalStringPtr(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	kString *Str = sfp[1].s;
	Value *ptr = self->CreateGlobalStringPtr(S_text(Str));
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateTrunc(Value V, Type DestTy);
static KMETHOD IRBuilder_createTrunc(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Type *DestTy = konoha::object_cast<Type *>(sfp[2].p);
	Value *ptr = self->CreateTrunc(V, DestTy);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateZExt(Value V, Type DestTy);
static KMETHOD IRBuilder_createZExt(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Type *DestTy = konoha::object_cast<Type *>(sfp[2].p);
	Value *ptr = self->CreateZExt(V, DestTy);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateSExt(Value V, Type DestTy);
static KMETHOD IRBuilder_createSExt(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Type *DestTy = konoha::object_cast<Type *>(sfp[2].p);
	Value *ptr = self->CreateSExt(V, DestTy);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFPToUI(Value V, Type DestTy);
static KMETHOD IRBuilder_createFPToUI(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Type *DestTy = konoha::object_cast<Type *>(sfp[2].p);
	Value *ptr = self->CreateFPToUI(V, DestTy);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFPToSI(Value V, Type DestTy);
static KMETHOD IRBuilder_createFPToSI(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Type *DestTy = konoha::object_cast<Type *>(sfp[2].p);
	Value *ptr = self->CreateFPToSI(V, DestTy);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateUIToFP(Value V, Type DestTy);
static KMETHOD IRBuilder_createUIToFP(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Type *DestTy = konoha::object_cast<Type *>(sfp[2].p);
	Value *ptr = self->CreateUIToFP(V, DestTy);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateSIToFP(Value V, Type DestTy);
static KMETHOD IRBuilder_createSIToFP(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Type *DestTy = konoha::object_cast<Type *>(sfp[2].p);
	Value *ptr = self->CreateSIToFP(V, DestTy);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFPTrunc(Value V, Type DestTy);
static KMETHOD IRBuilder_createFPTrunc(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Type *DestTy = konoha::object_cast<Type *>(sfp[2].p);
	Value *ptr = self->CreateFPTrunc(V, DestTy);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFPExt(Value V, Type DestTy);
static KMETHOD IRBuilder_createFPExt(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Type *DestTy = konoha::object_cast<Type *>(sfp[2].p);
	Value *ptr = self->CreateFPExt(V, DestTy);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreatePtrToInt(Value V, Type DestTy);
static KMETHOD IRBuilder_createPtrToInt(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Type *DestTy = konoha::object_cast<Type *>(sfp[2].p);
	Value *ptr = self->CreatePtrToInt(V, DestTy);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateIntToPtr(Value V, Type DestTy);
static KMETHOD IRBuilder_createIntToPtr(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Type *DestTy = konoha::object_cast<Type *>(sfp[2].p);
	Value *ptr = self->CreateIntToPtr(V, DestTy);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateBitCast(Value V, Type DestTy);
static KMETHOD IRBuilder_createBitCast(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Type *DestTy = konoha::object_cast<Type *>(sfp[2].p);
	Value *ptr = self->CreateBitCast(V, DestTy);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateZExtOrBitCast(Value V, Type DestTy);
static KMETHOD IRBuilder_createZExtOrBitCast(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Type *DestTy = konoha::object_cast<Type *>(sfp[2].p);
	Value *ptr = self->CreateZExtOrBitCast(V, DestTy);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateSExtOrBitCast(Value V, Type DestTy);
static KMETHOD IRBuilder_createSExtOrBitCast(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Type *DestTy = konoha::object_cast<Type *>(sfp[2].p);
	Value *ptr = self->CreateSExtOrBitCast(V, DestTy);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateTruncOrBitCast(Value V, Type DestTy);
static KMETHOD IRBuilder_createTruncOrBitCast(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Type *DestTy = konoha::object_cast<Type *>(sfp[2].p);
	Value *ptr = self->CreateTruncOrBitCast(V, DestTy);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreatePointerCast(Value V, Type DestTy);
static KMETHOD IRBuilder_createPointerCast(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Type *DestTy = konoha::object_cast<Type *>(sfp[2].p);
	Value *ptr = self->CreatePointerCast(V, DestTy);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateIntCast(Value V, Type DestTy, boolean isSigned);
static KMETHOD IRBuilder_createIntCast(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Type *DestTy = konoha::object_cast<Type *>(sfp[2].p);
	kbool_t isSigned = sfp[3].bvalue;
	Value *ptr = self->CreateIntCast(V, DestTy, isSigned);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFPCast(Value V, Type DestTy);
static KMETHOD IRBuilder_createFPCast(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V = konoha::object_cast<Value *>(sfp[1].p);
	Type *DestTy = konoha::object_cast<Type *>(sfp[2].p);
	Value *ptr = self->CreateFPCast(V, DestTy);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateICmpEQ(Value LHS, Value RHS);
static KMETHOD IRBuilder_createICmpEQ(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateICmpEQ(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateICmpNE(Value LHS, Value RHS);
static KMETHOD IRBuilder_createICmpNE(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateICmpNE(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateICmpUGT(Value LHS, Value RHS);
static KMETHOD IRBuilder_createICmpUGT(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateICmpUGT(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateICmpUGE(Value LHS, Value RHS);
static KMETHOD IRBuilder_createICmpUGE(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateICmpUGE(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateICmpULT(Value LHS, Value RHS);
static KMETHOD IRBuilder_createICmpULT(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateICmpULT(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateICmpULE(Value LHS, Value RHS);
static KMETHOD IRBuilder_createICmpULE(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateICmpULE(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateICmpSGT(Value LHS, Value RHS);
static KMETHOD IRBuilder_createICmpSGT(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateICmpSGT(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateICmpSGE(Value LHS, Value RHS);
static KMETHOD IRBuilder_createICmpSGE(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateICmpSGE(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateICmpSLT(Value LHS, Value RHS);
static KMETHOD IRBuilder_createICmpSLT(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateICmpSLT(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateICmpSLE(Value LHS, Value RHS);
static KMETHOD IRBuilder_createICmpSLE(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateICmpSLE(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFCmpOEQ(Value LHS, Value RHS);
static KMETHOD IRBuilder_createFCmpOEQ(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateFCmpOEQ(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFCmpOGT(Value LHS, Value RHS);
static KMETHOD IRBuilder_createFCmpOGT(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateFCmpOGT(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFCmpOGE(Value LHS, Value RHS);
static KMETHOD IRBuilder_createFCmpOGE(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateFCmpOGE(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFCmpOLT(Value LHS, Value RHS);
static KMETHOD IRBuilder_createFCmpOLT(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateFCmpOLT(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFCmpOLE(Value LHS, Value RHS);
static KMETHOD IRBuilder_createFCmpOLE(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateFCmpOLE(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFCmpONE(Value LHS, Value RHS);
static KMETHOD IRBuilder_createFCmpONE(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateFCmpONE(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFCmpORD(Value LHS, Value RHS);
static KMETHOD IRBuilder_createFCmpORD(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateFCmpORD(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFCmpUNO(Value LHS, Value RHS);
static KMETHOD IRBuilder_createFCmpUNO(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateFCmpUNO(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFCmpUEQ(Value LHS, Value RHS);
static KMETHOD IRBuilder_createFCmpUEQ(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateFCmpUEQ(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFCmpUGT(Value LHS, Value RHS);
static KMETHOD IRBuilder_createFCmpUGT(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateFCmpUGT(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFCmpUGE(Value LHS, Value RHS);
static KMETHOD IRBuilder_createFCmpUGE(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateFCmpUGE(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFCmpULT(Value LHS, Value RHS);
static KMETHOD IRBuilder_createFCmpULT(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateFCmpULT(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFCmpULE(Value LHS, Value RHS);
static KMETHOD IRBuilder_createFCmpULE(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateFCmpULE(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateFCmpUNE(Value LHS, Value RHS);
static KMETHOD IRBuilder_createFCmpUNE(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateFCmpUNE(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## PHINode IRBuilder.CreatePHI(Type Ty, int numReservedValues);
static KMETHOD IRBuilder_createPHI(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Type *Ty = konoha::object_cast<Type *>(sfp[1].p);
	kint_t num = sfp[2].ivalue;
	PHINode *ptr = self->CreatePHI(Ty, num);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## void IRBuilder.addIncoming(Type Ty, BasicBlock bb);
static KMETHOD PHINode_addIncoming(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	PHINode *self = konoha::object_cast<PHINode *>(sfp[0].p);
	Value *v = konoha::object_cast<Value *>(sfp[1].p);
	BasicBlock *bb = konoha::object_cast<BasicBlock *>(sfp[2].p);
	self->addIncoming(v, bb);
	RETURNvoid_();
}

//## CallInst IRBuilder.CreateCall1(Value Callee, Value Arg);
static KMETHOD IRBuilder_createCall1(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Callee = konoha::object_cast<Value *>(sfp[1].p);
	Value *Arg = konoha::object_cast<Value *>(sfp[2].p);
	CallInst *ptr = self->CreateCall(Callee, Arg);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## CallInst IRBuilder.CreateCall2(Value Callee, Value Arg1, Value Arg2);
static KMETHOD IRBuilder_createCall2(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Callee = konoha::object_cast<Value *>(sfp[1].p);
	Value *Arg1 = konoha::object_cast<Value *>(sfp[2].p);
	Value *Arg2 = konoha::object_cast<Value *>(sfp[3].p);
	CallInst *ptr = self->CreateCall2(Callee, Arg1, Arg2);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## CallInst IRBuilder.CreateCall3(Value Callee, Value Arg1, Value Arg2, Value Arg3);
static KMETHOD IRBuilder_createCall3(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Callee = konoha::object_cast<Value *>(sfp[1].p);
	Value *Arg1 = konoha::object_cast<Value *>(sfp[2].p);
	Value *Arg2 = konoha::object_cast<Value *>(sfp[3].p);
	Value *Arg3 = konoha::object_cast<Value *>(sfp[4].p);
	CallInst *ptr = self->CreateCall3(Callee, Arg1, Arg2, Arg3);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## CallInst IRBuilder.CreateCall4(Value Callee, Value Arg1, Value Arg2, Value Arg3, Value Arg4);
static KMETHOD IRBuilder_createCall4(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Callee = konoha::object_cast<Value *>(sfp[1].p);
	Value *Arg1 = konoha::object_cast<Value *>(sfp[2].p);
	Value *Arg2 = konoha::object_cast<Value *>(sfp[3].p);
	Value *Arg3 = konoha::object_cast<Value *>(sfp[4].p);
	Value *Arg4 = konoha::object_cast<Value *>(sfp[5].p);
	CallInst *ptr = self->CreateCall4(Callee, Arg1, Arg2, Arg3, Arg4);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## CallInst IRBuilder.CreateCall5(Value Callee, Value Arg1, Value Arg2, Value Arg3, Value Arg4, Value Arg5);
static KMETHOD IRBuilder_createCall5(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Callee = konoha::object_cast<Value *>(sfp[1].p);
	Value *Arg1 = konoha::object_cast<Value *>(sfp[2].p);
	Value *Arg2 = konoha::object_cast<Value *>(sfp[3].p);
	Value *Arg3 = konoha::object_cast<Value *>(sfp[4].p);
	Value *Arg4 = konoha::object_cast<Value *>(sfp[5].p);
	Value *Arg5 = konoha::object_cast<Value *>(sfp[6].p);
	CallInst *ptr = self->CreateCall5(Callee, Arg1, Arg2, Arg3, Arg4, Arg5);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## CallInst IRBuilder.CreateCall(Value Callee, ArrayRef< Value > Args);
static KMETHOD IRBuilder_createCall(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Callee = konoha::object_cast<Value *>(sfp[1].p);
	kArray *Args = sfp[2].a;
	std::vector<Value*> List;
	konoha::convert_array(List, Args);
	CallInst *ptr = self->CreateCall(Callee, List);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateSelect(Value C, Value True, Value False);
static KMETHOD IRBuilder_createSelect(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *C = konoha::object_cast<Value *>(sfp[1].p);
	Value *True = konoha::object_cast<Value *>(sfp[2].p);
	Value *False = konoha::object_cast<Value *>(sfp[3].p);
	Value *ptr = self->CreateSelect(C, True, False);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## VAArgInst IRBuilder.CreateVAArg(Value List, Type Ty);
static KMETHOD IRBuilder_createVAArg(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *List = konoha::object_cast<Value *>(sfp[1].p);
	Type *Ty = konoha::object_cast<Type *>(sfp[2].p);
	VAArgInst *ptr = self->CreateVAArg(List, Ty);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateExtractElement(Value Vec, Value Idx);
static KMETHOD IRBuilder_createExtractElement(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Vec = konoha::object_cast<Value *>(sfp[1].p);
	Value *Idx = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreateExtractElement(Vec, Idx);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateInsertElement(Value Vec, Value NewElt, Value Idx);
static KMETHOD IRBuilder_createInsertElement(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Vec = konoha::object_cast<Value *>(sfp[1].p);
	Value *NewElt = konoha::object_cast<Value *>(sfp[2].p);
	Value *Idx = konoha::object_cast<Value *>(sfp[3].p);
	Value *ptr = self->CreateInsertElement(Vec, NewElt, Idx);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateShuffleVector(Value V1, Value V2, Value Mask);
static KMETHOD IRBuilder_createShuffleVector(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *V1 = konoha::object_cast<Value *>(sfp[1].p);
	Value *V2 = konoha::object_cast<Value *>(sfp[2].p);
	Value *Mask = konoha::object_cast<Value *>(sfp[3].p);
	Value *ptr = self->CreateShuffleVector(V1, V2, Mask);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

////## Value IRBuilder.CreateExtractValue(Value Agg, Array<int> Idxs);
//KMETHOD IRBuilder_createExtractValue(CTX, ksfp_t *sfp _RIX)
//{
//	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
//	Value *Agg = konoha::object_cast<Value *>(sfp[1].p);
//	kArray *Idxs = sfp[2].a;
//	std::vector<int> List;
//	konoha::convert_array_int(List, Idxs);
//	Value *ptr = self->CreateExtractValue(Agg, List.begin(), List.end());
//	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
//	RETURN_(p);
//}

////## Value IRBuilder.CreateInsertValue(Value Agg, Value Val, Array<int> Idxs);
//KMETHOD IRBuilder_createInsertValue(CTX, ksfp_t *sfp _RIX)
//{
//	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
//	Value *Agg = konoha::object_cast<Value *>(sfp[1].p);
//	Value *Val = konoha::object_cast<Value *>(sfp[2].p);
//	kArray *Idxs = sfp[2].a;
//	std::vector<int> List;
//	konoha::convert_array_int(List, Idxs);
//	Value *ptr = self->CreateInsertValue(Agg, Val, List.begin(), List.end());
//	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
//	RETURN_(p);
//}

//## Value IRBuilder.CreateIsNull(Value Arg);
static KMETHOD IRBuilder_createIsNull(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Arg = konoha::object_cast<Value *>(sfp[1].p);
	Value *ptr = self->CreateIsNull(Arg);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreateIsNotNull(Value Arg);
static KMETHOD IRBuilder_createIsNotNull(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *Arg = konoha::object_cast<Value *>(sfp[1].p);
	Value *ptr = self->CreateIsNotNull(Arg);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Value IRBuilder.CreatePtrDiff(Value LHS, Value RHS);
static KMETHOD IRBuilder_createPtrDiff(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	Value *LHS = konoha::object_cast<Value *>(sfp[1].p);
	Value *RHS = konoha::object_cast<Value *>(sfp[2].p);
	Value *ptr = self->CreatePtrDiff(LHS, RHS);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## void IRBuilder.SetInsertPoint(BasicBlock BB);
static KMETHOD IRBuilder_setInsertPoint(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	BasicBlock * BB = konoha::object_cast<BasicBlock *>(sfp[1].p);
	self->SetInsertPoint(BB);
	RETURNvoid_();
}

//## BasicBlock IRBuilder.GetInsertBlock();
static KMETHOD IRBuilder_getInsertBlock(CTX, ksfp_t *sfp _RIX)
{
	IRBuilder<> *self = konoha::object_cast<IRBuilder<> *>(sfp[0].p);
	BasicBlock *BB = self->GetInsertBlock();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(BB) K_RIXPARAM);
	RETURN_(p);
}

//## Function BasicBlock.getParent();
static KMETHOD BasicBlock_getParent(CTX, ksfp_t *sfp _RIX)
{
	BasicBlock *self = konoha::object_cast<BasicBlock *>(sfp[0].p);
	Function *ptr = self->getParent();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Instruction BasicBlock.getTerminator();
static KMETHOD BasicBlock_getTerminator(CTX, ksfp_t *sfp _RIX)
{
	BasicBlock *self = konoha::object_cast<BasicBlock *>(sfp[0].p);
	TerminatorInst *ptr = self->getTerminator();
	if (ptr) {
		kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
		RETURN_(p);
	} else {
		RETURN_(K_NULL);
	}
}


////## iterator BasicBlock.begin();
//KMETHOD BasicBlock_begin(CTX, ksfp_t *sfp _RIX)
//{
//	BasicBlock *self = konoha::object_cast<BasicBlock *>(sfp[0].p);
//	*ptr = self->Create();
//	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
//	RETURN_(K_NULL);
//}
//
////## iterator BasicBlock.end();
//KMETHOD BasicBlock_end(CTX, ksfp_t *sfp _RIX)
//{
//	BasicBlock *self = konoha::object_cast<BasicBlock *>(sfp[0].p);
//	*ptr = self->Create();
//	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
//	RETURN_(K_NULL);
//}

//## Instruction BasicBlock.getLastInst();
static KMETHOD BasicBlock_getLastInst(CTX, ksfp_t *sfp _RIX)
{
	BasicBlock *self = konoha::object_cast<BasicBlock *>(sfp[0].p);
	BasicBlock::iterator I = self->end();
	Instruction *ptr;
	if (self->size() > 0)
		--I;
	ptr = I;
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Instruction BasicBlock.insertBefore(Instruction before, Instruction inst);
static KMETHOD BasicBlock_insertBefore(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	BasicBlock *self = konoha::object_cast<BasicBlock *>(sfp[0].p);
	Instruction *inst0 = konoha::object_cast<Instruction *>(sfp[1].p);
	Instruction *inst1 = konoha::object_cast<Instruction *>(sfp[2].p);
	self->getInstList().insert(inst0, inst1);
	RETURNvoid_();
}

//## int BasicBlock.size();
static KMETHOD BasicBlock_size(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	BasicBlock *self = konoha::object_cast<BasicBlock *>(sfp[0].p);
	int ret = self->size();
	RETURNi_(ret);
}

//## boolean BasicBlock.empty();
static KMETHOD BasicBlock_empty(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	BasicBlock *self = konoha::object_cast<BasicBlock *>(sfp[0].p);
	bool isEmpty = self->empty();
	RETURNb_(isEmpty);
}

//## Argument Argument.new(Type ty, int scid);
static KMETHOD Argument_new(CTX, ksfp_t *sfp _RIX)
{
	Type *ty = konoha::object_cast<Type *>(sfp[1].p);
	Value *v = new Argument(ty, "", 0);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(v) K_RIXPARAM);
	RETURN_(p);
}

//static void str_replace (std::string& str, const std::string& from, const std::string& to) {
//	std::string::size_type pos = 0;
//	while ((pos = str.find(from, pos)) != std::string::npos) {
//		str.replace( pos, from.size(), to );
//		pos++;
//	}
//}
//
//## Module Module.new(String name);
static KMETHOD Module_new(CTX, ksfp_t *sfp _RIX)
{
	kString *name = sfp[1].s;
	LLVMContext &Context = getGlobalContext();
	Module *M = new Module(S_text(name), Context);
#if 0
	Triple T(sys::getDefaultTargetTriple());
	const Target *Target = 0;
	std::string Arch = T.getArchName();
	for (TargetRegistry::iterator it = TargetRegistry::begin(),
			ie = TargetRegistry::end(); it != ie; ++it) {
		std::string tmp(it->getName());
		str_replace(tmp, "-", "_");
		if (Arch == tmp) {
			Target = &*it;
			break;
		}
	}
	assert(Target != 0);
	std::string FeaturesStr;
	TargetOptions Options;
	TargetMachine *TM = Target->createTargetMachine(T.getTriple(), Target->getName(), FeaturesStr, Options);
	M->setTargetTriple(T.getTriple());
	M->setDataLayout(TM->getTargetData()->getStringRepresentation());
#endif
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(M) K_RIXPARAM);
	RETURN_(p);
}

//## void Module.dump();
static KMETHOD Module_dump(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	Module *self = konoha::object_cast<Module *>(sfp[0].p);
	(*self).dump();
	RETURNvoid_();
}

//## Type Module.getTypeByName(String name);
static KMETHOD Module_getTypeByName(CTX, ksfp_t *sfp _RIX)
{
	Module *self = konoha::object_cast<Module *>(sfp[0].p);
	kString *name = sfp[1].s;
	Type *ptr = self->getTypeByName(S_text(name));
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## void BasicBlock.dump();
static KMETHOD BasicBlock_dump(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	BasicBlock *self = konoha::object_cast<BasicBlock *>(sfp[0].p);
	(*self).dump();
	RETURNvoid_();
}

//## Function Module.getOrInsertFunction(String name, FunctionType fnTy);
static KMETHOD Module_getOrInsertFunction(CTX, ksfp_t *sfp _RIX)
{
	Module *self = konoha::object_cast<Module *>(sfp[0].p);
	kString *name = sfp[1].s;
	FunctionType *fnTy = konoha::object_cast<FunctionType *>(sfp[2].p);
	Function *ptr = cast<Function>(self->getOrInsertFunction(S_text(name), fnTy));
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static @Native Function Function.create(String name, FunctionType fnTy, Module m, Linkage linkage);
static KMETHOD Function_create(CTX, ksfp_t *sfp _RIX)
{
	kString *name = sfp[1].s;
	FunctionType *fnTy = konoha::object_cast<FunctionType *>(sfp[2].p);
	Module *m = konoha::object_cast<Module *>(sfp[3].p);
	kint_t v = sfp[4].ivalue;
	GlobalValue::LinkageTypes linkage = (GlobalValue::LinkageTypes) v;
	Function *ptr = Function::Create(fnTy, linkage, S_text(name), m);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Native void Function.dump();
static KMETHOD Function_dump(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	Function *func = konoha::object_cast<Function *>(sfp[0].p);
	func->dump();
	RETURNvoid_();
}

//## @Native void Function.addFnAttr(Int attributes);
static KMETHOD Function_addFnAttr(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	Function *F = konoha::object_cast<Function *>(sfp[0].p);
	Attributes N = (Attributes) sfp[1].ivalue;
	F->addFnAttr(N);
	RETURNvoid_();
}

//## ExecutionEngine Module.createExecutionEngine(int optLevel);
static KMETHOD Module_createExecutionEngine(CTX, ksfp_t *sfp _RIX)
{
	Module *self = konoha::object_cast<Module *>(sfp[0].p);
	CodeGenOpt::Level OptLevel = (CodeGenOpt::Level) sfp[1].ivalue;
	ExecutionEngine *ptr = EngineBuilder(self).setEngineKind(EngineKind::JIT).setOptLevel(OptLevel).create();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

static int BasicBlock_compareTo(kRawPtr *p1, kRawPtr *p2)
{
	return (p1->rawptr != p2->rawptr);
}

//void defBasicBlock(CTX _UNUSED_, kcid_t cid _UNUSED_, kclassdef_t *cdef)
//{
//	cdef->name = "llvm::BasicBlock";
//	cdef->compareTo = BasicBlock_compareTo;
//}

//## @Static BasicBlock BasicBlock.create(Function parent, String name);
static KMETHOD BasicBlock_create(CTX, ksfp_t *sfp _RIX)
{
	Function * parent = konoha::object_cast<Function *>(sfp[1].p);
	kString *name = sfp[2].s;
	const char *bbname = "";
	if (IS_NOTNULL(name)) {
		bbname = S_text(name);
	}
	BasicBlock *ptr = BasicBlock::Create(getGlobalContext(), bbname, parent);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static FunctionType.get(Type retTy, Array<Type> args, boolean b);
static KMETHOD FunctionType_get(CTX, ksfp_t *sfp _RIX)
{
	Type *retTy = konoha::object_cast<Type *>(sfp[1].p);
	kArray * args = sfp[2].a;
	kbool_t b = sfp[3].bvalue;
	std::vector<Type*> List;
	konoha::convert_array(List, args);
	FunctionType *ptr = FunctionType::get(retTy, List, b);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Native Value ConstantInt.get(Type type, int v);
static KMETHOD ConstantInt_get(CTX, ksfp_t *sfp _RIX)
{
	Type *type  = konoha::object_cast<Type *>(sfp[1].p);
	kint_t v = sfp[2].ivalue;
	Value *ptr = ConstantInt::get(type, v);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Native Value ConstantFP.get(Type type, float v);
static KMETHOD ConstantFP_get(CTX, ksfp_t *sfp _RIX)
{
	Type *type  = konoha::object_cast<Type *>(sfp[1].p);
	kfloat_t v = sfp[2].fvalue;
	Value *ptr = ConstantFP::get(type, v);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static @Native Value ConstantPointerNull.get(Type type);
static KMETHOD ConstantPointerNull_get(CTX, ksfp_t *sfp _RIX)
{
	PointerType *type  = konoha::object_cast<PointerType *>(sfp[1].p);
	Value *ptr = ConstantPointerNull::get(type);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static @Native Value ConstantStruct.get(Type type, Array<Constant> V);
static KMETHOD ConstantStruct_get(CTX, ksfp_t *sfp _RIX)
{
	StructType *type  = konoha::object_cast<StructType *>(sfp[1].p);
	kArray *args = sfp[2].a;
	std::vector<Constant*> List;
	konoha::convert_array(List, args);
	Value *ptr = ConstantStruct::get(type, List);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static StructType.get(Array<Type> args, boolean isPacked);
static KMETHOD StructType_get(CTX, ksfp_t *sfp _RIX)
{
	kArray *args = sfp[1].a;
	kbool_t isPacked = sfp[2].bvalue;
	std::vector<Type*> List;
	konoha::convert_array(List, args);
	StructType *ptr = StructType::get(getGlobalContext(), List, isPacked);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Static @Native StructType.create(Array<Type> args, String name, boolean isPacked);
static KMETHOD StructType_create(CTX, ksfp_t *sfp _RIX)
{
	kArray *args = sfp[1].a;
	kString *name = sfp[2].s;
	kbool_t isPacked = sfp[3].bvalue;
	StructType *ptr;
	if (IS_NULL(args)) {
		ptr = StructType::create(getGlobalContext(), S_text(name));
	} else if (kArray_size(args) == 0) {
		std::vector<Type*> List;
		ptr = StructType::create(getGlobalContext(), S_text(name));
		ptr->setBody(List, isPacked);
	} else {
		std::vector<Type*> List;
		konoha::convert_array(List, args);
		ptr = StructType::create(List, S_text(name), isPacked);
	}
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Native @Static ArrayType ArrayType.get(Type t, int elemSize);
static KMETHOD ArrayType_get(CTX, ksfp_t *sfp _RIX)
{
	Type *Ty = konoha::object_cast<Type *>(sfp[1].p);
	kint_t N = sfp[2].bvalue;
	ArrayType *ptr = ArrayType::get(Ty, N);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## @Native void StructType.setBody(Array<Type> args, boolean isPacked);
static KMETHOD StructType_setBody(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	StructType *type  = konoha::object_cast<StructType *>(sfp[0].p);
	kArray *args = sfp[1].a;
	kbool_t isPacked = sfp[2].bvalue;
	std::vector<Type*> List;
	konoha::convert_array(List, args);
	type->setBody(List, isPacked);
	RETURNvoid_();
}

//## @Native boolean StructType.isOpaque();
static KMETHOD StructType_isOpaque(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	StructType *type  = konoha::object_cast<StructType *>(sfp[0].p);
	bool ret = type->isOpaque();
	RETURNb_(ret);
}

//## NativeFunction ExecutionEngine.getPointerToFunction(Function func);
static KMETHOD ExecutionEngine_getPointerToFunction(CTX, ksfp_t *sfp _RIX)
{
	ExecutionEngine *ee = konoha::object_cast<ExecutionEngine *>(sfp[0].p);
	Function *func = konoha::object_cast<Function *>(sfp[1].p);
	void *ptr = ee->getPointerToFunction(func);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}
//## @Native void ExecutionEngine.addGlobalMapping(GlobalVariable g, int addr);
static KMETHOD ExecutionEngine_addGlobalMapping(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	ExecutionEngine *ee = konoha::object_cast<ExecutionEngine *>(sfp[0].p);
	GlobalVariable *g   = konoha::object_cast<GlobalVariable *>(sfp[1].p);
	long addr = sfp[2].ivalue;
	ee->addGlobalMapping(g, (void*)addr);
	RETURNvoid_();
}
//## @Native GlobalVariable GlobalVariable.new(Module m, Type ty, Constant c, Linkage linkage, String name);
static KMETHOD GlobalVariable_new(CTX, ksfp_t *sfp _RIX)
{
	Module *m     = konoha::object_cast<Module *>(sfp[1].p);
	Type *ty      = konoha::object_cast<Type *>(sfp[2].p);
	Constant *c   = konoha::object_cast<Constant *>(sfp[3].p);
	GlobalValue::LinkageTypes linkage = (GlobalValue::LinkageTypes) sfp[4].ivalue;
	kString *name = sfp[5].s;
	bool isConstant = (c) ? true : false;
	GlobalVariable *ptr = new GlobalVariable(*m, ty, isConstant, linkage, c, S_text(name));
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

static void PassManagerBuilder_ptr_init(CTX _UNUSED_, kRawPtr *po, void *conf)
{
	po->rawptr = conf;
}

static void PassManagerBuilder_ptr_free(CTX _UNUSED_, kRawPtr *po)
{
	PassManagerBuilder *o = static_cast<PassManagerBuilder *>(po->rawptr);
	delete o;
}

static KMETHOD PassManagerBuilder_new(CTX, ksfp_t *sfp _RIX)
{
	PassManagerBuilder *self = new PassManagerBuilder();
	self->OptLevel = 3;
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(self) K_RIXPARAM);
	RETURN_(p);
}
static KMETHOD PassManagerBuilder_populateModulePassManager(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	PassManagerBuilder *self = konoha::object_cast<PassManagerBuilder *>(sfp[0].p);
	PassManager *manager = konoha::object_cast<PassManager *>(sfp[1].p);
	self->populateModulePassManager(*manager);
	RETURNvoid_();
}

static void PassManager_ptr_init(CTX _UNUSED_, kRawPtr *po, void *conf)
{
	po->rawptr = conf;
}

static void PassManager_ptr_free(CTX _UNUSED_, kRawPtr *po)
{
	PassManager *o = static_cast<PassManager *>(po->rawptr);
	delete o;
}

//## PassManager PassManager.new()
static KMETHOD PassManager_new(CTX, ksfp_t *sfp _RIX)
{
	PassManager *self = new PassManager();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(self) K_RIXPARAM);
	RETURN_(p);
}

//## void PassManager.run(Function func)
static KMETHOD PassManager_run(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	PassManager *self = konoha::object_cast<PassManager *>(sfp[0].p);
	Module *m = konoha::object_cast<Module *>(sfp[1].p);
	self->run(*m);
	RETURNvoid_();
}
//## void PassManager.add(Pass p)
static KMETHOD PassManager_addPass(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	PassManager *self = konoha::object_cast<PassManager *>(sfp[0].p);
	Pass *pass = konoha::object_cast<Pass *>(sfp[1].p);
	self->add(pass);
	RETURNvoid_();
}
//## void PassManager.add(Pass p)
static KMETHOD PassManager_addImmutablePass(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	PassManager *self = konoha::object_cast<PassManager *>(sfp[0].p);
	ImmutablePass *pass = konoha::object_cast<ImmutablePass *>(sfp[1].p);
	self->add(pass);
	RETURNvoid_();
}
//## void PassManager.addFunctionPass(Pass p)
static KMETHOD PassManager_addFunctionPass(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	PassManager *self = konoha::object_cast<PassManager *>(sfp[0].p);
	FunctionPass *pass = konoha::object_cast<FunctionPass *>(sfp[1].p);
	self->add(pass);
	RETURNvoid_();
}
//## void PassManager.addModulePass(Pass p)
static KMETHOD PassManager_addModulePass(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	PassManager *self = konoha::object_cast<PassManager *>(sfp[0].p);
	ModulePass *pass = konoha::object_cast<ModulePass *>(sfp[1].p);
	self->add(pass);
	RETURNvoid_();
}

static void FunctionPassManager_ptr_init(CTX _UNUSED_, kRawPtr *po, void *conf)
{
	po->rawptr = conf;
}

static void FunctionPassManager_ptr_free(CTX _UNUSED_, kRawPtr *po)
{
	FunctionPassManager *o = static_cast<FunctionPassManager *>(po->rawptr);
	delete o;
}

//## FunctionPassManager FunctionPassManager.new(Module m)
static KMETHOD FunctionPassManager_new(CTX, ksfp_t *sfp _RIX)
{
	Module *m = konoha::object_cast<Module *>(sfp[1].p);
	FunctionPassManager *self = new FunctionPassManager(m);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(self) K_RIXPARAM);
	RETURN_(p);
}
//## void FuncitonPassManager.add(Pass p)
static KMETHOD FunctionPassManager_add(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	FunctionPassManager *self = konoha::object_cast<FunctionPassManager *>(sfp[0].p);
	Pass *pass = konoha::object_cast<Pass *>(sfp[1].p);
	self->add(pass);
	RETURNvoid_();
}
//## void FunctionPassManager.doInitialization()
static KMETHOD FunctionPassManager_doInitialization(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	FunctionPassManager *self = konoha::object_cast<FunctionPassManager *>(sfp[0].p);
	self->doInitialization();
	RETURNvoid_();
}

//## void FunctionPassManager.run(Function func)
static KMETHOD FunctionPassManager_run(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	FunctionPassManager *self = konoha::object_cast<FunctionPassManager *>(sfp[0].p);
	Function *func = konoha::object_cast<Function *>(sfp[1].p);
	self->run(*func);
	RETURNvoid_();
}

//## TargetData ExecutionEngine.getTargetData();
static KMETHOD ExecutionEngine_getTargetData(CTX, ksfp_t *sfp _RIX)
{
	ExecutionEngine *ee = konoha::object_cast<ExecutionEngine *>(sfp[0].p);
	TargetData *ptr = new TargetData(*(ee->getTargetData()));
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## void Method.setFunction(NativeFunction func);
static KMETHOD Method_setFunction(CTX, ksfp_t *sfp _RIX)
{
	kMethod *mtd = (kMethod*) sfp[0].o;
	kRawPtr *po = sfp[1].p;
	union anyptr { void *p; knh_Fmethod f;} ptr = {po->rawptr};
	kMethod_setFunc(mtd, ptr.f);
	RETURNvoid_();
}

//## @Native Array<Value> Function.getArguments();
static KMETHOD Function_getArguments(CTX, ksfp_t *sfp _RIX)
{
	ktype_t rtype = sfp[K_MTDIDX].mtdNC->pa->rtype;
	kcid_t cid = CT_(rtype)->p1;
	Function *func = konoha::object_cast<Function *>(sfp[0].p);
	kArray *a = (kArray*) new_kObject(CT_(rtype), 0);
	for (Function::arg_iterator I = func->arg_begin(), E = func->arg_end();
			I != E; ++I) {
		Value *v = I;
		kObject *o = new_kObject(CT_(cid)/*"Value"*/, WRAP(v));
		kArray_add(a, o);
	}
	RETURN_(a);
}
//## void Value.replaceAllUsesWith(Value v);
static KMETHOD Value_replaceAllUsesWith(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	Value *self = konoha::object_cast<Value *>(sfp[0].p);
	Value *v = konoha::object_cast<Value *>(sfp[1].p);
	self->replaceAllUsesWith(v);
	RETURNvoid_();
}
//## Value Value.setName(String name);
static KMETHOD Value_setName(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	Value *self = konoha::object_cast<Value *>(sfp[0].p);
	kString *name = sfp[1].s;
	self->setName(S_text(name));
	RETURNvoid_();
}
//## void LoadInst.setAlignment(int align);
static KMETHOD LoadInst_setAlignment(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	LoadInst *self = konoha::object_cast<LoadInst *>(sfp[0].p);
	int align = sfp[1].ivalue;
	self->setAlignment(align);
	RETURNvoid_();
}
//## void StoreInst.setAlignment(int align);
static KMETHOD StoreInst_setAlignment(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	StoreInst *self = konoha::object_cast<StoreInst *>(sfp[0].p);
	int align = sfp[1].ivalue;
	self->setAlignment(align);
	RETURNvoid_();
}
//## Type Value.getType();
static KMETHOD Value_getType(CTX, ksfp_t *sfp _RIX)
{
	Value *self = konoha::object_cast<Value *>(sfp[0].p);
	const Type *ptr = self->getType();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## void Value.dump();
static KMETHOD Value_dump(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	Value *self = konoha::object_cast<Value *>(sfp[0].p);
	self->dump();
	RETURNvoid_();
}

//## @Native void Type.dump();
static KMETHOD Type_dump(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	Type *type = konoha::object_cast<Type *>(sfp[0].p);
	type->dump();
	RETURNvoid_();
}

//## @Static boolean DynamicLibrary.loadLibraryPermanently(String libname);
static KMETHOD DynamicLibrary_loadLibraryPermanently(CTX, ksfp_t *sfp _RIX)
{
	const char *libname = S_text(sfp[1].s);
	std::string ErrMsg;
	kbool_t ret = sys::DynamicLibrary::LoadLibraryPermanently(libname, &ErrMsg);
	if (ret == 0) {
		//TODO
		//KNH_NTRACE2(_ctx, "LoadLibraryPermanently", K_FAILED, KNH_LDATA(LOG_s("libname", libname), LOG_msg(ErrMsg.c_str())));
	}
	RETURNb_(ret);
}

//## @Static Int DynamicLibrary.searchForAddressOfSymbol(String fname);
static KMETHOD DynamicLibrary_searchForAddressOfSymbol(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	const char *fname = S_text(sfp[1].s);
	kint_t ret = 0;
	void *symAddr = NULL;
	if (!(symAddr = sys::DynamicLibrary::SearchForAddressOfSymbol(fname))) {
		ret = reinterpret_cast<kint_t>(symAddr);
	}
	RETURNi_(ret);
}

//## FunctionPass LLVM.createDomPrinterPass();
static KMETHOD LLVM_createDomPrinterPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createDomPrinterPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createDomOnlyPrinterPass();
static KMETHOD LLVM_createDomOnlyPrinterPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createDomOnlyPrinterPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createDomViewerPass();
static KMETHOD LLVM_createDomViewerPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createDomViewerPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createDomOnlyViewerPass();
static KMETHOD LLVM_createDomOnlyViewerPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createDomOnlyViewerPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createPostDomPrinterPass();
static KMETHOD LLVM_createPostDomPrinterPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createPostDomPrinterPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createPostDomOnlyPrinterPass();
static KMETHOD LLVM_createPostDomOnlyPrinterPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createPostDomOnlyPrinterPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createPostDomViewerPass();
static KMETHOD LLVM_createPostDomViewerPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createPostDomViewerPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createPostDomOnlyViewerPass();
static KMETHOD LLVM_createPostDomOnlyViewerPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createPostDomOnlyViewerPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createGlobalsModRefPass();
static KMETHOD LLVM_createGlobalsModRefPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createGlobalsModRefPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createAliasDebugger();
static KMETHOD LLVM_createAliasDebugger(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createAliasDebugger();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ModulePass LLVM.createAliasAnalysisCounterPass();
static KMETHOD LLVM_createAliasAnalysisCounterPass(CTX, ksfp_t *sfp _RIX)
{
	ModulePass *ptr = createAliasAnalysisCounterPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createAAEvalPass();
static KMETHOD LLVM_createAAEvalPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createAAEvalPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createLibCallAliasAnalysisPass(LibCallInfo lci);
static KMETHOD LLVM_createLibCallAliasAnalysisPass(CTX, ksfp_t *sfp _RIX)
{
	LibCallInfo *lci = konoha::object_cast<LibCallInfo *>(sfp[0].p);
	FunctionPass *ptr = createLibCallAliasAnalysisPass(lci);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createScalarEvolutionAliasAnalysisPass();
static KMETHOD LLVM_createScalarEvolutionAliasAnalysisPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createScalarEvolutionAliasAnalysisPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ModulePass LLVM.createProfileLoaderPass();
static KMETHOD LLVM_createProfileLoaderPass(CTX, ksfp_t *sfp _RIX)
{
	ModulePass *ptr = createProfileLoaderPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createProfileEstimatorPass();
static KMETHOD LLVM_createProfileEstimatorPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createProfileEstimatorPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createProfileVerifierPass();
static KMETHOD LLVM_createProfileVerifierPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createProfileVerifierPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ModulePass LLVM.createPathProfileLoaderPass();
static KMETHOD LLVM_createPathProfileLoaderPass(CTX, ksfp_t *sfp _RIX)
{
	ModulePass *ptr = createPathProfileLoaderPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ModulePass LLVM.createPathProfileVerifierPass();
static KMETHOD LLVM_createPathProfileVerifierPass(CTX, ksfp_t *sfp _RIX)
{
	ModulePass *ptr = createPathProfileVerifierPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createLazyValueInfoPass();
static KMETHOD LLVM_createLazyValueInfoPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createLazyValueInfoPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## LoopPass LLVM.createLoopDependenceAnalysisPass();
static KMETHOD LLVM_createLoopDependenceAnalysisPass(CTX, ksfp_t *sfp _RIX)
{
	LoopPass *ptr = createLoopDependenceAnalysisPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createInstCountPass();
static KMETHOD LLVM_createInstCountPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createInstCountPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createDbgInfoPrinterPass();
static KMETHOD LLVM_createDbgInfoPrinterPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createDbgInfoPrinterPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createRegionInfoPass();
static KMETHOD LLVM_createRegionInfoPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createRegionInfoPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ModulePass LLVM.createModuleDebugInfoPrinterPass();
static KMETHOD LLVM_createModuleDebugInfoPrinterPass(CTX, ksfp_t *sfp _RIX)
{
	ModulePass *ptr = createModuleDebugInfoPrinterPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createMemDepPrinter();
static KMETHOD LLVM_createMemDepPrinter(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createMemDepPrinter();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createPostDomTree();
static KMETHOD LLVM_createPostDomTree(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createPostDomTree();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createRegionViewerPass();
static KMETHOD LLVM_createRegionViewerPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createRegionViewerPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createRegionOnlyViewerPass();
static KMETHOD LLVM_createRegionOnlyViewerPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createRegionOnlyViewerPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createRegionPrinterPass();
static KMETHOD LLVM_createRegionPrinterPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createRegionPrinterPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createRegionOnlyPrinterPass();
static KMETHOD LLVM_createRegionOnlyPrinterPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createRegionOnlyPrinterPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createLintPass();
static KMETHOD LLVM_createLintPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createLintPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

////## ModulePass LLVM.createPrintModulePass(raw_ostream *OS);
//KMETHOD LLVM_createPrintModulePass(CTX, ksfp_t *sfp _RIX)
//{
//	raw_ostream **OS = konoha::object_cast<raw_ostream *>(sfp[0].p);
//	ModulePass *ptr = createPrintModulePass(*OS);
//	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
//	RETURN_(p);
//}
//
////## FunctionPass LLVM.createPrintFunctionPass(String banner, OutputStream os, boolean deleteStream);
//KMETHOD LLVM_createPrintFunctionPass(CTX, ksfp_t *sfp _RIX)
//{
//	String *banner = konoha::object_cast<String *>(sfp[0].p);
//	OutputStream *os = konoha::object_cast<OutputStream *>(sfp[1].p);
//	bool deleteStream = sfp[2].bvalue;
//	FunctionPass *ptr = createPrintFunctionPass(banner,os,deleteStream);
//	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
//	RETURN_(p);
//}

////## ModulePass LLVM.createEdgeProfilerPass();
//KMETHOD LLVM_createEdgeProfilerPass(CTX, ksfp_t *sfp _RIX)
//{
//	ModulePass *ptr = createEdgeProfilerPass();
//	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
//	RETURN_(p);
//}

////## ModulePass LLVM.createOptimalEdgeProfilerPass();
//KMETHOD LLVM_createOptimalEdgeProfilerPass(CTX, ksfp_t *sfp _RIX)
//{
//	ModulePass *ptr = createOptimalEdgeProfilerPass();
//	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
//	RETURN_(p);
//}

////## ModulePass LLVM.createPathProfilerPass();
//KMETHOD LLVM_createPathProfilerPass(CTX, ksfp_t *sfp _RIX)
//{
//	ModulePass *ptr = createPathProfilerPass();
//	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
//	RETURN_(p);
//}

////## ModulePass LLVM.createGCOVProfilerPass(boolean emitNotes, boolean emitData, boolean use402Format);
//KMETHOD LLVM_createGCOVProfilerPass(CTX, ksfp_t *sfp _RIX)
//{
//	bool emitNotes = sfp[0].bvalue;
//	bool emitData = sfp[1].bvalue;
//	bool use402Format = sfp[2].bvalue;
//	ModulePass *ptr = createGCOVProfilerPass(emitNotes,emitData,use402Format);
//	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
//	RETURN_(p);
//}

//## ModulePass LLVM.createStripSymbolsPass(bool onlyDebugInfo);
static KMETHOD LLVM_createStripSymbolsPass(CTX, ksfp_t *sfp _RIX)
{
	bool onlyDebugInfo = sfp[0].bvalue;
	ModulePass *ptr = createStripSymbolsPass(onlyDebugInfo);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ModulePass LLVM.createStripNonDebugSymbolsPass();
static KMETHOD LLVM_createStripNonDebugSymbolsPass(CTX, ksfp_t *sfp _RIX)
{
	ModulePass *ptr = createStripNonDebugSymbolsPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ModulePass LLVM.createStripDeadDebugInfoPass();
static KMETHOD LLVM_createStripDeadDebugInfoPass(CTX, ksfp_t *sfp _RIX)
{
	ModulePass *ptr = createStripDeadDebugInfoPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ModulePass LLVM.createConstantMergePass();
static KMETHOD LLVM_createConstantMergePass(CTX, ksfp_t *sfp _RIX)
{
	ModulePass *ptr = createConstantMergePass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ModulePass LLVM.createGlobalOptimizerPass();
static KMETHOD LLVM_createGlobalOptimizerPass(CTX, ksfp_t *sfp _RIX)
{
	ModulePass *ptr = createGlobalOptimizerPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ModulePass LLVM.createGlobalDCEPass();
static KMETHOD LLVM_createGlobalDCEPass(CTX, ksfp_t *sfp _RIX)
{
	ModulePass *ptr = createGlobalDCEPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createFunctionInliningPass(int threshold);
static KMETHOD LLVM_createFunctionInliningPass(CTX, ksfp_t *sfp _RIX)
{
	int threshold = sfp[0].ivalue;
	Pass *ptr = createFunctionInliningPass(threshold);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createAlwaysInlinerPass();
static KMETHOD LLVM_createAlwaysInlinerPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createAlwaysInlinerPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createPruneEHPass();
static KMETHOD LLVM_createPruneEHPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createPruneEHPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ModulePass LLVM.createInternalizePass(bool allButMain);
static KMETHOD LLVM_createInternalizePass(CTX, ksfp_t *sfp _RIX)
{
	bool allButMain = sfp[0].bvalue;
	ModulePass *ptr = createInternalizePass(allButMain);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ModulePass LLVM.createDeadArgEliminationPass();
static KMETHOD LLVM_createDeadArgEliminationPass(CTX, ksfp_t *sfp _RIX)
{
	ModulePass *ptr = createDeadArgEliminationPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createArgumentPromotionPass(int maxElements);
static KMETHOD LLVM_createArgumentPromotionPass(CTX, ksfp_t *sfp _RIX)
{
	int maxElements = sfp[0].ivalue;
	Pass *ptr = createArgumentPromotionPass(maxElements);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ModulePass LLVM.createIPConstantPropagationPass();
static KMETHOD LLVM_createIPConstantPropagationPass(CTX, ksfp_t *sfp _RIX)
{
	ModulePass *ptr = createIPConstantPropagationPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ModulePass LLVM.createIPSCCPPass();
static KMETHOD LLVM_createIPSCCPPass(CTX, ksfp_t *sfp _RIX)
{
	ModulePass *ptr = createIPSCCPPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createLoopExtractorPass();
static KMETHOD LLVM_createLoopExtractorPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createLoopExtractorPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createSingleLoopExtractorPass();
static KMETHOD LLVM_createSingleLoopExtractorPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createSingleLoopExtractorPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ModulePass LLVM.createBlockExtractorPass();
static KMETHOD LLVM_createBlockExtractorPass(CTX, ksfp_t *sfp _RIX)
{
	ModulePass *ptr = createBlockExtractorPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ModulePass LLVM.createStripDeadPrototypesPass();
static KMETHOD LLVM_createStripDeadPrototypesPass(CTX, ksfp_t *sfp _RIX)
{
	ModulePass *ptr = createStripDeadPrototypesPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createFunctionAttrsPass();
static KMETHOD LLVM_createFunctionAttrsPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createFunctionAttrsPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ModulePass LLVM.createMergeFunctionsPass();
static KMETHOD LLVM_createMergeFunctionsPass(CTX, ksfp_t *sfp _RIX)
{
	ModulePass *ptr = createMergeFunctionsPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ModulePass LLVM.createPartialInliningPass();
static KMETHOD LLVM_createPartialInliningPass(CTX, ksfp_t *sfp _RIX)
{
	ModulePass *ptr = createPartialInliningPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createConstantPropagationPass();
static KMETHOD LLVM_createConstantPropagationPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createConstantPropagationPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createSCCPPass();
static KMETHOD LLVM_createSCCPPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createSCCPPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createDeadInstEliminationPass();
static KMETHOD LLVM_createDeadInstEliminationPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createDeadInstEliminationPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createDeadCodeEliminationPass();
static KMETHOD LLVM_createDeadCodeEliminationPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createDeadCodeEliminationPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createDeadStoreEliminationPass();
static KMETHOD LLVM_createDeadStoreEliminationPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createDeadStoreEliminationPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createAggressiveDCEPass();
static KMETHOD LLVM_createAggressiveDCEPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createAggressiveDCEPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createScalarReplAggregatesPass(int threshold);
static KMETHOD LLVM_createScalarReplAggregatesPass(CTX, ksfp_t *sfp _RIX)
{
	int threshold = sfp[0].ivalue;
	FunctionPass *ptr = createScalarReplAggregatesPass(threshold);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createIndVarSimplifyPass();
static KMETHOD LLVM_createIndVarSimplifyPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createIndVarSimplifyPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createInstructionCombiningPass();
static KMETHOD LLVM_createInstructionCombiningPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createInstructionCombiningPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createLICMPass();
static KMETHOD LLVM_createLICMPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createLICMPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createLoopUnswitchPass(bool optimizeForSize);
static KMETHOD LLVM_createLoopUnswitchPass(CTX, ksfp_t *sfp _RIX)
{
	bool optimizeForSize = sfp[0].bvalue;
	Pass *ptr = createLoopUnswitchPass(optimizeForSize);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createLoopInstSimplifyPass();
static KMETHOD LLVM_createLoopInstSimplifyPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createLoopInstSimplifyPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createLoopUnrollPass(int threshold, int count, int allowPartial);
static KMETHOD LLVM_createLoopUnrollPass(CTX, ksfp_t *sfp _RIX)
{
	int threshold = sfp[0].ivalue;
	int count = sfp[1].ivalue;
	int allowPartial = sfp[2].ivalue;
	Pass *ptr = createLoopUnrollPass(threshold,count,allowPartial);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createLoopRotatePass();
static KMETHOD LLVM_createLoopRotatePass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createLoopRotatePass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createLoopIdiomPass();
static KMETHOD LLVM_createLoopIdiomPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createLoopIdiomPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createPromoteMemoryToRegisterPass();
static KMETHOD LLVM_createPromoteMemoryToRegisterPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createPromoteMemoryToRegisterPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createDemoteRegisterToMemoryPass();
static KMETHOD LLVM_createDemoteRegisterToMemoryPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createDemoteRegisterToMemoryPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createReassociatePass();
static KMETHOD LLVM_createReassociatePass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createReassociatePass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createJumpThreadingPass();
static KMETHOD LLVM_createJumpThreadingPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createJumpThreadingPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createCFGSimplificationPass();
static KMETHOD LLVM_createCFGSimplificationPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createCFGSimplificationPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createBreakCriticalEdgesPass();
static KMETHOD LLVM_createBreakCriticalEdgesPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createBreakCriticalEdgesPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createLoopSimplifyPass();
static KMETHOD LLVM_createLoopSimplifyPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createLoopSimplifyPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createTailCallEliminationPass();
static KMETHOD LLVM_createTailCallEliminationPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createTailCallEliminationPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createLowerSwitchPass();
static KMETHOD LLVM_createLowerSwitchPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createLowerSwitchPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createBlockPlacementPass();
static KMETHOD LLVM_createBlockPlacementPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createBlockPlacementPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createLCSSAPass();
static KMETHOD LLVM_createLCSSAPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createLCSSAPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createEarlyCSEPass();
static KMETHOD LLVM_createEarlyCSEPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createEarlyCSEPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createGVNPass(bool noLoads);
static KMETHOD LLVM_createGVNPass(CTX, ksfp_t *sfp _RIX)
{
	bool noLoads = sfp[0].bvalue;
	FunctionPass *ptr = createGVNPass(noLoads);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createMemCpyOptPass();
static KMETHOD LLVM_createMemCpyOptPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createMemCpyOptPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createLoopDeletionPass();
static KMETHOD LLVM_createLoopDeletionPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createLoopDeletionPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createSimplifyLibCallsPass();
static KMETHOD LLVM_createSimplifyLibCallsPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createSimplifyLibCallsPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createInstructionNamerPass();
static KMETHOD LLVM_createInstructionNamerPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createInstructionNamerPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createSinkingPass();
static KMETHOD LLVM_createSinkingPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createSinkingPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createLowerAtomicPass();
static KMETHOD LLVM_createLowerAtomicPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createLowerAtomicPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createCorrelatedValuePropagationPass();
static KMETHOD LLVM_createCorrelatedValuePropagationPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createCorrelatedValuePropagationPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createObjCARCExpandPass();
static KMETHOD LLVM_createObjCARCExpandPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createObjCARCExpandPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createObjCARCContractPass();
static KMETHOD LLVM_createObjCARCContractPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createObjCARCContractPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createObjCARCOptPass();
static KMETHOD LLVM_createObjCARCOptPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createObjCARCOptPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createInstructionSimplifierPass();
static KMETHOD LLVM_createInstructionSimplifierPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createInstructionSimplifierPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## FunctionPass LLVM.createLowerExpectIntrinsicPass();
static KMETHOD LLVM_createLowerExpectIntrinsicPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createLowerExpectIntrinsicPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## Pass LLVM.createUnifyFunctionExitNodesPass();
static KMETHOD LLVM_createUnifyFunctionExitNodesPass(CTX, ksfp_t *sfp _RIX)
{
	Pass *ptr = createUnifyFunctionExitNodesPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ImmutablePass LLVM.createTypeBasedAliasAnalysisPass();
static KMETHOD LLVM_createTypeBasedAliasAnalysisPass(CTX, ksfp_t *sfp _RIX)
{
	ImmutablePass *ptr = createTypeBasedAliasAnalysisPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ImmutablePass LLVM.createBasicAliasAnalysisPass();
static KMETHOD LLVM_createBasicAliasAnalysisPass(CTX, ksfp_t *sfp _RIX)
{
	ImmutablePass *ptr = createBasicAliasAnalysisPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//## ImmutablePass LLVM.createVerifierPass();
static KMETHOD LLVM_createVerifierPass(CTX, ksfp_t *sfp _RIX)
{
	FunctionPass *ptr = createVerifierPass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

#ifdef USE_LLVM_3_1
//## BasicBlockPass LLVM.createBBVectorizePass();
static KMETHOD LLVM_createBBVectorizePass(CTX, ksfp_t *sfp _RIX)
{
	BasicBlockPass *ptr = createBBVectorizePass();
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}
#endif

//FunctionType Intrinsic::getType(int id, Type[] args);
static KMETHOD Intrinsic_getType(CTX, ksfp_t *sfp _RIX)
{
	Intrinsic::ID id = (Intrinsic::ID) sfp[1].ivalue;
	kArray *args = sfp[2].a;
	std::vector<Type*> List;
	konoha::convert_array(List, args);
	FunctionType *ptr = Intrinsic::getType(getGlobalContext(), id, List);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

//Function     Intrinsic::getDeclaration(Module m, int id, Type[] args);
static KMETHOD Intrinsic_getDeclaration(CTX, ksfp_t *sfp _RIX)
{
	Module *m = konoha::object_cast<Module *>(sfp[1].p);
	Intrinsic::ID id = (Intrinsic::ID) sfp[2].ivalue;
	kArray *args = sfp[3].a;
	std::vector<Type*> List;
	konoha::convert_array(List, args);
	Function *ptr = Intrinsic::getDeclaration(m, id, List);
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(ptr) K_RIXPARAM);
	RETURN_(p);
}

static KMETHOD LLVM_parseBitcodeFile(CTX, ksfp_t *sfp _RIX)
{
	kString *Str = sfp[1].s;
	LLVMContext &Context = getGlobalContext();
	std::string ErrMsg;
	OwningPtr<MemoryBuffer> BufferPtr;
	std::string fname(S_text(Str));
	if (error_code ec = MemoryBuffer::getFile(fname, BufferPtr)) {
		std::cout << "Could not open file" << ec.message() << std::endl;
	}
	MemoryBuffer *Buffer = BufferPtr.take();
	//Module *m = getLazyBitcodeModule(Buffer, Context, &ErrMsg);
	Module *m = ParseBitcodeFile(Buffer, Context, &ErrMsg);
	if (!m) {
		std::cout << "error" << ErrMsg << std::endl;
	}
	kRawPtr *p = new_ReturnCppObject(_ctx, sfp, WRAP(m) K_RIXPARAM);
	RETURN_(p);
}

//TODO Scriptnize
static KMETHOD Instruction_setMetadata(CTX _UNUSED_, ksfp_t *sfp _RIX)
{
	Instruction *inst = konoha::object_cast<Instruction *>(sfp[0].p);
	Module *m = konoha::object_cast<Module *>(sfp[1].p);
	kString *Str = sfp[2].s;
	kint_t N = Int_to(kint_t,sfp[3]);
	Value *Info[] = {
		ConstantInt::get(Type::getInt32Ty(getGlobalContext()), N)
	};
	LLVMContext &Context = getGlobalContext();
	MDNode *node = MDNode::get(Context, Info);
	NamedMDNode *NMD = m->getOrInsertNamedMetadata(S_text(Str));
	unsigned KindID = Context.getMDKindID(S_text(Str));
	NMD->addOperand(node);
	inst->setMetadata(KindID, node);
	RETURNvoid_();
}

//static knh_IntData_t IntIntrinsic[] = {
//	{"Pow"  ,    (int) Intrinsic::pow},
//	{"Sqrt" ,    (int) Intrinsic::sqrt},
//	{"Exp"  ,    (int) Intrinsic::exp},
//	{"Log10",    (int) Intrinsic::log10},
//	{"Log"  ,    (int) Intrinsic::log},
//	{"Sin"  ,    (int) Intrinsic::sin},
//	{"Cos"  ,    (int) Intrinsic::cos},
//	{NULL, 0}
//};
//
//static knh_IntData_t IntGlobalVariable[] = {
//	{"ExternalLinkage",                 GlobalValue::ExternalLinkage},
//	{"AvailableExternallyLinkage",      GlobalValue::AvailableExternallyLinkage},
//	{"LinkOnceAnyLinkage",              GlobalValue::LinkOnceODRLinkage},
//	{"WeakAnyLinkage",                  GlobalValue::WeakAnyLinkage},
//	{"WeakODRLinkage",                  GlobalValue::WeakODRLinkage},
//	{"AppendingLinkage",                GlobalValue::AppendingLinkage},
//	{"InternalLinkage",                 GlobalValue::InternalLinkage},
//	{"PrivateLinkage",                  GlobalValue::PrivateLinkage},
//	{"LinkerPrivateLinkage",            GlobalValue::LinkerPrivateLinkage},
//	{"LinkerPrivateWeakLinkage",        GlobalValue::LinkerPrivateWeakLinkage},
//	{"LinkerPrivateWeakDefAutoLinkage", GlobalValue::LinkerPrivateWeakDefAutoLinkage},
//	{"DLLImportLinkage",                GlobalValue::DLLImportLinkage},
//	{"DLLExportLinkage",                GlobalValue::DLLExportLinkage},
//	{"ExternalWeakLinkage",             GlobalValue::ExternalWeakLinkage},
//	{"CommonLinkage",                   GlobalValue::CommonLinkage},
//	{NULL, 0}
//};
//
//void defGlobalValue(CTX _UNUSED_, kcid_t cid _UNUSED_, kclassdef_t *cdef)
//{
//	cdef->name = "GlobalValue";
//}
//
//void constGlobalValue(CTX, kcid_t cid, const knh_LoaderAPI_t *kapi)
//{
//	kapi->loadClassIntConst(_ctx, cid, IntGlobalVariable);
//}
//
//void defIntrinsic(CTX _UNUSED_, kcid_t cid _UNUSED_, kclassdef_t *cdef)
//{
//	cdef->name = "Intrinsic";
//}
//
//void constIntrinsic(CTX, kcid_t cid, const knh_LoaderAPI_t *kapi)
//{
//	kapi->loadClassIntConst(_ctx, cid, IntIntrinsic);
//}
//
//#define C_(S) {#S , S ## _i}
//using namespace llvm::Attribute;
//static const knh_IntData_t IntAttributes[] = {
//	C_(None),
//	C_(ZExt),
//	C_(SExt),
//	C_(NoReturn),
//	C_(InReg),
//	C_(StructRet),
//	C_(NoUnwind),
//	C_(NoAlias),
//	C_(ByVal),
//	C_(Nest),
//	C_(ReadNone),
//	C_(ReadOnly),
//	C_(NoInline),
//	C_(AlwaysInline),
//	C_(OptimizeForSize),
//	C_(StackProtect),
//	C_(StackProtectReq),
//	C_(Alignment),
//	C_(NoCapture),
//	C_(NoRedZone),
//	C_(NoImplicitFloat),
//	C_(Naked),
//	C_(InlineHint),
//	C_(StackAlignment),
//	C_(ReturnsTwice),
//	C_(UWTable),
//	C_(NonLazyBind),
//	{NULL, 0}
//};
//#undef C_
//
//void defAttributes(CTX _UNUSED_, kcid_t cid _UNUSED_, kclassdef_t *cdef)
//{
//	cdef->name = "Attributes";
//}
//
//void constAttributes(CTX _UNUSED_, kcid_t cid _UNUSED_, const knh_LoaderAPI_t *kapi)
//{
//	kapi->loadClassIntConst(_ctx, cid, IntAttributes);
//}

#define _Public   kMethod_Public
#define _Const    kMethod_Const
#define _Coercion kMethod_Coercion
#define _F(F)   (intptr_t)(F)

static kbool_t llvm_initPackage(CTX, struct kKonohaSpace *ks, int argc, const char **args, kline_t pline)
{

	static const char *TypeDefName[] = {
		"Type",
		"IntegerType",
		"PointerType",
	};
	{
		static KCLASSDEF TypeDef;
		bzero(&TypeDef, sizeof(KCLASSDEF));
		TypeDef.cid  = CLASS_newid;
		TypeDef.init = Type_init;
		TypeDef.free = Type_free;
		for (int i = 0; i < 3; ++i) {
			TypeDef.structname = TypeDefName[i];
			kaddClassDef(NULL, &TypeDef, 0);
		}
	}
	static KCLASSDEF BasicBlockDef = {
		"BasicBlock"/*structname*/,
		CLASS_newid/*cid*/,  0/*cflag*/,
		0/*bcid*/, 0/*supcid*/, 0/*cstruct_size*/,
		NULL/*fields*/, 0/*fsize*/, 0/*fallocsize*/,
		0/*packid*/, 0/*packdom*/,
		0/*init*/,
		0/*reftrace*/,
		0/*free*/,
		0/*fnull*/,
		0/*p*/, 0/*unbox*/,
		BasicBlock_compareTo/*compareTo*/,
		0/*getkey*/,
		0/*hashCode*/,
		0/*initdef*/
	};
	static KCLASSDEF PassManagerBuilderDef = {
		"BasicBlock"/*structname*/,
		CLASS_newid/*cid*/,  0/*cflag*/,
		0/*bcid*/, 0/*supcid*/, 0/*cstruct_size*/,
		NULL/*fields*/, 0/*fsize*/, 0/*fallocsize*/,
		0/*packid*/, 0/*packdom*/,
		PassManagerBuilder_ptr_init/*init*/,
		0/*reftrace*/,
		PassManagerBuilder_ptr_free/*free*/,
		0/*fnull*/,
		0/*p*/, 0/*unbox*/,
		0/*compareTo*/,
		0/*getkey*/,
		0/*hashCode*/,
		0/*initdef*/
	};
	static KCLASSDEF PassManagerDef = {
		"PassManager"/*structname*/,
		CLASS_newid/*cid*/,  0/*cflag*/,
		0/*bcid*/, 0/*supcid*/, 0/*cstruct_size*/,
		NULL/*fields*/, 0/*fsize*/, 0/*fallocsize*/,
		0/*packid*/, 0/*packdom*/,
		PassManager_ptr_init/*init*/,
		0/*reftrace*/,
		PassManager_ptr_free/*free*/,
		0/*fnull*/,
		0/*p*/, 0/*unbox*/,
		0/*compareTo*/,
		0/*getkey*/,
		0/*hashCode*/,
		0/*initdef*/
	};
	static KCLASSDEF FunctionPassManagerDef = {
		"FunctionPassManager"/*structname*/,
		CLASS_newid/*cid*/,  0/*cflag*/,
		0/*bcid*/, 0/*supcid*/, 0/*cstruct_size*/,
		NULL/*fields*/, 0/*fsize*/, 0/*fallocsize*/,
		0/*packid*/, 0/*packdom*/,
		FunctionPassManager_ptr_init/*init*/,
		0/*reftrace*/,
		FunctionPassManager_ptr_free/*free*/,
		0/*fnull*/,
		0/*p*/, 0/*unbox*/,
		0/*compareTo*/,
		0/*getkey*/,
		0/*hashCode*/,
		0/*initdef*/
	};
	kaddClassDef(NULL, &BasicBlockDef, pline);
	kaddClassDef(NULL, &PassManagerDef, pline);
	kaddClassDef(NULL, &PassManagerBuilderDef, pline);
	kaddClassDef(NULL, &FunctionPassManagerDef, pline);
	intptr_t methoddata[] = {
		//_Public|_Static, _F(Type_getVoidTy), TY_Type, MN_("getVoidTy"), 0, 
		//_Public|_Static, _F(Type_getLabelTy), TY_Type, MN_("getLabelTy"), 0, 
		//_Public|_Static, _F(Type_getFloatTy), TY_Type, MN_("getFloatTy"), 0, 
		//_Public|_Static, _F(Type_getDoubleTy), TY_Type, MN_("getDoubleTy"), 0, 
		//_Public|_Static, _F(Type_getMetadataTy), TY_Type, MN_("getMetadataTy"), 0, 
		//_Public|_Static, _F(Type_getX86_FP80Ty), TY_Type, MN_("getX86_FP80Ty"), 0, 
		//_Public|_Static, _F(Type_getFP128Ty), TY_Type, MN_("getFP128Ty"), 0, 
		//_Public|_Static, _F(Type_getPPC_FP128Ty), TY_Type, MN_("getPPC_FP128Ty"), 0, 
		//_Public|_Static, _F(Type_getX86_MMXTy), TY_Type, MN_("getX86_MMXTy"), 0, 
		//_Public|_Static, _F(Type_getInt1Ty), TY_Type, MN_("getInt1Ty"), 0, 
		//_Public|_Static, _F(Type_getInt8Ty), TY_Type, MN_("getInt8Ty"), 0, 
		//_Public|_Static, _F(Type_getInt16Ty), TY_Type, MN_("getInt16Ty"), 0, 
		//_Public|_Static, _F(Type_getInt32Ty), TY_Type, MN_("getInt32Ty"), 0, 
		//_Public|_Static, _F(Type_getInt64Ty), TY_Type, MN_("getInt64Ty"), 0, 
		//_Public|_Static, _F(PointerType_get), TY_PointerType, MN_("get"), 1, TY_Type, MN_("type"),
		//_Public|_Static, _F(Type_getFloatPtrTy), TY_Type, MN_("getFloatPtrTy"), 0, 
		//_Public|_Static, _F(Type_getDoublePtrTy), TY_Type, MN_("getDoublePtrTy"), 0, 
		//_Public|_Static, _F(Type_getX86_FP80PtrTy), TY_Type, MN_("getX86_FP80PtrTy"), 0, 
		//_Public|_Static, _F(Type_getFP128PtrTy), TY_Type, MN_("getFP128PtrTy"), 0, 
		//_Public|_Static, _F(Type_getPPC_FP128PtrTy), TY_Type, MN_("getPPC_FP128PtrTy"), 0, 
		//_Public|_Static, _F(Type_getX86_MMXPtrTy), TY_Type, MN_("getX86_MMXPtrTy"), 0, 
		//_Public|_Static, _F(Type_getInt1PtrTy), TY_Type, MN_("getInt1PtrTy"), 0, 
		//_Public|_Static, _F(Type_getInt8PtrTy), TY_Type, MN_("getInt8PtrTy"), 0, 
		//_Public|_Static, _F(Type_getInt16PtrTy), TY_Type, MN_("getInt16PtrTy"), 0, 
		//_Public|_Static, _F(Type_getInt32PtrTy), TY_Type, MN_("getInt32PtrTy"), 0, 
		//_Public|_Static, _F(Type_getInt64PtrTy), TY_Type, MN_("getInt64PtrTy"), 0, 
		//_Public, _F(IRBuilder_new), TY_IRBuilder, MN_("new"), 1, TY_BasicBlock, MN_("bb"),
		//_Public, _F(IRBuilder_createRetVoid), TY_IRBuilder, MN_("createRetVoid"), 0, 
		//_Public, _F(IRBuilder_createRet), TY_IRBuilder, MN_("createRet"), 1, TY_Value, MN_("v"),
		//_Public, _F(IRBuilder_createBr), TY_IRBuilder, MN_("createBr"), 1, TY_BasicBlock, MN_("dest"),
		//_Public, _F(IRBuilder_createCondBr), TY_IRBuilder, MN_("createCondBr"), 3, TY_Value, MN_("cond"),TY_BasicBlock, MN_("trueBB"),TY_BasicBlock, MN_("falseBB"),
		//_Public, _F(IRBuilder_createSwitch), TY_IRBuilder, MN_("createSwitch"), 2, TY_Value, MN_("v"),TY_BasicBlock, MN_("dest"),
		//_Public, _F(IRBuilder_createIndirectBr), TY_IRBuilder, MN_("createIndirectBr"), 1, TY_Value, MN_("addr"),
		//_Public, _F(IRBuilder_createInvoke0), TY_IRBuilder, MN_("createInvoke0"), 3, TY_Value, MN_("callee"),TY_BasicBlock, MN_("normalDest"),TY_BasicBlock, MN_("unwindDest"),
		//_Public, _F(IRBuilder_createInvoke1), TY_IRBuilder, MN_("createInvoke1"), 4, TY_Value, MN_("callee"),TY_BasicBlock, MN_("normalDest"),TY_BasicBlock, MN_("unwindDest"),TY_Value, MN_("arg1"),
		//_Public, _F(IRBuilder_createInvoke3), TY_IRBuilder, MN_("createInvoke3"), 6, TY_Value, MN_("callee"),TY_BasicBlock, MN_("normalDest"),TY_BasicBlock, MN_("unwindDest"),TY_Value, MN_("arg1"),TY_Value, MN_("arg2"),TY_Value, MN_("arg3"),
		//_Public, _F(IRBuilder_createUnreachable), TY_IRBuilder, MN_("createUnreachable"), 0, 
		//_Public, _F(IRBuilder_createAdd), TY_IRBuilder, MN_("createAdd"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createNSWAdd), TY_IRBuilder, MN_("createNSWAdd"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createNUWAdd), TY_IRBuilder, MN_("createNUWAdd"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createFAdd), TY_IRBuilder, MN_("createFAdd"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createSub), TY_IRBuilder, MN_("createSub"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createNSWSub), TY_IRBuilder, MN_("createNSWSub"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createNUWSub), TY_IRBuilder, MN_("createNUWSub"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createFSub), TY_IRBuilder, MN_("createFSub"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createMul), TY_IRBuilder, MN_("createMul"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createNSWMul), TY_IRBuilder, MN_("createNSWMul"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createNUWMul), TY_IRBuilder, MN_("createNUWMul"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createFMul), TY_IRBuilder, MN_("createFMul"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createUDiv), TY_IRBuilder, MN_("createUDiv"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createExactUDiv), TY_IRBuilder, MN_("createExactUDiv"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createSDiv), TY_IRBuilder, MN_("createSDiv"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createExactSDiv), TY_IRBuilder, MN_("createExactSDiv"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createFDiv), TY_IRBuilder, MN_("createFDiv"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createURem), TY_IRBuilder, MN_("createURem"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createSRem), TY_IRBuilder, MN_("createSRem"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createFRem), TY_IRBuilder, MN_("createFRem"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createShl), TY_IRBuilder, MN_("createShl"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createLShr), TY_IRBuilder, MN_("createLShr"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createAShr), TY_IRBuilder, MN_("createAShr"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createAnd), TY_IRBuilder, MN_("createAnd"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createOr), TY_IRBuilder, MN_("createOr"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createXor), TY_IRBuilder, MN_("createXor"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createNeg), TY_IRBuilder, MN_("createNeg"), 1, TY_Value, MN_("v"),
		//_Public, _F(IRBuilder_createNSWNeg), TY_IRBuilder, MN_("createNSWNeg"), 1, TY_Value, MN_("v"),
		//_Public, _F(IRBuilder_createNUWNeg), TY_IRBuilder, MN_("createNUWNeg"), 1, TY_Value, MN_("v"),
		//_Public, _F(IRBuilder_createFNeg), TY_IRBuilder, MN_("createFNeg"), 1, TY_Value, MN_("v"),
		//_Public, _F(IRBuilder_createNot), TY_IRBuilder, MN_("createNot"), 1, TY_Value, MN_("v"),
		//_Public, _F(IRBuilder_createAlloca), TY_IRBuilder, MN_("createAlloca"), 2, TY_Type, MN_("ty"),TY_Value, MN_("arraySize"),
		//_Public, _F(AllocaInst_new), TY_AllocaInst, MN_("new"), 2, TY_Type, MN_("ty"),TY_Value, MN_("arraySize"),
		//_Public, _F(IRBuilder_createLoad), TY_IRBuilder, MN_("createLoad"), 2, TY_Value, MN_("ptr"),TY_boolean, MN_("isVolatile"),
		//_Public, _F(LoadInst_new), TY_LoadInst, MN_("new"), 1, TY_Value, MN_("ptr"),
		//_Public, _F(IRBuilder_createStore), TY_IRBuilder, MN_("createStore"), 3, TY_Value, MN_("val"),TY_Value, MN_("ptr"),TY_boolean, MN_("isVolatile"),
		//_Public, _F(StoreInst_new), TY_StoreInst, MN_("new"), 2, TY_Value, MN_("val"),TY_Value, MN_("ptr"),
		//_Public|_Static, _F(GetElementPtrInst_create), TY_GetElementPtrInst, MN_("create"), 2, TY_Value, MN_("ptr"),TY_Array<Value>, MN_("idxList"),
		//_Public|_Static, _F(GetElementPtrInst_createInBounds), TY_GetElementPtrInst, MN_("createInBounds"), 2, TY_Value, MN_("ptr"),TY_Array<Value>, MN_("idxList"),
		//_Public, _F(IRBuilder_createGEP), TY_IRBuilder, MN_("createGEP"), 2, TY_Value, MN_("ptr"),TY_Array<Value>, MN_("idxList"),
		//_Public, _F(IRBuilder_createInBoundsGEP), TY_IRBuilder, MN_("createInBoundsGEP"), 2, TY_Value, MN_("ptr"),TY_Array<Value>, MN_("idxList"),
		//_Public, _F(IRBuilder_createGEP1), TY_IRBuilder, MN_("createGEP1"), 2, TY_Value, MN_("ptr"),TY_Value, MN_("idx"),
		//_Public, _F(IRBuilder_createInBoundsGEP1), TY_IRBuilder, MN_("createInBoundsGEP1"), 2, TY_Value, MN_("ptr"),TY_Value, MN_("idx"),
		//_Public, _F(IRBuilder_createConstGEP1_32), TY_IRBuilder, MN_("createConstGEP1_32"), 2, TY_Value, MN_("ptr"),TY_int, MN_("idx0"),
		//_Public, _F(IRBuilder_createConstInBoundsGEP1_32), TY_IRBuilder, MN_("createConstInBoundsGEP1_32"), 2, TY_Value, MN_("ptr"),TY_int, MN_("idx0"),
		//_Public, _F(IRBuilder_createConstGEP2_32), TY_IRBuilder, MN_("createConstGEP2_32"), 3, TY_Value, MN_("ptr"),TY_int, MN_("idx0"),TY_int, MN_("idx1"),
		//_Public, _F(IRBuilder_createConstInBoundsGEP2_32), TY_IRBuilder, MN_("createConstInBoundsGEP2_32"), 3, TY_Value, MN_("ptr"),TY_int, MN_("idx0"),TY_int, MN_("idx1"),
		//_Public, _F(IRBuilder_createConstGEP1_64), TY_IRBuilder, MN_("createConstGEP1_64"), 2, TY_Value, MN_("ptr"),TY_int, MN_("idx0"),
		//_Public, _F(IRBuilder_createConstInBoundsGEP1_64), TY_IRBuilder, MN_("createConstInBoundsGEP1_64"), 2, TY_Value, MN_("ptr"),TY_int, MN_("idx0"),
		//_Public, _F(IRBuilder_createConstGEP2_64), TY_IRBuilder, MN_("createConstGEP2_64"), 3, TY_Value, MN_("ptr"),TY_int, MN_("idx0"),TY_int, MN_("idx1"),
		//_Public, _F(IRBuilder_createConstInBoundsGEP2_64), TY_IRBuilder, MN_("createConstInBoundsGEP2_64"), 3, TY_Value, MN_("ptr"),TY_int, MN_("idx0"),TY_int, MN_("idx1"),
		//_Public, _F(IRBuilder_createStructGEP), TY_IRBuilder, MN_("createStructGEP"), 2, TY_Value, MN_("ptr"),TY_int, MN_("idx"),
		//_Public, _F(IRBuilder_createGlobalString), TY_IRBuilder, MN_("createGlobalString"), 1, TY_String, MN_("str"),
		//_Public, _F(IRBuilder_createGlobalStringPtr), TY_IRBuilder, MN_("createGlobalStringPtr"), 1, TY_String, MN_("str"),
		//_Public, _F(IRBuilder_createTrunc), TY_IRBuilder, MN_("createTrunc"), 2, TY_Value, MN_("v"),TY_Type, MN_("destTy"),
		//_Public, _F(IRBuilder_createZExt), TY_IRBuilder, MN_("createZExt"), 2, TY_Value, MN_("v"),TY_Type, MN_("destTy"),
		//_Public, _F(IRBuilder_createSExt), TY_IRBuilder, MN_("createSExt"), 2, TY_Value, MN_("v"),TY_Type, MN_("destTy"),
		//_Public, _F(IRBuilder_createFPToUI), TY_IRBuilder, MN_("createFPToUI"), 2, TY_Value, MN_("v"),TY_Type, MN_("destTy"),
		//_Public, _F(IRBuilder_createFPToSI), TY_IRBuilder, MN_("createFPToSI"), 2, TY_Value, MN_("v"),TY_Type, MN_("destTy"),
		//_Public, _F(IRBuilder_createUIToFP), TY_IRBuilder, MN_("createUIToFP"), 2, TY_Value, MN_("v"),TY_Type, MN_("destTy"),
		//_Public, _F(IRBuilder_createSIToFP), TY_IRBuilder, MN_("createSIToFP"), 2, TY_Value, MN_("v"),TY_Type, MN_("destTy"),
		//_Public, _F(IRBuilder_createFPTrunc), TY_IRBuilder, MN_("createFPTrunc"), 2, TY_Value, MN_("v"),TY_Type, MN_("destTy"),
		//_Public, _F(IRBuilder_createFPExt), TY_IRBuilder, MN_("createFPExt"), 2, TY_Value, MN_("v"),TY_Type, MN_("destTy"),
		//_Public, _F(IRBuilder_createPtrToInt), TY_IRBuilder, MN_("createPtrToInt"), 2, TY_Value, MN_("v"),TY_Type, MN_("destTy"),
		//_Public, _F(IRBuilder_createIntToPtr), TY_IRBuilder, MN_("createIntToPtr"), 2, TY_Value, MN_("v"),TY_Type, MN_("destTy"),
		//_Public, _F(IRBuilder_createBitCast), TY_IRBuilder, MN_("createBitCast"), 2, TY_Value, MN_("v"),TY_Type, MN_("destTy"),
		//_Public, _F(IRBuilder_createZExtOrBitCast), TY_IRBuilder, MN_("createZExtOrBitCast"), 2, TY_Value, MN_("v"),TY_Type, MN_("destTy"),
		//_Public, _F(IRBuilder_createSExtOrBitCast), TY_IRBuilder, MN_("createSExtOrBitCast"), 2, TY_Value, MN_("v"),TY_Type, MN_("destTy"),
		//_Public, _F(IRBuilder_createTruncOrBitCast), TY_IRBuilder, MN_("createTruncOrBitCast"), 2, TY_Value, MN_("v"),TY_Type, MN_("destTy"),
		//_Public, _F(IRBuilder_createPointerCast), TY_IRBuilder, MN_("createPointerCast"), 2, TY_Value, MN_("v"),TY_Type, MN_("destTy"),
		//_Public, _F(IRBuilder_createIntCast), TY_IRBuilder, MN_("createIntCast"), 3, TY_Value, MN_("v"),TY_Type, MN_("destTy"),TY_boolean, MN_("isSigned"),
		//_Public, _F(IRBuilder_createFPCast), TY_IRBuilder, MN_("createFPCast"), 2, TY_Value, MN_("v"),TY_Type, MN_("destTy"),
		//_Public, _F(IRBuilder_createICmpEQ), TY_IRBuilder, MN_("createICmpEQ"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createICmpNE), TY_IRBuilder, MN_("createICmpNE"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createICmpUGT), TY_IRBuilder, MN_("createICmpUGT"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createICmpUGE), TY_IRBuilder, MN_("createICmpUGE"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createICmpULT), TY_IRBuilder, MN_("createICmpULT"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createICmpULE), TY_IRBuilder, MN_("createICmpULE"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createICmpSGT), TY_IRBuilder, MN_("createICmpSGT"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createICmpSGE), TY_IRBuilder, MN_("createICmpSGE"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createICmpSLT), TY_IRBuilder, MN_("createICmpSLT"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createICmpSLE), TY_IRBuilder, MN_("createICmpSLE"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createFCmpOEQ), TY_IRBuilder, MN_("createFCmpOEQ"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createFCmpOGT), TY_IRBuilder, MN_("createFCmpOGT"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createFCmpOGE), TY_IRBuilder, MN_("createFCmpOGE"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createFCmpOLT), TY_IRBuilder, MN_("createFCmpOLT"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createFCmpOLE), TY_IRBuilder, MN_("createFCmpOLE"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createFCmpONE), TY_IRBuilder, MN_("createFCmpONE"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createFCmpORD), TY_IRBuilder, MN_("createFCmpORD"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createFCmpUNO), TY_IRBuilder, MN_("createFCmpUNO"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createFCmpUEQ), TY_IRBuilder, MN_("createFCmpUEQ"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createFCmpUGT), TY_IRBuilder, MN_("createFCmpUGT"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createFCmpUGE), TY_IRBuilder, MN_("createFCmpUGE"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createFCmpULT), TY_IRBuilder, MN_("createFCmpULT"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createFCmpULE), TY_IRBuilder, MN_("createFCmpULE"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createFCmpUNE), TY_IRBuilder, MN_("createFCmpUNE"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_createPHI), TY_IRBuilder, MN_("createPHI"), 2, TY_Type, MN_("ty"),TY_int, MN_("numReservedValues"),
		//_Public, _F(PHINode_addIncoming), TY_PHINode, MN_("addIncoming"), 2, TY_Value, MN_("v"),TY_BasicBlock, MN_("bb"),
		//_Public, _F(IRBuilder_createCall1), TY_IRBuilder, MN_("createCall1"), 2, TY_Value, MN_("callee"),TY_Value, MN_("arg"),
		//_Public, _F(IRBuilder_createCall2), TY_IRBuilder, MN_("createCall2"), 3, TY_Value, MN_("callee"),TY_Value, MN_("arg1"),TY_Value, MN_("arg2"),
		//_Public, _F(IRBuilder_createCall3), TY_IRBuilder, MN_("createCall3"), 4, TY_Value, MN_("callee"),TY_Value, MN_("arg1"),TY_Value, MN_("arg2"),TY_Value, MN_("arg3"),
		//_Public, _F(IRBuilder_createCall4), TY_IRBuilder, MN_("createCall4"), 5, TY_Value, MN_("callee"),TY_Value, MN_("arg1"),TY_Value, MN_("arg2"),TY_Value, MN_("arg3"),TY_Value, MN_("arg4"),
		//_Public, _F(IRBuilder_createCall5), TY_IRBuilder, MN_("createCall5"), 6, TY_Value, MN_("callee"),TY_Value, MN_("arg1"),TY_Value, MN_("arg2"),TY_Value, MN_("arg3"),TY_Value, MN_("arg4"),TY_Value, MN_("arg5"),
		//_Public, _F(IRBuilder_createCall), TY_IRBuilder, MN_("createCall"), 2, TY_Value, MN_("callee"),TY_Array<Value>, MN_("args"),
		//_Public, _F(IRBuilder_createSelect), TY_IRBuilder, MN_("createSelect"), 3, TY_Value, MN_("c"),TY_Value, MN_("trueV"),TY_Value, MN_("falseV"),
		//_Public, _F(IRBuilder_createVAArg), TY_IRBuilder, MN_("createVAArg"), 2, TY_Value, MN_("list"),TY_Type, MN_("ty"),
		//_Public, _F(IRBuilder_createExtractElement), TY_IRBuilder, MN_("createExtractElement"), 2, TY_Value, MN_("vec"),TY_Value, MN_("idx"),
		//_Public, _F(IRBuilder_createInsertElement), TY_IRBuilder, MN_("createInsertElement"), 3, TY_Value, MN_("vec"),TY_Value, MN_("newElt"),TY_Value, MN_("idx"),
		//_Public, _F(IRBuilder_createShuffleVector), TY_IRBuilder, MN_("createShuffleVector"), 3, TY_Value, MN_("v1"),TY_Value, MN_("v2"),TY_Value, MN_("mask"),
		//_Public, _F(IRBuilder_createIsNull), TY_IRBuilder, MN_("createIsNull"), 1, TY_Value, MN_("arg"),
		//_Public, _F(IRBuilder_createIsNotNull), TY_IRBuilder, MN_("createIsNotNull"), 1, TY_Value, MN_("arg"),
		//_Public, _F(IRBuilder_createPtrDiff), TY_IRBuilder, MN_("createPtrDiff"), 2, TY_Value, MN_("lhs"),TY_Value, MN_("rhs"),
		//_Public, _F(IRBuilder_setInsertPoint), TY_IRBuilder, MN_("setInsertPoint"), 1, TY_BasicBlock, MN_("bb"),
		//_Public, _F(IRBuilder_getInsertBlock), TY_IRBuilder, MN_("getInsertBlock"), 0, 
		//_Public, _F(BasicBlock_getParent), TY_BasicBlock, MN_("getParent"), 0, 
		//_Public, _F(BasicBlock_insertBefore), TY_BasicBlock, MN_("insertBefore"), 2, TY_Instruction, MN_("before"),TY_Instruction, MN_("inst"),
		//_Public, _F(BasicBlock_getLastInst), TY_BasicBlock, MN_("getLastInst"), 0, 
		//_Public, _F(BasicBlock_getTerminator), TY_BasicBlock, MN_("getTerminator"), 0, 
		//_Public, _F(Instruction_setMetadata), TY_Instruction, MN_("setMetadata"), 3, TY_Module, MN_("m"),TY_String, MN_("name"),TY_int, MN_("value"),
		//_Public, _F(Function_dump), TY_Function, MN_("dump"), 0, 
		//_Public, _F(Value_dump), TY_Value, MN_("dump"), 0, 
		//_Public, _F(Type_dump), TY_Type, MN_("dump"), 0, 
		//_Public, _F(BasicBlock_dump), TY_BasicBlock, MN_("dump"), 0, 
		//_Public|_Static, _F(Function_create), TY_Function, MN_("create"), 4, TY_String, MN_("name"),TY_FunctionType, MN_("fnTy"),TY_Module, MN_("m"),TY_int, MN_("linkage"),
		//_Public, _F(Function_addFnAttr), TY_Function, MN_("addFnAttr"), 1, TY_Int, MN_("attributes"),
		//_Public, _F(BasicBlock_size), TY_BasicBlock, MN_("size"), 0, 
		//_Public, _F(BasicBlock_empty), TY_BasicBlock, MN_("empty"), 0, 
		//_Public, _F(Module_new), TY_Module, MN_("new"), 1, TY_String, MN_("name"),
		//_Public, _F(Module_getTypeByName), TY_Module, MN_("getTypeByName"), 1, TY_String, MN_("name"),
		//_Public, _F(Module_dump), TY_Module, MN_("dump"), 0, 
		//_Public, _F(Module_getOrInsertFunction), TY_Module, MN_("getOrInsertFunction"), 2, TY_String, MN_("name"),TY_FunctionType, MN_("fnTy"),
		//_Public, _F(Module_createExecutionEngine), TY_Module, MN_("createExecutionEngine"), 1, TY_int, MN_("optLevel"),
		//_Public|_Static, _F(BasicBlock_create), TY_BasicBlock, MN_("create"), 2, TY_Function, MN_("parent"),TY_String, MN_("name"),
		//_Public|_Static, _F(FunctionType_get), TY_FunctionType, MN_("get"), 3, TY_Type, MN_("retTy"),TY_Array<Type>, MN_("args"),TY_boolean, MN_("b"),
		//_Public|_Static, _F(ArrayType_get), TY_ArrayType, MN_("get"), 2, TY_Type, MN_("t"),TY_int, MN_("elemSize"),
		//_Public|_Static, _F(StructType_get), TY_StructType, MN_("get"), 2, TY_Array<Type>, MN_("args"),TY_boolean, MN_("isPacked"),
		//_Public|_Static, _F(StructType_create), TY_StructType, MN_("create"), 3, TY_Array<Type>, MN_("args"),TY_String, MN_("name"),TY_boolean, MN_("isPacked"),
		//_Public, _F(StructType_setBody), TY_StructType, MN_("setBody"), 2, TY_Array<Type>, MN_("args"),TY_boolean, MN_("isPacked"),
		//_Public, _F(StructType_isOpaque), TY_StructType, MN_("isOpaque"), 0, 
		//_Public, _F(ExecutionEngine_getPointerToFunction), TY_ExecutionEngine, MN_("getPointerToFunction"), 1, TY_Function, MN_("func"),
		//_Public, _F(ExecutionEngine_addGlobalMapping), TY_ExecutionEngine, MN_("addGlobalMapping"), 2, TY_GlobalVariable, MN_("g"),TY_int, MN_("addr"),
		//_Public, _F(GlobalVariable_new), TY_GlobalVariable, MN_("new"), 5, TY_Module, MN_("m"),TY_Type, MN_("ty"),TY_Constant, MN_("c"),TY_int, MN_("linkage"),TY_String, MN_("name"),
		//_Public, _F(PassManagerBuilder_new), TY_PassManagerBuilder, MN_("new"), 0, 
		//_Public, _F(PassManagerBuilder_populateModulePassManager), TY_PassManagerBuilder, MN_("populateModulePassManager"), 1, TY_PassManager, MN_("manager"),
		//_Public, _F(PassManager_new), TY_PassManager, MN_("new"), 0, 
		//_Public, _F(FunctionPassManager_new), TY_FunctionPassManager, MN_("new"), 1, TY_Module, MN_("m"),
		//_Public, _F(PassManager_addPass), TY_PassManager, MN_("addPass"), 1, TY_Pass, MN_("p"),
		//_Public, _F(PassManager_addImmutablePass), TY_PassManager, MN_("addImmutablePass"), 1, TY_ImmutablePass, MN_("p"),
		//_Public, _F(PassManager_addFunctionPass), TY_PassManager, MN_("addFunctionPass"), 1, TY_FunctionPass, MN_("p"),
		//_Public, _F(PassManager_addModulePass), TY_PassManager, MN_("addModulePass"), 1, TY_ModulePass, MN_("p"),
		//_Public, _F(FunctionPassManager_add), TY_FunctionPassManager, MN_("add"), 1, TY_Pass, MN_("p"),
		//_Public, _F(FunctionPassManager_run), TY_FunctionPassManager, MN_("run"), 1, TY_Function, MN_("func"),
		//_Public, _F(FunctionPassManager_doInitialization), TY_FunctionPassManager, MN_("doInitialization"), 0, 
		//_Public, _F(ExecutionEngine_getTargetData), TY_ExecutionEngine, MN_("getTargetData"), 0, 
		//_Public, _F(Argument_new), TY_Argument, MN_("new"), 1, TY_Type, MN_("type"),
		//_Public, _F(Value_replaceAllUsesWith), TY_Value, MN_("replaceAllUsesWith"), 1, TY_Value, MN_("v"),
		//_Public, _F(Value_setName), TY_Value, MN_("setName"), 1, TY_String, MN_("name"),
		//_Public, _F(Value_getType), TY_Value, MN_("getType"), 0, 
		//_Public, _F(Function_getArguments), TY_Function, MN_("getArguments"), 0, 
		//_Public, _F(LoadInst_setAlignment), TY_LoadInst, MN_("setAlignment"), 1, TY_Int, MN_("align"),
		//_Public, _F(StoreInst_setAlignment), TY_StoreInst, MN_("setAlignment"), 1, TY_Int, MN_("align"),
		//_Public, _F(Method_setFunction), TY_Method, MN_("setFunction"), 1, TY_NativeFunction, MN_("nf"),
		//_Public|_Static, _F(ConstantInt_get), TY_ConstantInt, MN_("get"), 2, TY_Type, MN_("type"),TY_int, MN_("v"),
		//_Public|_Static, _F(ConstantFP_get), TY_ConstantFP, MN_("get"), 2, TY_Type, MN_("type"),TY_float, MN_("v"),
		//_Public|_Static, _F(ConstantPointerNull_get), TY_ConstantPointerNull, MN_("get"), 1, TY_Type, MN_("type"),
		//_Public|_Static, _F(ConstantStruct_get), TY_ConstantStruct, MN_("get"), 2, TY_Type, MN_("type"),TY_Array<Constant>, MN_("v"),
		//_Public|_Static, _F(DynamicLibrary_loadLibraryPermanently), TY_DynamicLibrary, MN_("loadLibraryPermanently"), 1, TY_String, MN_("libname"),
		//_Public|_Static, _F(DynamicLibrary_searchForAddressOfSymbol), TY_DynamicLibrary, MN_("searchForAddressOfSymbol"), 1, TY_String, MN_("fname"),
		//_Public|_Static, _F(LLVM_createDomPrinterPass), TY_LLVM, MN_("createDomPrinterPass"), 0, 
		//_Public|_Static, _F(LLVM_createDomOnlyPrinterPass), TY_LLVM, MN_("createDomOnlyPrinterPass"), 0, 
		//_Public|_Static, _F(LLVM_createDomViewerPass), TY_LLVM, MN_("createDomViewerPass"), 0, 
		//_Public|_Static, _F(LLVM_createDomOnlyViewerPass), TY_LLVM, MN_("createDomOnlyViewerPass"), 0, 
		//_Public|_Static, _F(LLVM_createPostDomPrinterPass), TY_LLVM, MN_("createPostDomPrinterPass"), 0, 
		//_Public|_Static, _F(LLVM_createPostDomOnlyPrinterPass), TY_LLVM, MN_("createPostDomOnlyPrinterPass"), 0, 
		//_Public|_Static, _F(LLVM_createPostDomViewerPass), TY_LLVM, MN_("createPostDomViewerPass"), 0, 
		//_Public|_Static, _F(LLVM_createPostDomOnlyViewerPass), TY_LLVM, MN_("createPostDomOnlyViewerPass"), 0, 
		//_Public|_Static, _F(LLVM_createGlobalsModRefPass), TY_LLVM, MN_("createGlobalsModRefPass"), 0, 
		//_Public|_Static, _F(LLVM_createAliasDebugger), TY_LLVM, MN_("createAliasDebugger"), 0, 
		//_Public|_Static, _F(LLVM_createAliasAnalysisCounterPass), TY_LLVM, MN_("createAliasAnalysisCounterPass"), 0, 
		//_Public|_Static, _F(LLVM_createAAEvalPass), TY_LLVM, MN_("createAAEvalPass"), 0, 
		//_Public|_Static, _F(LLVM_createLibCallAliasAnalysisPass), TY_LLVM, MN_("createLibCallAliasAnalysisPass"), 1, TY_LibCallInfo, MN_("lci"),
		//_Public|_Static, _F(LLVM_createScalarEvolutionAliasAnalysisPass), TY_LLVM, MN_("createScalarEvolutionAliasAnalysisPass"), 0, 
		//_Public|_Static, _F(LLVM_createProfileLoaderPass), TY_LLVM, MN_("createProfileLoaderPass"), 0, 
		//_Public|_Static, _F(LLVM_createProfileEstimatorPass), TY_LLVM, MN_("createProfileEstimatorPass"), 0, 
		//_Public|_Static, _F(LLVM_createProfileVerifierPass), TY_LLVM, MN_("createProfileVerifierPass"), 0, 
		//_Public|_Static, _F(LLVM_createPathProfileLoaderPass), TY_LLVM, MN_("createPathProfileLoaderPass"), 0, 
		//_Public|_Static, _F(LLVM_createPathProfileVerifierPass), TY_LLVM, MN_("createPathProfileVerifierPass"), 0, 
		//_Public|_Static, _F(LLVM_createLazyValueInfoPass), TY_LLVM, MN_("createLazyValueInfoPass"), 0, 
		//_Public|_Static, _F(LLVM_createLoopDependenceAnalysisPass), TY_LLVM, MN_("createLoopDependenceAnalysisPass"), 0, 
		//_Public|_Static, _F(LLVM_createInstCountPass), TY_LLVM, MN_("createInstCountPass"), 0, 
		//_Public|_Static, _F(LLVM_createDbgInfoPrinterPass), TY_LLVM, MN_("createDbgInfoPrinterPass"), 0, 
		//_Public|_Static, _F(LLVM_createRegionInfoPass), TY_LLVM, MN_("createRegionInfoPass"), 0, 
		//_Public|_Static, _F(LLVM_createModuleDebugInfoPrinterPass), TY_LLVM, MN_("createModuleDebugInfoPrinterPass"), 0, 
		//_Public|_Static, _F(LLVM_createMemDepPrinter), TY_LLVM, MN_("createMemDepPrinter"), 0, 
		//_Public|_Static, _F(LLVM_createPostDomTree), TY_LLVM, MN_("createPostDomTree"), 0, 
		//_Public|_Static, _F(LLVM_createRegionViewerPass), TY_LLVM, MN_("createRegionViewerPass"), 0, 
		//_Public|_Static, _F(LLVM_createRegionOnlyViewerPass), TY_LLVM, MN_("createRegionOnlyViewerPass"), 0, 
		//_Public|_Static, _F(LLVM_createRegionPrinterPass), TY_LLVM, MN_("createRegionPrinterPass"), 0, 
		//_Public|_Static, _F(LLVM_createRegionOnlyPrinterPass), TY_LLVM, MN_("createRegionOnlyPrinterPass"), 0, 
		//_Public|_Static, _F(LLVM_createLintPass), TY_LLVM, MN_("createLintPass"), 0, 
		//_Public|_Static, _F(LLVM_createStripSymbolsPass), TY_LLVM, MN_("createStripSymbolsPass"), 1, TY_boolean, MN_("onlyDebugInfo"),
		//_Public|_Static, _F(LLVM_createStripNonDebugSymbolsPass), TY_LLVM, MN_("createStripNonDebugSymbolsPass"), 0, 
		//_Public|_Static, _F(LLVM_createStripDeadDebugInfoPass), TY_LLVM, MN_("createStripDeadDebugInfoPass"), 0, 
		//_Public|_Static, _F(LLVM_createConstantMergePass), TY_LLVM, MN_("createConstantMergePass"), 0, 
		//_Public|_Static, _F(LLVM_createGlobalOptimizerPass), TY_LLVM, MN_("createGlobalOptimizerPass"), 0, 
		//_Public|_Static, _F(LLVM_createGlobalDCEPass), TY_LLVM, MN_("createGlobalDCEPass"), 0, 
		//_Public|_Static, _F(LLVM_createFunctionInliningPass), TY_LLVM, MN_("createFunctionInliningPass"), 1, TY_int, MN_("threshold"),
		//_Public|_Static, _F(LLVM_createAlwaysInlinerPass), TY_LLVM, MN_("createAlwaysInlinerPass"), 0, 
		//_Public|_Static, _F(LLVM_createPruneEHPass), TY_LLVM, MN_("createPruneEHPass"), 0, 
		//_Public|_Static, _F(LLVM_createInternalizePass), TY_LLVM, MN_("createInternalizePass"), 1, TY_boolean, MN_("allButMain"),
		//_Public|_Static, _F(LLVM_createDeadArgEliminationPass), TY_LLVM, MN_("createDeadArgEliminationPass"), 0, 
		//_Public|_Static, _F(LLVM_createArgumentPromotionPass), TY_LLVM, MN_("createArgumentPromotionPass"), 1, TY_int, MN_("maxElements"),
		//_Public|_Static, _F(LLVM_createIPConstantPropagationPass), TY_LLVM, MN_("createIPConstantPropagationPass"), 0, 
		//_Public|_Static, _F(LLVM_createIPSCCPPass), TY_LLVM, MN_("createIPSCCPPass"), 0, 
		//_Public|_Static, _F(LLVM_createLoopExtractorPass), TY_LLVM, MN_("createLoopExtractorPass"), 0, 
		//_Public|_Static, _F(LLVM_createSingleLoopExtractorPass), TY_LLVM, MN_("createSingleLoopExtractorPass"), 0, 
		//_Public|_Static, _F(LLVM_createBlockExtractorPass), TY_LLVM, MN_("createBlockExtractorPass"), 0, 
		//_Public|_Static, _F(LLVM_createStripDeadPrototypesPass), TY_LLVM, MN_("createStripDeadPrototypesPass"), 0, 
		//_Public|_Static, _F(LLVM_createFunctionAttrsPass), TY_LLVM, MN_("createFunctionAttrsPass"), 0, 
		//_Public|_Static, _F(LLVM_createMergeFunctionsPass), TY_LLVM, MN_("createMergeFunctionsPass"), 0, 
		//_Public|_Static, _F(LLVM_createPartialInliningPass), TY_LLVM, MN_("createPartialInliningPass"), 0, 
		//_Public|_Static, _F(LLVM_createConstantPropagationPass), TY_LLVM, MN_("createConstantPropagationPass"), 0, 
		//_Public|_Static, _F(LLVM_createSCCPPass), TY_LLVM, MN_("createSCCPPass"), 0, 
		//_Public|_Static, _F(LLVM_createDeadInstEliminationPass), TY_LLVM, MN_("createDeadInstEliminationPass"), 0, 
		//_Public|_Static, _F(LLVM_createDeadCodeEliminationPass), TY_LLVM, MN_("createDeadCodeEliminationPass"), 0, 
		//_Public|_Static, _F(LLVM_createDeadStoreEliminationPass), TY_LLVM, MN_("createDeadStoreEliminationPass"), 0, 
		//_Public|_Static, _F(LLVM_createAggressiveDCEPass), TY_LLVM, MN_("createAggressiveDCEPass"), 0, 
		//_Public|_Static, _F(LLVM_createScalarReplAggregatesPass), TY_LLVM, MN_("createScalarReplAggregatesPass"), 1, TY_int, MN_("threshold"),
		//_Public|_Static, _F(LLVM_createIndVarSimplifyPass), TY_LLVM, MN_("createIndVarSimplifyPass"), 0, 
		//_Public|_Static, _F(LLVM_createInstructionCombiningPass), TY_LLVM, MN_("createInstructionCombiningPass"), 0, 
		//_Public|_Static, _F(LLVM_createLICMPass), TY_LLVM, MN_("createLICMPass"), 0, 
		//_Public|_Static, _F(LLVM_createLoopUnswitchPass), TY_LLVM, MN_("createLoopUnswitchPass"), 1, TY_boolean, MN_("optimizeForSize"),
		//_Public|_Static, _F(LLVM_createLoopInstSimplifyPass), TY_LLVM, MN_("createLoopInstSimplifyPass"), 0, 
		//_Public|_Static, _F(LLVM_createLoopUnrollPass), TY_LLVM, MN_("createLoopUnrollPass"), 3, TY_int, MN_("threshold"),TY_int, MN_("count"),TY_int, MN_("allowPartial"),
		//_Public|_Static, _F(LLVM_createLoopRotatePass), TY_LLVM, MN_("createLoopRotatePass"), 0, 
		//_Public|_Static, _F(LLVM_createLoopIdiomPass), TY_LLVM, MN_("createLoopIdiomPass"), 0, 
		//_Public|_Static, _F(LLVM_createPromoteMemoryToRegisterPass), TY_LLVM, MN_("createPromoteMemoryToRegisterPass"), 0, 
		//_Public|_Static, _F(LLVM_createDemoteRegisterToMemoryPass), TY_LLVM, MN_("createDemoteRegisterToMemoryPass"), 0, 
		//_Public|_Static, _F(LLVM_createReassociatePass), TY_LLVM, MN_("createReassociatePass"), 0, 
		//_Public|_Static, _F(LLVM_createJumpThreadingPass), TY_LLVM, MN_("createJumpThreadingPass"), 0, 
		//_Public|_Static, _F(LLVM_createCFGSimplificationPass), TY_LLVM, MN_("createCFGSimplificationPass"), 0, 
		//_Public|_Static, _F(LLVM_createBreakCriticalEdgesPass), TY_LLVM, MN_("createBreakCriticalEdgesPass"), 0, 
		//_Public|_Static, _F(LLVM_createLoopSimplifyPass), TY_LLVM, MN_("createLoopSimplifyPass"), 0, 
		//_Public|_Static, _F(LLVM_createTailCallEliminationPass), TY_LLVM, MN_("createTailCallEliminationPass"), 0, 
		//_Public|_Static, _F(LLVM_createLowerSwitchPass), TY_LLVM, MN_("createLowerSwitchPass"), 0, 
		//_Public|_Static, _F(LLVM_createBlockPlacementPass), TY_LLVM, MN_("createBlockPlacementPass"), 0, 
		//_Public|_Static, _F(LLVM_createLCSSAPass), TY_LLVM, MN_("createLCSSAPass"), 0, 
		//_Public|_Static, _F(LLVM_createEarlyCSEPass), TY_LLVM, MN_("createEarlyCSEPass"), 0, 
		//_Public|_Static, _F(LLVM_createGVNPass), TY_LLVM, MN_("createGVNPass"), 1, TY_boolean, MN_("noLoads"),
		//_Public|_Static, _F(LLVM_createMemCpyOptPass), TY_LLVM, MN_("createMemCpyOptPass"), 0, 
		//_Public|_Static, _F(LLVM_createLoopDeletionPass), TY_LLVM, MN_("createLoopDeletionPass"), 0, 
		//_Public|_Static, _F(LLVM_createSimplifyLibCallsPass), TY_LLVM, MN_("createSimplifyLibCallsPass"), 0, 
		//_Public|_Static, _F(LLVM_createInstructionNamerPass), TY_LLVM, MN_("createInstructionNamerPass"), 0, 
		//_Public|_Static, _F(LLVM_createSinkingPass), TY_LLVM, MN_("createSinkingPass"), 0, 
		//_Public|_Static, _F(LLVM_createLowerAtomicPass), TY_LLVM, MN_("createLowerAtomicPass"), 0, 
		//_Public|_Static, _F(LLVM_createCorrelatedValuePropagationPass), TY_LLVM, MN_("createCorrelatedValuePropagationPass"), 0, 
		//_Public|_Static, _F(LLVM_createObjCARCExpandPass), TY_LLVM, MN_("createObjCARCExpandPass"), 0, 
		//_Public|_Static, _F(LLVM_createObjCARCContractPass), TY_LLVM, MN_("createObjCARCContractPass"), 0, 
		//_Public|_Static, _F(LLVM_createObjCARCOptPass), TY_LLVM, MN_("createObjCARCOptPass"), 0, 
		//_Public|_Static, _F(LLVM_createInstructionSimplifierPass), TY_LLVM, MN_("createInstructionSimplifierPass"), 0, 
		//_Public|_Static, _F(LLVM_createLowerExpectIntrinsicPass), TY_LLVM, MN_("createLowerExpectIntrinsicPass"), 0, 
		//_Public|_Static, _F(LLVM_createUnifyFunctionExitNodesPass), TY_LLVM, MN_("createUnifyFunctionExitNodesPass"), 0, 
		//_Public|_Static, _F(LLVM_createTypeBasedAliasAnalysisPass), TY_LLVM, MN_("createTypeBasedAliasAnalysisPass"), 0, 
		//_Public|_Static, _F(LLVM_createBasicAliasAnalysisPass), TY_LLVM, MN_("createBasicAliasAnalysisPass"), 0, 
		//_Public|_Static, _F(LLVM_createVerifierPass), TY_LLVM, MN_("createVerifierPass"), 0, 
		//_Public|_Static, _F(Intrinsic_getType), TY_Intrinsic, MN_("getType"), 2, TY_int, MN_("id"),TY_ARRAY_Type, MN_("args"),
		//_Public|_Static, _F(Intrinsic_getDeclaration), TY_Intrinsic, MN_("getDeclaration"), 3, TY_Module, MN_("m"),TY_int, MN_("id"),TY_ARRAY_Type, MN_("args"),
		//_Public|_Static, _F(LLVM_parseBitcodeFile), TY_LLVM, MN_("parseBitcodeFile"), 1, TY_String, MN_("bcfile"),
		DEND,
	};
	kloadMethodData(NULL, methoddata);

	return true;
}

static kbool_t llvm_setupPackage(CTX, struct kKonohaSpace *ks, kline_t pline)
{
	return true;
}

static kbool_t llvm_initKonohaSpace(CTX,  struct kKonohaSpace *ks, kline_t pline)
{
	return true;
}

static kbool_t llvm_setupKonohaSpace(CTX, struct kKonohaSpace *ks, kline_t pline)
{
	return true;
}

KPACKDEF* llvm_init(void)
{
	InitializeNativeTarget();
	static KPACKDEF d = {
		K_CHECKSUM,
		"llvm", "3.0", "", "", "",
		llvm_initPackage,
		llvm_setupPackage,
		llvm_initKonohaSpace,
		llvm_setupKonohaSpace,
		K_REVISION
	};

	return &d;
}

#ifdef __cplusplus
}
#endif