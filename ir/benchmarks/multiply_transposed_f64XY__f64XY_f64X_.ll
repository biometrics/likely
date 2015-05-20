; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%f64X = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f64XY* @multiply_transposed(%f64XY*, %f64X*) {
entry:
  %2 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %5 = zext i32 %rows to i64
  %centered_y_step = zext i32 %columns to i64
  %6 = getelementptr inbounds %u0CXYT, %u0CXYT* %4, i64 1
  %7 = bitcast %u0CXYT* %6 to double*
  %8 = ptrtoint %u0CXYT* %6 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  %11 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 6, i64 0
  %12 = ptrtoint double* %11 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  call void @llvm.assume(i1 %14)
  %15 = getelementptr inbounds %f64X, %f64X* %1, i64 0, i32 6, i64 0
  %16 = ptrtoint double* %15 to i64
  %17 = and i64 %16, 31
  %18 = icmp eq i64 %17, 0
  call void @llvm.assume(i1 %18)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %19 = mul nuw nsw i64 %y, %centered_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %20 = add nuw nsw i64 %x, %19
  %21 = getelementptr %f64XY, %f64XY* %0, i64 0, i32 6, i64 %20
  %22 = load double, double* %21, align 8, !llvm.mem.parallel_loop_access !1
  %23 = getelementptr %f64X, %f64X* %1, i64 0, i32 6, i64 %x
  %24 = load double, double* %23, align 8, !llvm.mem.parallel_loop_access !1
  %25 = fsub fast double %22, %24
  %26 = getelementptr double, double* %7, i64 %20
  store double %25, double* %26, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %centered_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %5
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %27 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %28 = getelementptr inbounds %u0CXYT, %u0CXYT* %27, i64 1
  %29 = bitcast %u0CXYT* %28 to double*
  %30 = ptrtoint %u0CXYT* %28 to i64
  %31 = and i64 %30, 31
  %32 = icmp eq i64 %31, 0
  call void @llvm.assume(i1 %32)
  br label %y_body18

y_body18:                                         ; preds = %x_exit22, %y_exit
  %y20 = phi i64 [ 0, %y_exit ], [ %y_increment28, %x_exit22 ]
  %33 = mul nuw nsw i64 %y20, %centered_y_step
  br label %x_body21

x_body21:                                         ; preds = %y_body18, %Flow
  %x23 = phi i64 [ %x_increment26, %Flow ], [ 0, %y_body18 ]
  %34 = icmp ugt i64 %y20, %x23
  br i1 %34, label %Flow, label %true_entry24

x_exit22:                                         ; preds = %Flow
  %y_increment28 = add nuw nsw i64 %y20, 1
  %y_postcondition29 = icmp eq i64 %y_increment28, %centered_y_step
  br i1 %y_postcondition29, label %y_exit19, label %y_body18

y_exit19:                                         ; preds = %x_exit22
  %dst = bitcast %u0CXYT* %27 to %f64XY*
  %35 = bitcast %u0CXYT* %4 to i8*
  call void @likely_release_mat(i8* %35)
  ret %f64XY* %dst

true_entry24:                                     ; preds = %x_body21, %true_entry24
  %36 = phi i32 [ %48, %true_entry24 ], [ 0, %x_body21 ]
  %37 = phi double [ %47, %true_entry24 ], [ 0.000000e+00, %x_body21 ]
  %38 = sext i32 %36 to i64
  %39 = mul nuw nsw i64 %38, %centered_y_step
  %40 = add nuw nsw i64 %39, %x23
  %41 = getelementptr double, double* %7, i64 %40
  %42 = load double, double* %41, align 8, !llvm.mem.parallel_loop_access !2
  %43 = add nuw nsw i64 %39, %y20
  %44 = getelementptr double, double* %7, i64 %43
  %45 = load double, double* %44, align 8, !llvm.mem.parallel_loop_access !2
  %46 = fmul fast double %45, %42
  %47 = fadd fast double %46, %37
  %48 = add nuw nsw i32 %36, 1
  %49 = icmp eq i32 %48, %rows
  br i1 %49, label %exit25, label %true_entry24

Flow:                                             ; preds = %x_body21, %exit25
  %x_increment26 = add nuw nsw i64 %x23, 1
  %x_postcondition27 = icmp eq i64 %x_increment26, %centered_y_step
  br i1 %x_postcondition27, label %x_exit22, label %x_body21

exit25:                                           ; preds = %true_entry24
  %50 = add nuw nsw i64 %x23, %33
  %51 = getelementptr double, double* %29, i64 %50
  store double %47, double* %51, align 8, !llvm.mem.parallel_loop_access !2
  %52 = mul nuw nsw i64 %x23, %centered_y_step
  %53 = add nuw nsw i64 %52, %y20
  %54 = getelementptr double, double* %29, i64 %53
  store double %47, double* %54, align 8, !llvm.mem.parallel_loop_access !2
  br label %Flow
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
