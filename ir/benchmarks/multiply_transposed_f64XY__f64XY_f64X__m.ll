; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%f64X = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind argmemonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @multiply_transposed_tmp_thunk0({ %f64XY*, %f64XY* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64XY*, %f64XY* }, { %f64XY*, %f64XY* }* %0, i64 0, i32 0
  %4 = load %f64XY*, %f64XY** %3, align 8
  %5 = getelementptr inbounds { %f64XY*, %f64XY* }, { %f64XY*, %f64XY* }* %0, i64 0, i32 1
  %6 = load %f64XY*, %f64XY** %5, align 8
  %7 = getelementptr inbounds %f64XY, %f64XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %7, align 4, !range !0
  %mat_y_step = zext i32 %columns1 to i64
  %8 = getelementptr inbounds %f64XY, %f64XY* %4, i64 0, i32 6, i64 0
  %9 = ptrtoint double* %8 to i64
  %10 = and i64 %9, 31
  %11 = icmp eq i64 %10, 0
  call void @llvm.assume(i1 %11)
  %12 = getelementptr inbounds %f64XY, %f64XY* %6, i64 0, i32 6, i64 0
  %13 = ptrtoint double* %12 to i64
  %14 = and i64 %13, 31
  %15 = icmp eq i64 %14, 0
  call void @llvm.assume(i1 %15)
  %16 = mul nuw nsw i64 %mat_y_step, %2
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %y_body ]
  %17 = getelementptr %f64XY, %f64XY* %6, i64 0, i32 6, i64 %y
  %18 = load double, double* %17, align 8, !llvm.mem.parallel_loop_access !1
  %19 = getelementptr %f64XY, %f64XY* %4, i64 0, i32 6, i64 %y
  store double %18, double* %19, align 8, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %16
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: nounwind
define private void @multiply_transposed_tmp_thunk1({ %f64XY*, %f64X* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64XY*, %f64X* }, { %f64XY*, %f64X* }* %0, i64 0, i32 0
  %4 = load %f64XY*, %f64XY** %3, align 8
  %5 = getelementptr inbounds { %f64XY*, %f64X* }, { %f64XY*, %f64X* }* %0, i64 0, i32 1
  %6 = load %f64X*, %f64X** %5, align 8
  %7 = getelementptr inbounds %f64XY, %f64XY* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !0
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
  %19 = load double, double* %18, align 8, !llvm.mem.parallel_loop_access !2
  %20 = getelementptr %f64X, %f64X* %6, i64 0, i32 6, i64 %x
  %21 = load double, double* %20, align 8, !llvm.mem.parallel_loop_access !2
  %22 = fsub fast double %19, %21
  store double %22, double* %18, align 8, !llvm.mem.parallel_loop_access !2
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
define private void @multiply_transposed_tmp_thunk2({ %f64XY*, %f64XY*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64XY*, %f64XY*, i32 }, { %f64XY*, %f64XY*, i32 }* %0, i64 0, i32 0
  %4 = load %f64XY*, %f64XY** %3, align 8
  %5 = getelementptr inbounds { %f64XY*, %f64XY*, i32 }, { %f64XY*, %f64XY*, i32 }* %0, i64 0, i32 1
  %6 = load %f64XY*, %f64XY** %5, align 8
  %7 = getelementptr inbounds { %f64XY*, %f64XY*, i32 }, { %f64XY*, %f64XY*, i32 }* %0, i64 0, i32 2
  %8 = load i32, i32* %7, align 4
  %9 = getelementptr inbounds %f64XY, %f64XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %9, align 4, !range !0
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
  %27 = load double, double* %26, align 8, !llvm.mem.parallel_loop_access !3
  %28 = add nuw nsw i64 %24, %y
  %29 = getelementptr %f64XY, %f64XY* %6, i64 0, i32 6, i64 %28
  %30 = load double, double* %29, align 8, !llvm.mem.parallel_loop_access !3
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
  store double %.lcssa, double* %36, align 8, !llvm.mem.parallel_loop_access !3
  %37 = mul nuw nsw i64 %x, %dst_y_step
  %38 = add nuw nsw i64 %37, %y
  %39 = getelementptr %f64XY, %f64XY* %4, i64 0, i32 6, i64 %38
  store double %.lcssa, double* %39, align 8, !llvm.mem.parallel_loop_access !3
  br label %Flow6
}

define %f64XY* @multiply_transposed(%f64XY*, %f64X*) {
entry:
  %2 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %5 = zext i32 %rows to i64
  %6 = alloca { %f64XY*, %f64XY* }, align 8
  %7 = bitcast { %f64XY*, %f64XY* }* %6 to %u0CXYT**
  store %u0CXYT* %4, %u0CXYT** %7, align 8
  %8 = getelementptr inbounds { %f64XY*, %f64XY* }, { %f64XY*, %f64XY* }* %6, i64 0, i32 1
  store %f64XY* %0, %f64XY** %8, align 8
  %9 = bitcast { %f64XY*, %f64XY* }* %6 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64XY*, %f64XY* }*, i64, i64)* @multiply_transposed_tmp_thunk0 to i8*), i8* %9, i64 %5)
  %10 = alloca { %f64XY*, %f64X* }, align 8
  %11 = bitcast { %f64XY*, %f64X* }* %10 to %u0CXYT**
  store %u0CXYT* %4, %u0CXYT** %11, align 8
  %12 = getelementptr inbounds { %f64XY*, %f64X* }, { %f64XY*, %f64X* }* %10, i64 0, i32 1
  store %f64X* %1, %f64X** %12, align 8
  %13 = bitcast { %f64XY*, %f64X* }* %10 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64XY*, %f64X* }*, i64, i64)* @multiply_transposed_tmp_thunk1 to i8*), i8* %13, i64 %5)
  %14 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %dst = bitcast %u0CXYT* %14 to %f64XY*
  %15 = zext i32 %columns to i64
  %16 = alloca { %f64XY*, %f64XY*, i32 }, align 8
  %17 = bitcast { %f64XY*, %f64XY*, i32 }* %16 to %u0CXYT**
  store %u0CXYT* %14, %u0CXYT** %17, align 8
  %18 = getelementptr inbounds { %f64XY*, %f64XY*, i32 }, { %f64XY*, %f64XY*, i32 }* %16, i64 0, i32 1
  %19 = bitcast %f64XY** %18 to %u0CXYT**
  store %u0CXYT* %4, %u0CXYT** %19, align 8
  %20 = getelementptr inbounds { %f64XY*, %f64XY*, i32 }, { %f64XY*, %f64XY*, i32 }* %16, i64 0, i32 2
  store i32 %rows, i32* %20, align 8
  %21 = bitcast { %f64XY*, %f64XY*, i32 }* %16 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64XY*, %f64XY*, i32 }*, i64, i64)* @multiply_transposed_tmp_thunk2 to i8*), i8* %21, i64 %15)
  %22 = bitcast %u0CXYT* %4 to i8*
  call void @likely_release_mat(i8* %22)
  ret %f64XY* %dst
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind argmemonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
