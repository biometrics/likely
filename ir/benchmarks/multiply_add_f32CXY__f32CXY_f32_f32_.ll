; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32CXY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

; Function Attrs: nounwind
define %f32CXY* @multiply_add(%f32CXY*, float, float) #1 {
entry:
  %3 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 2
  %channels = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 3
  %columns = load i32, i32* %4, align 4, !range !0
  %5 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = tail call %u0CXYT* @likely_new(i32 28960, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %7 = zext i32 %rows to i64
  %dst_c = zext i32 %channels to i64
  %dst_x = zext i32 %columns to i64
  %8 = getelementptr inbounds %u0CXYT, %u0CXYT* %6, i64 1
  %9 = bitcast %u0CXYT* %8 to float*
  %10 = ptrtoint %u0CXYT* %8 to i64
  %11 = and i64 %10, 31
  %12 = icmp eq i64 %11, 0
  tail call void @llvm.assume(i1 %12)
  %13 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 0
  %14 = ptrtoint float* %13 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  tail call void @llvm.assume(i1 %16)
  %17 = mul nuw i64 %dst_x, %dst_c
  br label %x_body

x_body:                                           ; preds = %entry, %x_exit
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %18 = mul i64 %y, %dst_x
  %tmp1 = mul i64 %18, %dst_c
  br label %c_body

c_body:                                           ; preds = %c_body, %x_body
  %c = phi i64 [ 0, %x_body ], [ %c_increment, %c_body ]
  %19 = add i64 %tmp1, %c
  %20 = getelementptr %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 %19
  %21 = load float, float* %20, align 4, !llvm.mem.parallel_loop_access !1
  %22 = fmul float %21, %1
  %23 = fadd float %22, %2
  %24 = getelementptr float, float* %9, i64 %19
  store float %23, float* %24, align 4, !llvm.mem.parallel_loop_access !1
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %17
  br i1 %c_postcondition, label %x_exit, label %c_body, !llvm.loop !1

x_exit:                                           ; preds = %c_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %7
  br i1 %y_postcondition, label %y_exit, label %x_body

y_exit:                                           ; preds = %x_exit
  %25 = bitcast %u0CXYT* %6 to %f32CXY*
  ret %f32CXY* %25
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
