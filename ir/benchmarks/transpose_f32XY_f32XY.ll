; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

define noalias %f32XY* @transpose(%f32XY* nocapture readonly) {
entry:
  %1 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 4
  %rows = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %rows, i32 %columns, i32 1, i8* null)
  %4 = zext i32 %columns to i64
  %dst_y_step = zext i32 %rows to i64
  %5 = getelementptr inbounds %u0CXYT, %u0CXYT* %3, i64 1
  %6 = bitcast %u0CXYT* %5 to float*
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %7 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %8 = mul nuw nsw i64 %x, %4
  %9 = add nuw nsw i64 %8, %y
  %10 = getelementptr %f32XY, %f32XY* %0, i64 0, i32 6, i64 %9
  %11 = load float, float* %10, align 4, !llvm.mem.parallel_loop_access !1
  %12 = add nuw nsw i64 %x, %7
  %13 = getelementptr float, float* %6, i64 %12
  store float %11, float* %13, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %4
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %dst = bitcast %u0CXYT* %3 to %f32XY*
  ret %f32XY* %dst
}

attributes #0 = { argmemonly nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
