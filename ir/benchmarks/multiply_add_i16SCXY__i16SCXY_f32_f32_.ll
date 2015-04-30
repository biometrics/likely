; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%i16SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %i16SCXY* @multiply_add(%i16SCXY*, float, float) {
entry:
  %3 = getelementptr %i16SCXY, %i16SCXY* %0, i64 0, i32 2
  %4 = bitcast i32* %3 to i64*
  %channels.combined = load i64, i64* %4, align 4
  %combine.extract.trunc = trunc i64 %channels.combined to i32
  %combine.extract.shift = lshr i64 %channels.combined, 32
  %combine.extract.trunc2 = trunc i64 %combine.extract.shift to i32
  %5 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = call %u0CXYT* @likely_new(i32 30224, i32 %combine.extract.trunc, i32 %combine.extract.trunc2, i32 %rows, i32 1, i8* null)
  %7 = zext i32 %rows to i64
  %dst_c = and i64 %channels.combined, 4294967295
  %8 = getelementptr inbounds %u0CXYT, %u0CXYT* %6, i64 1
  %9 = bitcast %u0CXYT* %8 to i16*
  %10 = ptrtoint %u0CXYT* %8 to i64
  %11 = and i64 %10, 31
  %12 = icmp eq i64 %11, 0
  call void @llvm.assume(i1 %12)
  %13 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 6, i64 0
  %14 = ptrtoint i16* %13 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  call void @llvm.assume(i1 %16)
  %17 = mul nuw nsw i64 %combine.extract.shift, %dst_c
  %18 = mul nuw nsw i64 %7, %17
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %19 = getelementptr %i16SCXY, %i16SCXY* %0, i64 0, i32 6, i64 %y
  %20 = load i16, i16* %19, align 2, !llvm.mem.parallel_loop_access !1
  %21 = sitofp i16 %20 to float
  %22 = fmul float %21, %1
  %23 = fadd float %22, %2
  %24 = getelementptr i16, i16* %9, i64 %y
  %25 = fcmp olt float %23, 0.000000e+00
  %26 = select i1 %25, float -5.000000e-01, float 5.000000e-01
  %27 = fadd float %23, %26
  %28 = fptosi float %27 to i16
  %29 = fcmp olt float %27, -3.276800e+04
  %30 = select i1 %29, i16 -32768, i16 %28
  %31 = fcmp ogt float %27, 3.276700e+04
  %32 = select i1 %31, i16 32767, i16 %30
  store i16 %32, i16* %24, align 2, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %18
  br i1 %y_postcondition, label %y_exit, label %y_body, !llvm.loop !1

y_exit:                                           ; preds = %y_body
  %33 = bitcast %u0CXYT* %6 to %i16SCXY*
  ret %i16SCXY* %33
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
