; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

define %f64XY* @matrix_multiplication(%f64XY*, %f64XY*) {
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
  %11 = ptrtoint %u0CXYT* %9 to i64
  %12 = and i64 %11, 31
  %13 = icmp eq i64 %12, 0
  call void @llvm.assume(i1 %13)
  %A_y_step = zext i32 %columns to i64
  %14 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 6, i64 0
  %15 = ptrtoint double* %14 to i64
  %16 = and i64 %15, 31
  %17 = icmp eq i64 %16, 0
  call void @llvm.assume(i1 %17)
  %18 = getelementptr inbounds %f64XY, %f64XY* %1, i64 0, i32 6, i64 0
  %19 = ptrtoint double* %18 to i64
  %20 = and i64 %19, 31
  %21 = icmp eq i64 %20, 0
  call void @llvm.assume(i1 %21)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %22 = mul nuw nsw i64 %y, %C_y_step
  %23 = mul nuw nsw i64 %y, %A_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %exit
  %x = phi i64 [ %x_increment, %exit ], [ 0, %y_body ]
  br label %true_entry

true_entry:                                       ; preds = %x_body, %true_entry
  %24 = phi i32 [ %36, %true_entry ], [ 0, %x_body ]
  %25 = phi double [ %35, %true_entry ], [ 0.000000e+00, %x_body ]
  %26 = sext i32 %24 to i64
  %27 = add nuw nsw i64 %26, %23
  %28 = getelementptr %f64XY, %f64XY* %0, i64 0, i32 6, i64 %27
  %29 = load double, double* %28, align 8, !llvm.mem.parallel_loop_access !1
  %30 = mul nuw nsw i64 %26, %C_y_step
  %31 = add nuw nsw i64 %30, %x
  %32 = getelementptr %f64XY, %f64XY* %1, i64 0, i32 6, i64 %31
  %33 = load double, double* %32, align 8, !llvm.mem.parallel_loop_access !1
  %34 = fmul fast double %33, %29
  %35 = fadd fast double %34, %25
  %36 = add nuw nsw i32 %24, 1
  %37 = icmp eq i32 %36, %columns
  br i1 %37, label %exit, label %true_entry

exit:                                             ; preds = %true_entry
  %38 = add nuw nsw i64 %x, %22
  %39 = getelementptr double, double* %10, i64 %38
  store double %35, double* %39, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %C_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %8
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %40 = bitcast %u0CXYT* %7 to %f64XY*
  ret %f64XY* %40
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
