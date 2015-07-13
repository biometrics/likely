; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64X = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind argmemonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @covariance_tmp_thunk0({ %f64X*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64X*, i32 }, { %f64X*, i32 }* %0, i64 0, i32 0
  %4 = load %f64X*, %f64X** %3, align 8
  %5 = getelementptr inbounds { %f64X*, i32 }, { %f64X*, i32 }* %0, i64 0, i32 1
  %6 = load i32, i32* %5, align 4
  %7 = getelementptr inbounds %f64X, %f64X* %4, i64 0, i32 6, i64 0
  %8 = ptrtoint double* %7 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  %11 = sitofp i32 %6 to double
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %12 = getelementptr %f64X, %f64X* %4, i64 0, i32 6, i64 %x
  store double %11, double* %12, align 8, !llvm.mem.parallel_loop_access !0
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: nounwind
define private void @covariance_tmp_thunk1({ %f64X*, double }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64X*, double }, { %f64X*, double }* %0, i64 0, i32 0
  %4 = load %f64X*, %f64X** %3, align 8
  %5 = getelementptr inbounds { %f64X*, double }, { %f64X*, double }* %0, i64 0, i32 1
  %6 = load double, double* %5, align 8
  %7 = getelementptr inbounds %f64X, %f64X* %4, i64 0, i32 6, i64 0
  %8 = ptrtoint double* %7 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %11 = getelementptr %f64X, %f64X* %4, i64 0, i32 6, i64 %x
  %12 = load double, double* %11, align 8, !llvm.mem.parallel_loop_access !1
  %13 = fmul fast double %12, %6
  store double %13, double* %11, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  ret void
}

; Function Attrs: nounwind
define private void @covariance_tmp_thunk2({ %f64XY*, %f64XY* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64XY*, %f64XY* }, { %f64XY*, %f64XY* }* %0, i64 0, i32 0
  %4 = load %f64XY*, %f64XY** %3, align 8
  %5 = getelementptr inbounds { %f64XY*, %f64XY* }, { %f64XY*, %f64XY* }* %0, i64 0, i32 1
  %6 = load %f64XY*, %f64XY** %5, align 8
  %7 = getelementptr inbounds %f64XY, %f64XY* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !2
  %mat_y_step = zext i32 %columns to i64
  %8 = getelementptr inbounds %f64XY, %f64XY* %4, i64 0, i32 6, i64 0
  %9 = ptrtoint double* %8 to i64
  %10 = and i64 %9, 31
  %11 = icmp eq i64 %10, 0
  call void @llvm.assume(i1 %11)
  %12 = getelementptr inbounds %f64XY, %f64XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %12, align 4, !range !2
  %val_y_step = zext i32 %columns1 to i64
  %13 = getelementptr inbounds %f64XY, %f64XY* %6, i64 0, i32 6, i64 0
  %14 = ptrtoint double* %13 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  call void @llvm.assume(i1 %16)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %17 = mul nuw nsw i64 %y, %val_y_step
  %18 = mul nuw nsw i64 %y, %mat_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %19 = add nuw nsw i64 %x, %17
  %20 = getelementptr %f64XY, %f64XY* %6, i64 0, i32 6, i64 %19
  %21 = load double, double* %20, align 8, !llvm.mem.parallel_loop_access !3
  %22 = add nuw nsw i64 %x, %18
  %23 = getelementptr %f64XY, %f64XY* %4, i64 0, i32 6, i64 %22
  store double %21, double* %23, align 8, !llvm.mem.parallel_loop_access !3
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

; Function Attrs: nounwind
define private void @covariance_tmp_thunk3({ %f64XY*, %f64X* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64XY*, %f64X* }, { %f64XY*, %f64X* }* %0, i64 0, i32 0
  %4 = load %f64XY*, %f64XY** %3, align 8
  %5 = getelementptr inbounds { %f64XY*, %f64X* }, { %f64XY*, %f64X* }* %0, i64 0, i32 1
  %6 = load %f64X*, %f64X** %5, align 8
  %7 = getelementptr inbounds %f64XY, %f64XY* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !2
  %mat_y_step = zext i32 %columns to i64
  %8 = getelementptr inbounds %f64XY, %f64XY* %4, i64 0, i32 6, i64 0
  %9 = ptrtoint double* %8 to i64
  %10 = and i64 %9, 31
  %11 = icmp eq i64 %10, 0
  call void @llvm.assume(i1 %11)
  %12 = getelementptr inbounds %f64X, %f64X* %6, i64 0, i32 6, i64 0
  %13 = ptrtoint double* %12 to i64
  %14 = and i64 %13, 31
  %15 = icmp eq i64 %14, 0
  call void @llvm.assume(i1 %15)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %16 = mul nuw nsw i64 %y, %mat_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %17 = add nuw nsw i64 %x, %16
  %18 = getelementptr %f64XY, %f64XY* %4, i64 0, i32 6, i64 %17
  %19 = load double, double* %18, align 8, !llvm.mem.parallel_loop_access !4
  %20 = getelementptr %f64X, %f64X* %6, i64 0, i32 6, i64 %x
  %21 = load double, double* %20, align 8, !llvm.mem.parallel_loop_access !4
  %22 = fsub fast double %19, %21
  store double %22, double* %18, align 8, !llvm.mem.parallel_loop_access !4
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

; Function Attrs: nounwind
define private void @covariance_tmp_thunk4({ %f64XY*, %f64XY*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64XY*, %f64XY*, i32 }, { %f64XY*, %f64XY*, i32 }* %0, i64 0, i32 0
  %4 = load %f64XY*, %f64XY** %3, align 8
  %5 = getelementptr inbounds { %f64XY*, %f64XY*, i32 }, { %f64XY*, %f64XY*, i32 }* %0, i64 0, i32 1
  %6 = load %f64XY*, %f64XY** %5, align 8
  %7 = getelementptr inbounds { %f64XY*, %f64XY*, i32 }, { %f64XY*, %f64XY*, i32 }* %0, i64 0, i32 2
  %8 = load i32, i32* %7, align 4
  %9 = getelementptr inbounds %f64XY, %f64XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %9, align 4, !range !2
  %dst_y_step = zext i32 %columns1 to i64
  %10 = getelementptr inbounds %f64XY, %f64XY* %4, i64 0, i32 6, i64 0
  %11 = ptrtoint double* %10 to i64
  %12 = and i64 %11, 31
  %13 = icmp eq i64 %12, 0
  call void @llvm.assume(i1 %13)
  %14 = getelementptr inbounds %f64XY, %f64XY* %6, i64 0, i32 6, i64 0
  %15 = ptrtoint double* %14 to i64
  %16 = and i64 %15, 31
  %17 = icmp eq i64 %16, 0
  call void @llvm.assume(i1 %17)
  %18 = icmp eq i32 %8, 0
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %19 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %Flow6
  %x = phi i64 [ %x_increment, %Flow6 ], [ 0, %y_body ]
  %20 = icmp ugt i64 %y, %x
  br i1 %20, label %Flow6, label %loop.preheader

loop.preheader:                                   ; preds = %x_body
  br i1 %18, label %exit4, label %true_entry3

x_exit:                                           ; preds = %Flow6
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void

true_entry3:                                      ; preds = %loop.preheader, %true_entry3
  %21 = phi i32 [ %33, %true_entry3 ], [ 0, %loop.preheader ]
  %22 = phi double [ %32, %true_entry3 ], [ 0.000000e+00, %loop.preheader ]
  %23 = sext i32 %21 to i64
  %24 = mul nuw nsw i64 %23, %dst_y_step
  %25 = add nuw nsw i64 %24, %x
  %26 = getelementptr %f64XY, %f64XY* %6, i64 0, i32 6, i64 %25
  %27 = load double, double* %26, align 8, !llvm.mem.parallel_loop_access !5
  %28 = add nuw nsw i64 %24, %y
  %29 = getelementptr %f64XY, %f64XY* %6, i64 0, i32 6, i64 %28
  %30 = load double, double* %29, align 8, !llvm.mem.parallel_loop_access !5
  %31 = fmul fast double %30, %27
  %32 = fadd fast double %31, %22
  %33 = add nuw nsw i32 %21, 1
  %34 = icmp eq i32 %33, %8
  br i1 %34, label %exit4, label %true_entry3

Flow6:                                            ; preds = %x_body, %exit4
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

exit4:                                            ; preds = %true_entry3, %loop.preheader
  %.lcssa = phi double [ 0.000000e+00, %loop.preheader ], [ %32, %true_entry3 ]
  %35 = add nuw nsw i64 %x, %19
  %36 = getelementptr %f64XY, %f64XY* %4, i64 0, i32 6, i64 %35
  store double %.lcssa, double* %36, align 8, !llvm.mem.parallel_loop_access !5
  %37 = mul nuw nsw i64 %x, %dst_y_step
  %38 = add nuw nsw i64 %37, %y
  %39 = getelementptr %f64XY, %f64XY* %4, i64 0, i32 6, i64 %38
  store double %.lcssa, double* %39, align 8, !llvm.mem.parallel_loop_access !5
  br label %Flow6
}

define %f64XY* @covariance(%f64XY*) {
entry:
  %1 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !2
  %2 = call %u0CXYT* @likely_new(i32 8512, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !2
  %4 = zext i32 %columns to i64
  %5 = alloca { %f64X*, i32 }, align 8
  %6 = bitcast { %f64X*, i32 }* %5 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %6, align 8
  %7 = getelementptr inbounds { %f64X*, i32 }, { %f64X*, i32 }* %5, i64 0, i32 1
  store i32 0, i32* %7, align 8
  %8 = bitcast { %f64X*, i32 }* %5 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64X*, i32 }*, i64, i64)* @covariance_tmp_thunk0 to i8*), i8* %8, i64 %4)
  %rows2 = load i32, i32* %3, align 4, !range !2
  %9 = zext i32 %rows2 to i64
  %10 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %11 = bitcast %u0CXYT* %10 to double*
  %12 = ptrtoint %u0CXYT* %10 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  call void @llvm.assume(i1 %14)
  %columns4 = load i32, i32* %1, align 4, !range !2
  %src_y_step = zext i32 %columns4 to i64
  %15 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 6, i64 0
  %16 = ptrtoint double* %15 to i64
  %17 = and i64 %16, 31
  %18 = icmp eq i64 %17, 0
  call void @llvm.assume(i1 %18)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %19 = mul nuw nsw i64 %y, %src_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %20 = getelementptr double, double* %11, i64 %x
  %21 = load double, double* %20, align 8
  %22 = add nuw nsw i64 %x, %19
  %23 = getelementptr %f64XY, %f64XY* %0, i64 0, i32 6, i64 %22
  %24 = load double, double* %23, align 8
  %25 = fadd fast double %24, %21
  store double %25, double* %20, align 8
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %src_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %9
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %26 = icmp eq i32 %rows, 1
  br i1 %26, label %exit, label %true_entry

true_entry:                                       ; preds = %y_exit
  %27 = uitofp i32 %rows to double
  %28 = fdiv fast double 1.000000e+00, %27
  %29 = alloca { %f64X*, double }, align 8
  %30 = bitcast { %f64X*, double }* %29 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %30, align 8
  %31 = getelementptr inbounds { %f64X*, double }, { %f64X*, double }* %29, i64 0, i32 1
  store double %28, double* %31, align 8
  %32 = bitcast { %f64X*, double }* %29 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64X*, double }*, i64, i64)* @covariance_tmp_thunk1 to i8*), i8* %32, i64 %4)
  %columns7.pre = load i32, i32* %1, align 4
  %rows8.pre = load i32, i32* %3, align 4
  br label %exit

exit:                                             ; preds = %y_exit, %true_entry
  %rows8 = phi i32 [ %rows2, %y_exit ], [ %rows8.pre, %true_entry ]
  %columns7 = phi i32 [ %columns4, %y_exit ], [ %columns7.pre, %true_entry ]
  %33 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns7, i32 %rows8, i32 1, i8* null)
  %34 = zext i32 %rows8 to i64
  %35 = alloca { %f64XY*, %f64XY* }, align 8
  %36 = bitcast { %f64XY*, %f64XY* }* %35 to %u0CXYT**
  store %u0CXYT* %33, %u0CXYT** %36, align 8
  %37 = getelementptr inbounds { %f64XY*, %f64XY* }, { %f64XY*, %f64XY* }* %35, i64 0, i32 1
  store %f64XY* %0, %f64XY** %37, align 8
  %38 = bitcast { %f64XY*, %f64XY* }* %35 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64XY*, %f64XY* }*, i64, i64)* @covariance_tmp_thunk2 to i8*), i8* %38, i64 %34)
  %39 = alloca { %f64XY*, %f64X* }, align 8
  %40 = bitcast { %f64XY*, %f64X* }* %39 to %u0CXYT**
  store %u0CXYT* %33, %u0CXYT** %40, align 8
  %41 = getelementptr inbounds { %f64XY*, %f64X* }, { %f64XY*, %f64X* }* %39, i64 0, i32 1
  %42 = bitcast %f64X** %41 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %42, align 8
  %43 = bitcast { %f64XY*, %f64X* }* %39 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64XY*, %f64X* }*, i64, i64)* @covariance_tmp_thunk3 to i8*), i8* %43, i64 %34)
  %44 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns7, i32 %columns7, i32 1, i8* null)
  %dst = bitcast %u0CXYT* %44 to %f64XY*
  %45 = zext i32 %columns7 to i64
  %46 = alloca { %f64XY*, %f64XY*, i32 }, align 8
  %47 = bitcast { %f64XY*, %f64XY*, i32 }* %46 to %u0CXYT**
  store %u0CXYT* %44, %u0CXYT** %47, align 8
  %48 = getelementptr inbounds { %f64XY*, %f64XY*, i32 }, { %f64XY*, %f64XY*, i32 }* %46, i64 0, i32 1
  %49 = bitcast %f64XY** %48 to %u0CXYT**
  store %u0CXYT* %33, %u0CXYT** %49, align 8
  %50 = getelementptr inbounds { %f64XY*, %f64XY*, i32 }, { %f64XY*, %f64XY*, i32 }* %46, i64 0, i32 2
  store i32 %rows8, i32* %50, align 8
  %51 = bitcast { %f64XY*, %f64XY*, i32 }* %46 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64XY*, %f64XY*, i32 }*, i64, i64)* @covariance_tmp_thunk4 to i8*), i8* %51, i64 %45)
  %52 = bitcast %u0CXYT* %2 to i8*
  call void @likely_release_mat(i8* %52)
  %53 = bitcast %u0CXYT* %33 to i8*
  call void @likely_release_mat(i8* %53)
  ret %f64XY* %dst
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind argmemonly }
attributes #1 = { nounwind }

!0 = distinct !{!0}
!1 = distinct !{!1}
!2 = !{i32 1, i32 -1}
!3 = distinct !{!3}
!4 = distinct !{!4}
!5 = distinct !{!5}
