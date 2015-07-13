; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind argmemonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f64XY* @transpose(%f64XY*) {
entry:
  %1 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 4
  %rows = load i32, i32* %2, align 4, !range !0
  %3 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %4 = zext i32 %rows to i64
  %dst_y_step = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0CXYT, %u0CXYT* %3, i64 1
  %6 = bitcast %u0CXYT* %5 to double*
  %7 = ptrtoint %u0CXYT* %5 to i64
  %8 = and i64 %7, 31
  %9 = icmp eq i64 %8, 0
  call void @llvm.assume(i1 %9)
  %10 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 6, i64 0
  %11 = ptrtoint double* %10 to i64
  %12 = and i64 %11, 31
  %13 = icmp eq i64 %12, 0
  call void @llvm.assume(i1 %13)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %14 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %15 = mul nuw nsw i64 %x, %dst_y_step
  %16 = add nuw nsw i64 %15, %y
  %17 = getelementptr %f64XY, %f64XY* %0, i64 0, i32 6, i64 %16
  %18 = load double, double* %17, align 8, !llvm.mem.parallel_loop_access !1
  %19 = add nuw nsw i64 %x, %14
  %20 = getelementptr double, double* %6, i64 %19
  store double %18, double* %20, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %4
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %dst = bitcast %u0CXYT* %3 to %f64XY*
  ret %f64XY* %dst
}

attributes #0 = { nounwind argmemonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
