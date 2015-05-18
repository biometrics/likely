; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64X = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @covariance_tmp_thunk0({ %f64X* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64X* }, { %f64X* }* %0, i64 0, i32 0
  %4 = load %f64X*, %f64X** %3, align 8
  %5 = getelementptr inbounds %f64X, %f64X* %4, i64 0, i32 6, i64 0
  %6 = ptrtoint double* %5 to i64
  %7 = and i64 %6, 31
  %8 = icmp eq i64 %7, 0
  call void @llvm.assume(i1 %8)
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %9 = getelementptr %f64X, %f64X* %4, i64 0, i32 6, i64 %x
  store double 0.000000e+00, double* %9, align 8, !llvm.mem.parallel_loop_access !0
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !0

x_exit:                                           ; preds = %x_body
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: nounwind
define private void @covariance_tmp_thunk1({ %f64X*, float }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64X*, float }, { %f64X*, float }* %0, i64 0, i32 0
  %4 = load %f64X*, %f64X** %3, align 8
  %5 = getelementptr inbounds { %f64X*, float }, { %f64X*, float }* %0, i64 0, i32 1
  %6 = load float, float* %5, align 4
  %7 = getelementptr inbounds %f64X, %f64X* %4, i64 0, i32 6, i64 0
  %8 = ptrtoint double* %7 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  %11 = fpext float %6 to double
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %12 = getelementptr %f64X, %f64X* %4, i64 0, i32 6, i64 %x
  %13 = load double, double* %12, align 8, !llvm.mem.parallel_loop_access !1
  %14 = fmul fast double %13, %11
  store double %14, double* %12, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !1

x_exit:                                           ; preds = %x_body
  ret void
}

; Function Attrs: nounwind
define private void @covariance_tmp_thunk2({ %f64XY*, %f64XY*, %f64X* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64XY*, %f64XY*, %f64X* }, { %f64XY*, %f64XY*, %f64X* }* %0, i64 0, i32 0
  %4 = load %f64XY*, %f64XY** %3, align 8
  %5 = getelementptr inbounds { %f64XY*, %f64XY*, %f64X* }, { %f64XY*, %f64XY*, %f64X* }* %0, i64 0, i32 1
  %6 = load %f64XY*, %f64XY** %5, align 8
  %7 = getelementptr inbounds { %f64XY*, %f64XY*, %f64X* }, { %f64XY*, %f64XY*, %f64X* }* %0, i64 0, i32 2
  %8 = load %f64X*, %f64X** %7, align 8
  %9 = getelementptr inbounds %f64XY, %f64XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %9, align 4, !range !2
  %centered_y_step = zext i32 %columns1 to i64
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
  %18 = getelementptr inbounds %f64X, %f64X* %8, i64 0, i32 6, i64 0
  %19 = ptrtoint double* %18 to i64
  %20 = and i64 %19, 31
  %21 = icmp eq i64 %20, 0
  call void @llvm.assume(i1 %21)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %22 = mul nuw nsw i64 %y, %centered_y_step
  br label %x_body

x_body:                                           ; preds = %x_body, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %x_body ]
  %23 = add nuw nsw i64 %x, %22
  %24 = getelementptr %f64XY, %f64XY* %6, i64 0, i32 6, i64 %23
  %25 = load double, double* %24, align 8, !llvm.mem.parallel_loop_access !3
  %26 = getelementptr %f64X, %f64X* %8, i64 0, i32 6, i64 %x
  %27 = load double, double* %26, align 8, !llvm.mem.parallel_loop_access !3
  %28 = fsub fast double %25, %27
  %29 = getelementptr %f64XY, %f64XY* %4, i64 0, i32 6, i64 %23
  store double %28, double* %29, align 8, !llvm.mem.parallel_loop_access !3
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %centered_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !3

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

; Function Attrs: nounwind
define private void @covariance_tmp_thunk3({ %f64XY*, %f64XY*, i32 }* noalias nocapture readonly, i64, i64) #1 {
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

x_body:                                           ; preds = %exit, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %exit ]
  %20 = icmp ugt i64 %y, %x
  br i1 %20, label %exit, label %label.preheader

label.preheader:                                  ; preds = %x_body
  br i1 %18, label %exit4, label %true_entry3

exit:                                             ; preds = %x_body, %exit4
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !4

x_exit:                                           ; preds = %exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void

true_entry3:                                      ; preds = %label.preheader, %true_entry3
  %21 = phi i32 [ %33, %true_entry3 ], [ 0, %label.preheader ]
  %22 = phi double [ %32, %true_entry3 ], [ 0.000000e+00, %label.preheader ]
  %23 = sext i32 %21 to i64
  %24 = mul nuw nsw i64 %23, %dst_y_step
  %25 = add nuw nsw i64 %24, %x
  %26 = getelementptr %f64XY, %f64XY* %6, i64 0, i32 6, i64 %25
  %27 = load double, double* %26, align 8, !llvm.mem.parallel_loop_access !4
  %28 = add nuw nsw i64 %24, %y
  %29 = getelementptr %f64XY, %f64XY* %6, i64 0, i32 6, i64 %28
  %30 = load double, double* %29, align 8, !llvm.mem.parallel_loop_access !4
  %31 = fmul fast double %30, %27
  %32 = fadd fast double %31, %22
  %33 = add nuw nsw i32 %21, 1
  %34 = icmp eq i32 %33, %8
  br i1 %34, label %exit4, label %true_entry3

exit4:                                            ; preds = %true_entry3, %label.preheader
  %.lcssa = phi double [ 0.000000e+00, %label.preheader ], [ %32, %true_entry3 ]
  %35 = add nuw nsw i64 %x, %19
  %36 = getelementptr %f64XY, %f64XY* %4, i64 0, i32 6, i64 %35
  store double %.lcssa, double* %36, align 8, !llvm.mem.parallel_loop_access !4
  %37 = mul nuw nsw i64 %x, %dst_y_step
  %38 = add nuw nsw i64 %37, %y
  %39 = getelementptr %f64XY, %f64XY* %4, i64 0, i32 6, i64 %38
  store double %.lcssa, double* %39, align 8, !llvm.mem.parallel_loop_access !4
  br label %exit
}

define %f64XY* @covariance(%f64XY*) {
entry:
  %1 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !2
  %2 = call %u0CXYT* @likely_new(i32 8512, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = zext i32 %columns to i64
  %4 = alloca { %f64X* }, align 8
  %5 = bitcast { %f64X* }* %4 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %5, align 8
  %6 = bitcast { %f64X* }* %4 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64X* }*, i64, i64)* @covariance_tmp_thunk0 to i8*), i8* %6, i64 %3)
  %7 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 4
  %rows = load i32, i32* %7, align 4, !range !2
  %8 = zext i32 %rows to i64
  %9 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %10 = bitcast %u0CXYT* %9 to double*
  %11 = ptrtoint %u0CXYT* %9 to i64
  %12 = and i64 %11, 31
  %13 = icmp eq i64 %12, 0
  call void @llvm.assume(i1 %13)
  %columns3 = load i32, i32* %1, align 4, !range !2
  %src_y_step = zext i32 %columns3 to i64
  %14 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 6, i64 0
  %15 = ptrtoint double* %14 to i64
  %16 = and i64 %15, 31
  %17 = icmp eq i64 %16, 0
  call void @llvm.assume(i1 %17)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %18 = mul nuw nsw i64 %y, %src_y_step
  br label %x_body

x_body:                                           ; preds = %x_body, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %x_body ]
  %19 = getelementptr double, double* %10, i64 %x
  %20 = load double, double* %19, align 8
  %21 = add nuw nsw i64 %x, %18
  %22 = getelementptr %f64XY, %f64XY* %0, i64 0, i32 6, i64 %21
  %23 = load double, double* %22, align 8
  %24 = fadd fast double %23, %20
  store double %24, double* %19, align 8
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %src_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %8
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %25 = uitofp i32 %rows to float
  %26 = fdiv fast float 1.000000e+00, %25
  %27 = alloca { %f64X*, float }, align 8
  %28 = bitcast { %f64X*, float }* %27 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %28, align 8
  %29 = getelementptr inbounds { %f64X*, float }, { %f64X*, float }* %27, i64 0, i32 1
  store float %26, float* %29, align 8
  %30 = bitcast { %f64X*, float }* %27 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64X*, float }*, i64, i64)* @covariance_tmp_thunk1 to i8*), i8* %30, i64 %3)
  %columns7 = load i32, i32* %1, align 4, !range !2
  %rows8 = load i32, i32* %7, align 4, !range !2
  %31 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns7, i32 %rows8, i32 1, i8* null)
  %32 = zext i32 %rows8 to i64
  %33 = alloca { %f64XY*, %f64XY*, %f64X* }, align 8
  %34 = bitcast { %f64XY*, %f64XY*, %f64X* }* %33 to %u0CXYT**
  store %u0CXYT* %31, %u0CXYT** %34, align 8
  %35 = getelementptr inbounds { %f64XY*, %f64XY*, %f64X* }, { %f64XY*, %f64XY*, %f64X* }* %33, i64 0, i32 1
  store %f64XY* %0, %f64XY** %35, align 8
  %36 = getelementptr inbounds { %f64XY*, %f64XY*, %f64X* }, { %f64XY*, %f64XY*, %f64X* }* %33, i64 0, i32 2
  %37 = bitcast %f64X** %36 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %37, align 8
  %38 = bitcast { %f64XY*, %f64XY*, %f64X* }* %33 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64XY*, %f64XY*, %f64X* }*, i64, i64)* @covariance_tmp_thunk2 to i8*), i8* %38, i64 %32)
  %39 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns7, i32 %columns7, i32 1, i8* null)
  %40 = bitcast %u0CXYT* %39 to %f64XY*
  %41 = zext i32 %columns7 to i64
  %42 = alloca { %f64XY*, %f64XY*, i32 }, align 8
  %43 = bitcast { %f64XY*, %f64XY*, i32 }* %42 to %u0CXYT**
  store %u0CXYT* %39, %u0CXYT** %43, align 8
  %44 = getelementptr inbounds { %f64XY*, %f64XY*, i32 }, { %f64XY*, %f64XY*, i32 }* %42, i64 0, i32 1
  %45 = bitcast %f64XY** %44 to %u0CXYT**
  store %u0CXYT* %31, %u0CXYT** %45, align 8
  %46 = getelementptr inbounds { %f64XY*, %f64XY*, i32 }, { %f64XY*, %f64XY*, i32 }* %42, i64 0, i32 2
  store i32 %rows8, i32* %46, align 8
  %47 = bitcast { %f64XY*, %f64XY*, i32 }* %42 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64XY*, %f64XY*, i32 }*, i64, i64)* @covariance_tmp_thunk3 to i8*), i8* %47, i64 %41)
  %48 = bitcast %u0CXYT* %2 to i8*
  call void @likely_release_mat(i8* %48)
  %49 = bitcast %u0CXYT* %31 to i8*
  call void @likely_release_mat(i8* %49)
  ret %f64XY* %40
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = distinct !{!0}
!1 = distinct !{!1}
!2 = !{i32 1, i32 -1}
!3 = distinct !{!3}
!4 = distinct !{!4}
