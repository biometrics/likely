; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32CXY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f32CXY* @multiply_add(%f32CXY*, float, float) {
entry:
  %3 = getelementptr %f32CXY, %f32CXY* %0, i64 0, i32 2
  %4 = bitcast i32* %3 to i64*
  %channels.combined = load i64, i64* %4, align 4
  %combine.extract.trunc = trunc i64 %channels.combined to i32
  %combine.extract.shift = lshr i64 %channels.combined, 32
  %combine.extract.trunc2 = trunc i64 %combine.extract.shift to i32
  %5 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = call %u0CXYT* @likely_new(i32 28960, i32 %combine.extract.trunc, i32 %combine.extract.trunc2, i32 %rows, i32 1, i8* null)
  %7 = zext i32 %rows to i64
  %dst_c = and i64 %channels.combined, 4294967295
  %8 = getelementptr inbounds %u0CXYT, %u0CXYT* %6, i64 1
  %9 = bitcast %u0CXYT* %8 to float*
  %10 = ptrtoint %u0CXYT* %8 to i64
  %11 = and i64 %10, 31
  %12 = icmp eq i64 %11, 0
  call void @llvm.assume(i1 %12)
  %13 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 0
  %14 = ptrtoint float* %13 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  call void @llvm.assume(i1 %16)
  %17 = mul nuw nsw i64 %combine.extract.shift, %dst_c
  %18 = mul nuw nsw i64 %7, %17
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %19 = getelementptr %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 %y
  %20 = load float, float* %19, align 4, !llvm.mem.parallel_loop_access !1
  %21 = fmul float %20, %1
  %22 = fadd float %21, %2
  %23 = getelementptr float, float* %9, i64 %y
  store float %22, float* %23, align 4, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %18
  br i1 %y_postcondition, label %y_exit, label %y_body, !llvm.loop !1

y_exit:                                           ; preds = %y_body
  %24 = bitcast %u0CXYT* %6 to %f32CXY*
  ret %f32CXY* %24
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
