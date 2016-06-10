; ModuleID = 'likely'
source_filename = "likely"

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

define noalias %f64XY* @matrix_multiplication(%f64XY* nocapture readonly, %f64XY* nocapture readonly) {
entry:
  %2 = getelementptr inbounds %f64XY, %f64XY* %1, i64 0, i32 4
  %rows = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 3
  %columns = load i32, i32* %3, align 4, !range !0
  %4 = icmp eq i32 %rows, %columns
  call void @llvm.assume(i1 %4)
  %5 = getelementptr inbounds %f64XY, %f64XY* %1, i64 0, i32 3
  %columns1 = load i32, i32* %5, align 4, !range !0
  %6 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 4
  %rows2 = load i32, i32* %6, align 4, !range !0
  %7 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns1, i32 %rows2, i32 1, i8* null)
  %8 = zext i32 %rows2 to i64
  %C_y_step = zext i32 %columns1 to i64
  %9 = getelementptr inbounds %u0CXYT, %u0CXYT* %7, i64 1
  %10 = bitcast %u0CXYT* %9 to double*
  %A_y_step = zext i32 %columns to i64
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %11 = mul nuw nsw i64 %y, %C_y_step
  %12 = mul nuw nsw i64 %y, %A_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %exit
  %x = phi i64 [ %x_increment, %exit ], [ 0, %y_body ]
  br label %true_entry

true_entry:                                       ; preds = %x_body, %true_entry
  %13 = phi i32 [ %25, %true_entry ], [ 0, %x_body ]
  %14 = phi double [ %24, %true_entry ], [ 0.000000e+00, %x_body ]
  %15 = sext i32 %13 to i64
  %16 = add nuw nsw i64 %15, %12
  %17 = getelementptr %f64XY, %f64XY* %0, i64 0, i32 6, i64 %16
  %18 = load double, double* %17, align 8, !llvm.mem.parallel_loop_access !1
  %19 = mul nuw nsw i64 %15, %C_y_step
  %20 = add nuw nsw i64 %19, %x
  %21 = getelementptr %f64XY, %f64XY* %1, i64 0, i32 6, i64 %20
  %22 = load double, double* %21, align 8, !llvm.mem.parallel_loop_access !1
  %23 = fmul fast double %22, %18
  %24 = fadd fast double %23, %14
  %25 = add nuw nsw i32 %13, 1
  %26 = icmp eq i32 %25, %rows
  br i1 %26, label %exit, label %true_entry

exit:                                             ; preds = %true_entry
  %27 = add nuw nsw i64 %x, %11
  %28 = getelementptr double, double* %10, i64 %27
  store double %24, double* %28, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %C_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %8
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %C = bitcast %u0CXYT* %7 to %f64XY*
  ret %f64XY* %C
}

attributes #0 = { nounwind }
attributes #1 = { argmemonly nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
