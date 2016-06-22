; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64Matrix = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: norecurse nounwind
define private void @covariance_tmp_thunk0({ %f64Matrix*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64Matrix*, i32 }, { %f64Matrix*, i32 }* %0, i64 0, i32 0
  %4 = load %f64Matrix*, %f64Matrix** %3, align 8
  %5 = getelementptr inbounds { %f64Matrix*, i32 }, { %f64Matrix*, i32 }* %0, i64 0, i32 1
  %6 = load i32, i32* %5, align 4
  %7 = sitofp i32 %6 to double
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %8 = getelementptr %f64Matrix, %f64Matrix* %4, i64 0, i32 6, i64 %x
  store double %7, double* %8, align 8, !llvm.mem.parallel_loop_access !0
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  ret void
}

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: norecurse nounwind
define private void @covariance_tmp_thunk1({ %f64Matrix*, double }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64Matrix*, double }, { %f64Matrix*, double }* %0, i64 0, i32 0
  %4 = load %f64Matrix*, %f64Matrix** %3, align 8
  %5 = getelementptr inbounds { %f64Matrix*, double }, { %f64Matrix*, double }* %0, i64 0, i32 1
  %6 = load double, double* %5, align 8
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %7 = getelementptr %f64Matrix, %f64Matrix* %4, i64 0, i32 6, i64 %x
  %8 = load double, double* %7, align 8, !llvm.mem.parallel_loop_access !1
  %9 = fmul fast double %8, %6
  store double %9, double* %7, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  ret void
}

; Function Attrs: norecurse nounwind
define private void @covariance_tmp_thunk2({ %f64Matrix*, %f64Matrix* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64Matrix*, %f64Matrix* }, { %f64Matrix*, %f64Matrix* }* %0, i64 0, i32 0
  %4 = load %f64Matrix*, %f64Matrix** %3, align 8
  %5 = getelementptr inbounds { %f64Matrix*, %f64Matrix* }, { %f64Matrix*, %f64Matrix* }* %0, i64 0, i32 1
  %6 = load %f64Matrix*, %f64Matrix** %5, align 8
  %7 = getelementptr inbounds %f64Matrix, %f64Matrix* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !2
  %mat_y_step = zext i32 %columns to i64
  %8 = getelementptr inbounds %f64Matrix, %f64Matrix* %6, i64 0, i32 3
  %columns1 = load i32, i32* %8, align 4, !range !2
  %val_y_step = zext i32 %columns1 to i64
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %9 = mul nuw nsw i64 %y, %val_y_step
  %10 = mul nuw nsw i64 %y, %mat_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %11 = add nuw nsw i64 %x, %9
  %12 = getelementptr %f64Matrix, %f64Matrix* %6, i64 0, i32 6, i64 %11
  %13 = load double, double* %12, align 8, !llvm.mem.parallel_loop_access !3
  %14 = add nuw nsw i64 %x, %10
  %15 = getelementptr %f64Matrix, %f64Matrix* %4, i64 0, i32 6, i64 %14
  store double %13, double* %15, align 8, !llvm.mem.parallel_loop_access !3
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %mat_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

; Function Attrs: norecurse nounwind
define private void @covariance_tmp_thunk3({ %f64Matrix*, %f64Matrix* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64Matrix*, %f64Matrix* }, { %f64Matrix*, %f64Matrix* }* %0, i64 0, i32 0
  %4 = load %f64Matrix*, %f64Matrix** %3, align 8
  %5 = getelementptr inbounds { %f64Matrix*, %f64Matrix* }, { %f64Matrix*, %f64Matrix* }* %0, i64 0, i32 1
  %6 = load %f64Matrix*, %f64Matrix** %5, align 8
  %7 = getelementptr inbounds %f64Matrix, %f64Matrix* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !2
  %mat_y_step = zext i32 %columns to i64
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %8 = mul nuw nsw i64 %y, %mat_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %9 = add nuw nsw i64 %x, %8
  %10 = getelementptr %f64Matrix, %f64Matrix* %4, i64 0, i32 6, i64 %9
  %11 = load double, double* %10, align 8, !llvm.mem.parallel_loop_access !4
  %12 = getelementptr %f64Matrix, %f64Matrix* %6, i64 0, i32 6, i64 %x
  %13 = load double, double* %12, align 8, !llvm.mem.parallel_loop_access !4
  %14 = fsub fast double %11, %13
  store double %14, double* %10, align 8, !llvm.mem.parallel_loop_access !4
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %mat_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

; Function Attrs: norecurse nounwind
define private void @covariance_tmp_thunk4({ %f64Matrix*, %f64Matrix*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64Matrix*, %f64Matrix*, i32 }, { %f64Matrix*, %f64Matrix*, i32 }* %0, i64 0, i32 0
  %4 = load %f64Matrix*, %f64Matrix** %3, align 8
  %5 = getelementptr inbounds { %f64Matrix*, %f64Matrix*, i32 }, { %f64Matrix*, %f64Matrix*, i32 }* %0, i64 0, i32 1
  %6 = load %f64Matrix*, %f64Matrix** %5, align 8
  %7 = getelementptr inbounds { %f64Matrix*, %f64Matrix*, i32 }, { %f64Matrix*, %f64Matrix*, i32 }* %0, i64 0, i32 2
  %8 = load i32, i32* %7, align 4
  %9 = getelementptr inbounds %f64Matrix, %f64Matrix* %6, i64 0, i32 3
  %columns1 = load i32, i32* %9, align 4, !range !2
  %dst_y_step = zext i32 %columns1 to i64
  %10 = icmp eq i32 %8, 0
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %11 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %Flow6
  %x = phi i64 [ %x_increment, %Flow6 ], [ 0, %y_body ]
  %12 = icmp ugt i64 %y, %x
  br i1 %12, label %Flow6, label %loop.preheader

loop.preheader:                                   ; preds = %x_body
  br i1 %10, label %exit4, label %true_entry3

x_exit:                                           ; preds = %Flow6
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void

true_entry3:                                      ; preds = %loop.preheader, %true_entry3
  %13 = phi i32 [ %25, %true_entry3 ], [ 0, %loop.preheader ]
  %14 = phi double [ %24, %true_entry3 ], [ 0.000000e+00, %loop.preheader ]
  %15 = zext i32 %13 to i64
  %16 = mul nuw nsw i64 %15, %dst_y_step
  %17 = add nuw nsw i64 %16, %x
  %18 = getelementptr %f64Matrix, %f64Matrix* %6, i64 0, i32 6, i64 %17
  %19 = load double, double* %18, align 8, !llvm.mem.parallel_loop_access !5
  %20 = add nuw nsw i64 %16, %y
  %21 = getelementptr %f64Matrix, %f64Matrix* %6, i64 0, i32 6, i64 %20
  %22 = load double, double* %21, align 8, !llvm.mem.parallel_loop_access !5
  %23 = fmul fast double %22, %19
  %24 = fadd fast double %23, %14
  %25 = add nuw nsw i32 %13, 1
  %26 = icmp eq i32 %25, %8
  br i1 %26, label %exit4, label %true_entry3

Flow6:                                            ; preds = %x_body, %exit4
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

exit4:                                            ; preds = %true_entry3, %loop.preheader
  %.lcssa = phi double [ 0.000000e+00, %loop.preheader ], [ %24, %true_entry3 ]
  %27 = add nuw nsw i64 %x, %11
  %28 = getelementptr %f64Matrix, %f64Matrix* %4, i64 0, i32 6, i64 %27
  store double %.lcssa, double* %28, align 8, !llvm.mem.parallel_loop_access !5
  %29 = mul nuw nsw i64 %x, %dst_y_step
  %30 = add nuw nsw i64 %29, %y
  %31 = getelementptr %f64Matrix, %f64Matrix* %4, i64 0, i32 6, i64 %30
  store double %.lcssa, double* %31, align 8, !llvm.mem.parallel_loop_access !5
  br label %Flow6
}

; Function Attrs: nounwind
define noalias %f64Matrix* @covariance(%f64Matrix* noalias nocapture) #2 {
entry:
  %1 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !2
  %2 = call %u0Matrix* @likely_new(i32 8512, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !2
  %4 = zext i32 %columns to i64
  %5 = alloca { %f64Matrix*, i32 }, align 8
  %6 = bitcast { %f64Matrix*, i32 }* %5 to %u0Matrix**
  store %u0Matrix* %2, %u0Matrix** %6, align 8
  %7 = getelementptr inbounds { %f64Matrix*, i32 }, { %f64Matrix*, i32 }* %5, i64 0, i32 1
  store i32 0, i32* %7, align 8
  %8 = bitcast { %f64Matrix*, i32 }* %5 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64Matrix*, i32 }*, i64, i64)* @covariance_tmp_thunk0 to i8*), i8* %8, i64 %4) #2
  %9 = zext i32 %rows to i64
  %10 = getelementptr inbounds %u0Matrix, %u0Matrix* %2, i64 1
  %11 = bitcast %u0Matrix* %10 to double*
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %12 = mul nuw nsw i64 %y, %4
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %13 = getelementptr double, double* %11, i64 %x
  %14 = load double, double* %13, align 8
  %15 = add nuw nsw i64 %x, %12
  %16 = getelementptr %f64Matrix, %f64Matrix* %0, i64 0, i32 6, i64 %15
  %17 = load double, double* %16, align 8
  %18 = fadd fast double %17, %14
  store double %18, double* %13, align 8
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %4
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %9
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %19 = icmp eq i32 %rows, 1
  br i1 %19, label %exit, label %true_entry

true_entry:                                       ; preds = %y_exit
  %20 = uitofp i32 %rows to double
  %21 = fdiv fast double 1.000000e+00, %20
  %22 = alloca { %f64Matrix*, double }, align 8
  %23 = bitcast { %f64Matrix*, double }* %22 to %u0Matrix**
  store %u0Matrix* %2, %u0Matrix** %23, align 8
  %24 = getelementptr inbounds { %f64Matrix*, double }, { %f64Matrix*, double }* %22, i64 0, i32 1
  store double %21, double* %24, align 8
  %25 = bitcast { %f64Matrix*, double }* %22 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64Matrix*, double }*, i64, i64)* @covariance_tmp_thunk1 to i8*), i8* %25, i64 %4) #2
  %columns7.pre = load i32, i32* %1, align 4, !range !2
  %rows8.pre = load i32, i32* %3, align 4, !range !2
  br label %exit

exit:                                             ; preds = %y_exit, %true_entry
  %rows8 = phi i32 [ 1, %y_exit ], [ %rows8.pre, %true_entry ]
  %columns7 = phi i32 [ %columns, %y_exit ], [ %columns7.pre, %true_entry ]
  %26 = call %u0Matrix* @likely_new(i32 24896, i32 1, i32 %columns7, i32 %rows8, i32 1, i8* null)
  %27 = zext i32 %rows8 to i64
  %28 = alloca { %f64Matrix*, %f64Matrix* }, align 8
  %29 = bitcast { %f64Matrix*, %f64Matrix* }* %28 to %u0Matrix**
  store %u0Matrix* %26, %u0Matrix** %29, align 8
  %30 = getelementptr inbounds { %f64Matrix*, %f64Matrix* }, { %f64Matrix*, %f64Matrix* }* %28, i64 0, i32 1
  store %f64Matrix* %0, %f64Matrix** %30, align 8
  %31 = bitcast { %f64Matrix*, %f64Matrix* }* %28 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64Matrix*, %f64Matrix* }*, i64, i64)* @covariance_tmp_thunk2 to i8*), i8* %31, i64 %27) #2
  %32 = alloca { %f64Matrix*, %f64Matrix* }, align 8
  %33 = bitcast { %f64Matrix*, %f64Matrix* }* %32 to %u0Matrix**
  store %u0Matrix* %26, %u0Matrix** %33, align 8
  %34 = getelementptr inbounds { %f64Matrix*, %f64Matrix* }, { %f64Matrix*, %f64Matrix* }* %32, i64 0, i32 1
  %35 = bitcast %f64Matrix** %34 to %u0Matrix**
  store %u0Matrix* %2, %u0Matrix** %35, align 8
  %36 = bitcast { %f64Matrix*, %f64Matrix* }* %32 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64Matrix*, %f64Matrix* }*, i64, i64)* @covariance_tmp_thunk3 to i8*), i8* %36, i64 %27) #2
  %37 = call %u0Matrix* @likely_new(i32 24896, i32 1, i32 %columns7, i32 %columns7, i32 1, i8* null)
  %dst = bitcast %u0Matrix* %37 to %f64Matrix*
  %38 = zext i32 %columns7 to i64
  %39 = alloca { %f64Matrix*, %f64Matrix*, i32 }, align 8
  %40 = bitcast { %f64Matrix*, %f64Matrix*, i32 }* %39 to %u0Matrix**
  store %u0Matrix* %37, %u0Matrix** %40, align 8
  %41 = getelementptr inbounds { %f64Matrix*, %f64Matrix*, i32 }, { %f64Matrix*, %f64Matrix*, i32 }* %39, i64 0, i32 1
  %42 = bitcast %f64Matrix** %41 to %u0Matrix**
  store %u0Matrix* %26, %u0Matrix** %42, align 8
  %43 = getelementptr inbounds { %f64Matrix*, %f64Matrix*, i32 }, { %f64Matrix*, %f64Matrix*, i32 }* %39, i64 0, i32 2
  store i32 %rows8, i32* %43, align 8
  %44 = bitcast { %f64Matrix*, %f64Matrix*, i32 }* %39 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64Matrix*, %f64Matrix*, i32 }*, i64, i64)* @covariance_tmp_thunk4 to i8*), i8* %44, i64 %38) #2
  %45 = bitcast %u0Matrix* %2 to i8*
  call void @likely_release_mat(i8* %45) #2
  %46 = bitcast %u0Matrix* %26 to i8*
  call void @likely_release_mat(i8* %46) #2
  ret %f64Matrix* %dst
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { argmemonly nounwind }
attributes #1 = { norecurse nounwind }
attributes #2 = { nounwind }

!0 = distinct !{!0}
!1 = distinct !{!1}
!2 = !{i32 1, i32 -1}
!3 = distinct !{!3}
!4 = distinct !{!4}
!5 = distinct !{!5}
