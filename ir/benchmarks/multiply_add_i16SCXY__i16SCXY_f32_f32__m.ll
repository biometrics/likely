; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%i16SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: nounwind argmemonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @multiply_add_tmp_thunk0({ %i16SCXY*, %i16SCXY*, float, float }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, float, float }, { %i16SCXY*, %i16SCXY*, float, float }* %0, i64 0, i32 0
  %4 = load %i16SCXY*, %i16SCXY** %3, align 8
  %5 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, float, float }, { %i16SCXY*, %i16SCXY*, float, float }* %0, i64 0, i32 1
  %6 = load %i16SCXY*, %i16SCXY** %5, align 8
  %7 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, float, float }, { %i16SCXY*, %i16SCXY*, float, float }* %0, i64 0, i32 2
  %8 = load float, float* %7, align 4
  %9 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, float, float }, { %i16SCXY*, %i16SCXY*, float, float }* %0, i64 0, i32 3
  %10 = load float, float* %9, align 4
  %11 = getelementptr inbounds %i16SCXY, %i16SCXY* %6, i64 0, i32 2
  %channels1 = load i32, i32* %11, align 4, !range !0
  %dst_c = zext i32 %channels1 to i64
  %12 = getelementptr inbounds %i16SCXY, %i16SCXY* %6, i64 0, i32 3
  %columns2 = load i32, i32* %12, align 4, !range !0
  %dst_x = zext i32 %columns2 to i64
  %13 = getelementptr inbounds %i16SCXY, %i16SCXY* %4, i64 0, i32 6, i64 0
  %14 = ptrtoint i16* %13 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  call void @llvm.assume(i1 %16)
  %17 = getelementptr inbounds %i16SCXY, %i16SCXY* %6, i64 0, i32 6, i64 0
  %18 = ptrtoint i16* %17 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  %21 = mul i64 %dst_c, %2
  %22 = mul i64 %21, %dst_x
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %y_body ]
  %23 = getelementptr %i16SCXY, %i16SCXY* %6, i64 0, i32 6, i64 %y
  %24 = load i16, i16* %23, align 2, !llvm.mem.parallel_loop_access !1
  %25 = sitofp i16 %24 to float
  %26 = fmul fast float %25, %8
  %val = fadd fast float %26, %10
  %27 = getelementptr %i16SCXY, %i16SCXY* %4, i64 0, i32 6, i64 %y
  %28 = fcmp fast olt float %val, 0.000000e+00
  %. = select i1 %28, float -5.000000e-01, float 5.000000e-01
  %29 = fadd fast float %., %val
  %30 = fptosi float %29 to i16
  %31 = fcmp fast olt float %29, -3.276800e+04
  %32 = select i1 %31, i16 -32768, i16 %30
  %33 = fcmp fast ogt float %29, 3.276700e+04
  %34 = select i1 %33, i16 32767, i16 %32
  store i16 %34, i16* %27, align 2, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %22
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %i16SCXY* @multiply_add(%i16SCXY*, float, float) {
entry:
  %3 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 2
  %channels = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 3
  %columns = load i32, i32* %4, align 4, !range !0
  %5 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = call %u0CXYT* @likely_new(i32 30224, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %dst = bitcast %u0CXYT* %6 to %i16SCXY*
  %7 = zext i32 %rows to i64
  %8 = alloca { %i16SCXY*, %i16SCXY*, float, float }, align 8
  %9 = bitcast { %i16SCXY*, %i16SCXY*, float, float }* %8 to %u0CXYT**
  store %u0CXYT* %6, %u0CXYT** %9, align 8
  %10 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, float, float }, { %i16SCXY*, %i16SCXY*, float, float }* %8, i64 0, i32 1
  store %i16SCXY* %0, %i16SCXY** %10, align 8
  %11 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, float, float }, { %i16SCXY*, %i16SCXY*, float, float }* %8, i64 0, i32 2
  store float %1, float* %11, align 8
  %12 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, float, float }, { %i16SCXY*, %i16SCXY*, float, float }* %8, i64 0, i32 3
  store float %2, float* %12, align 4
  %13 = bitcast { %i16SCXY*, %i16SCXY*, float, float }* %8 to i8*
  call void @likely_fork(i8* bitcast (void ({ %i16SCXY*, %i16SCXY*, float, float }*, i64, i64)* @multiply_add_tmp_thunk0 to i8*), i8* %13, i64 %7)
  ret %i16SCXY* %dst
}

attributes #0 = { nounwind argmemonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
