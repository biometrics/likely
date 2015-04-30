; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%i32CXY = type { i32, i32, i32, i32, i32, i32, [0 x i32] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %i32CXY* @multiply_add(%i32CXY*, float, float) {
entry:
  %3 = getelementptr %i32CXY, %i32CXY* %0, i64 0, i32 2
  %4 = bitcast i32* %3 to i64*
  %channels.combined = load i64, i64* %4, align 4
  %combine.extract.trunc = trunc i64 %channels.combined to i32
  %combine.extract.shift = lshr i64 %channels.combined, 32
  %combine.extract.trunc2 = trunc i64 %combine.extract.shift to i32
  %5 = getelementptr inbounds %i32CXY, %i32CXY* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = call %u0CXYT* @likely_new(i32 29216, i32 %combine.extract.trunc, i32 %combine.extract.trunc2, i32 %rows, i32 1, i8* null)
  %7 = zext i32 %rows to i64
  %dst_c = and i64 %channels.combined, 4294967295
  %8 = getelementptr inbounds %u0CXYT, %u0CXYT* %6, i64 1, i32 0
  %9 = ptrtoint i32* %8 to i64
  %10 = and i64 %9, 31
  %11 = icmp eq i64 %10, 0
  call void @llvm.assume(i1 %11)
  %12 = getelementptr inbounds %i32CXY, %i32CXY* %0, i64 0, i32 6, i64 0
  %13 = ptrtoint i32* %12 to i64
  %14 = and i64 %13, 31
  %15 = icmp eq i64 %14, 0
  call void @llvm.assume(i1 %15)
  %16 = mul nuw nsw i64 %combine.extract.shift, %dst_c
  %17 = mul nuw nsw i64 %7, %16
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %18 = getelementptr %i32CXY, %i32CXY* %0, i64 0, i32 6, i64 %y
  %19 = load i32, i32* %18, align 4, !llvm.mem.parallel_loop_access !1
  %20 = sitofp i32 %19 to float
  %21 = fmul float %20, %1
  %22 = fadd float %21, %2
  %23 = fcmp olt float %22, 0.000000e+00
  %24 = select i1 %23, float -5.000000e-01, float 5.000000e-01
  %25 = fadd float %22, %24
  %26 = fptosi float %25 to i32
  %27 = getelementptr i32, i32* %8, i64 %y
  store i32 %26, i32* %27, align 4, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %17
  br i1 %y_postcondition, label %y_exit, label %y_body, !llvm.loop !1

y_exit:                                           ; preds = %y_body
  %28 = bitcast %u0CXYT* %6 to %i32CXY*
  ret %i32CXY* %28
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
