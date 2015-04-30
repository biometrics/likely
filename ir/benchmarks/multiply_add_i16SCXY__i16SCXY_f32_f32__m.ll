; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%i16SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: nounwind readonly
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
  %11 = getelementptr %i16SCXY, %i16SCXY* %4, i64 0, i32 2
  %12 = bitcast i32* %11 to i64*
  %channels.combined = load i64, i64* %12, align 4
  %dst_c = and i64 %channels.combined, 4294967295
  %combine.extract.shift9 = lshr i64 %channels.combined, 32
  %13 = getelementptr inbounds %i16SCXY, %i16SCXY* %4, i64 0, i32 6, i64 0
  %14 = ptrtoint i16* %13 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  call void @llvm.assume(i1 %16)
  %17 = getelementptr %i16SCXY, %i16SCXY* %6, i64 0, i32 2
  %18 = bitcast i32* %17 to i64*
  %channels1.combined = load i64, i64* %18, align 4
  %src_c = and i64 %channels1.combined, 4294967295
  %combine.extract.shift = lshr i64 %channels1.combined, 32
  %19 = getelementptr inbounds %i16SCXY, %i16SCXY* %6, i64 0, i32 6, i64 0
  %20 = ptrtoint i16* %19 to i64
  %21 = and i64 %20, 31
  %22 = icmp eq i64 %21, 0
  call void @llvm.assume(i1 %22)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %23 = mul i64 %y, %combine.extract.shift
  %24 = mul i64 %y, %combine.extract.shift9
  br label %x_body

x_body:                                           ; preds = %c_exit, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %c_exit ]
  %tmp = add i64 %x, %23
  %tmp4 = mul i64 %tmp, %src_c
  %tmp5 = add i64 %x, %24
  %tmp6 = mul i64 %tmp5, %dst_c
  br label %c_body

c_body:                                           ; preds = %c_body, %x_body
  %c = phi i64 [ 0, %x_body ], [ %c_increment, %c_body ]
  %25 = add i64 %tmp4, %c
  %26 = getelementptr %i16SCXY, %i16SCXY* %6, i64 0, i32 6, i64 %25
  %27 = load i16, i16* %26, align 2, !llvm.mem.parallel_loop_access !0
  %28 = sitofp i16 %27 to float
  %29 = fmul float %8, %28
  %30 = fadd float %10, %29
  %31 = fcmp olt float %30, 0.000000e+00
  %32 = select i1 %31, float -5.000000e-01, float 5.000000e-01
  %33 = fadd float %30, %32
  %34 = fptosi float %33 to i16
  %35 = fcmp olt float %33, -3.276800e+04
  %36 = select i1 %35, i16 -32768, i16 %34
  %37 = fcmp ogt float %33, 3.276700e+04
  %38 = select i1 %37, i16 32767, i16 %36
  %39 = add i64 %tmp6, %c
  %40 = getelementptr %i16SCXY, %i16SCXY* %4, i64 0, i32 6, i64 %39
  store i16 %38, i16* %40, align 2, !llvm.mem.parallel_loop_access !0
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %dst_c
  br i1 %c_postcondition, label %c_exit, label %c_body, !llvm.loop !0

c_exit:                                           ; preds = %c_body
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %combine.extract.shift9
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %c_exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %i16SCXY* @multiply_add(%i16SCXY*, float, float) {
entry:
  %3 = getelementptr %i16SCXY, %i16SCXY* %0, i64 0, i32 2
  %4 = bitcast i32* %3 to i64*
  %channels.combined = load i64, i64* %4, align 4
  %combine.extract.trunc = trunc i64 %channels.combined to i32
  %combine.extract.shift = lshr i64 %channels.combined, 32
  %combine.extract.trunc1 = trunc i64 %combine.extract.shift to i32
  %5 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !1
  %6 = call %u0CXYT* @likely_new(i32 30224, i32 %combine.extract.trunc, i32 %combine.extract.trunc1, i32 %rows, i32 1, i8* null)
  %7 = bitcast %u0CXYT* %6 to %i16SCXY*
  %8 = zext i32 %rows to i64
  %9 = alloca { %i16SCXY*, %i16SCXY*, float, float }, align 8
  %10 = bitcast { %i16SCXY*, %i16SCXY*, float, float }* %9 to %u0CXYT**
  store %u0CXYT* %6, %u0CXYT** %10, align 8
  %11 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, float, float }, { %i16SCXY*, %i16SCXY*, float, float }* %9, i64 0, i32 1
  store %i16SCXY* %0, %i16SCXY** %11, align 8
  %12 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, float, float }, { %i16SCXY*, %i16SCXY*, float, float }* %9, i64 0, i32 2
  store float %1, float* %12, align 8
  %13 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, float, float }, { %i16SCXY*, %i16SCXY*, float, float }* %9, i64 0, i32 3
  store float %2, float* %13, align 4
  %14 = bitcast { %i16SCXY*, %i16SCXY*, float, float }* %9 to i8*
  call void @likely_fork(i8* bitcast (void ({ %i16SCXY*, %i16SCXY*, float, float }*, i64, i64)* @multiply_add_tmp_thunk0 to i8*), i8* %14, i64 %8)
  ret %i16SCXY* %7
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = distinct !{!0}
!1 = !{i32 1, i32 -1}
