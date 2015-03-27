; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%f32CXY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: nounwind
define %f32XY* @convert_grayscale(%f32CXY*) #0 {
entry:
  %1 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 4
  %rows = load i32, i32* %2, align 4, !range !0
  %3 = tail call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %4 = zext i32 %rows to i64
  %dst_y_step = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0CXYT, %u0CXYT* %3, i64 1
  %6 = bitcast %u0CXYT* %5 to float*
  %7 = ptrtoint %u0CXYT* %5 to i64
  %8 = and i64 %7, 31
  %9 = icmp eq i64 %8, 0
  tail call void @llvm.assume(i1 %9)
  %10 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 2
  %channels = load i32, i32* %10, align 4, !range !0
  %src_c = zext i32 %channels to i64
  %11 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 0
  %12 = ptrtoint float* %11 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  tail call void @llvm.assume(i1 %14)
  %15 = mul nuw nsw i64 %4, %dst_y_step
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %16 = mul nuw nsw i64 %y, %src_c
  %17 = getelementptr %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 %16
  %18 = load float, float* %17, align 4, !llvm.mem.parallel_loop_access !1
  %19 = add nuw nsw i64 %16, 1
  %20 = getelementptr %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 %19
  %21 = load float, float* %20, align 4, !llvm.mem.parallel_loop_access !1
  %22 = add nuw nsw i64 %16, 2
  %23 = getelementptr %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 %22
  %24 = load float, float* %23, align 4, !llvm.mem.parallel_loop_access !1
  %25 = fmul float %18, 0x3FBD2F1AA0000000
  %26 = fmul float %21, 0x3FE2C8B440000000
  %27 = fadd float %25, %26
  %28 = fmul float %24, 0x3FD322D0E0000000
  %29 = fadd float %27, %28
  %30 = getelementptr float, float* %6, i64 %y
  store float %29, float* %30, align 4, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %15
  br i1 %y_postcondition, label %y_exit, label %y_body, !llvm.loop !1

y_exit:                                           ; preds = %y_body
  %31 = bitcast %u0CXYT* %3 to %f32XY*
  ret %f32XY* %31
}

attributes #0 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
