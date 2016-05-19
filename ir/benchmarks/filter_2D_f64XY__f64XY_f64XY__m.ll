; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: norecurse nounwind
define private void @filter_2D_tmp_thunk0({ %f64XY*, i32 }* noalias nocapture readonly, i64, i64) #2 {
entry:
  %3 = getelementptr inbounds { %f64XY*, i32 }, { %f64XY*, i32 }* %0, i64 0, i32 0
  %4 = load %f64XY*, %f64XY** %3, align 8
  %5 = getelementptr inbounds { %f64XY*, i32 }, { %f64XY*, i32 }* %0, i64 0, i32 1
  %6 = load i32, i32* %5, align 4
  %7 = getelementptr inbounds %f64XY, %f64XY* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !0
  %mat_y_step = zext i32 %columns to i64
  %8 = getelementptr inbounds %f64XY, %f64XY* %4, i64 0, i32 6, i64 0
  %9 = ptrtoint double* %8 to i64
  %10 = and i64 %9, 31
  %11 = icmp eq i64 %10, 0
  call void @llvm.assume(i1 %11)
  %12 = sitofp i32 %6 to double
  %13 = mul nuw nsw i64 %mat_y_step, %2
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %y_body ]
  %14 = getelementptr %f64XY, %f64XY* %4, i64 0, i32 6, i64 %y
  store double %12, double* %14, align 8, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %13
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  ret void
}

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: norecurse nounwind
define private void @filter_2D_tmp_thunk1({ %f64XY*, %f64XY*, i32, i32 }* noalias nocapture readonly, i64, i64) #2 {
entry:
  %3 = getelementptr inbounds { %f64XY*, %f64XY*, i32, i32 }, { %f64XY*, %f64XY*, i32, i32 }* %0, i64 0, i32 0
  %4 = load %f64XY*, %f64XY** %3, align 8
  %5 = getelementptr inbounds { %f64XY*, %f64XY*, i32, i32 }, { %f64XY*, %f64XY*, i32, i32 }* %0, i64 0, i32 1
  %6 = load %f64XY*, %f64XY** %5, align 8
  %7 = getelementptr inbounds { %f64XY*, %f64XY*, i32, i32 }, { %f64XY*, %f64XY*, i32, i32 }* %0, i64 0, i32 2
  %8 = load i32, i32* %7, align 4
  %9 = getelementptr inbounds { %f64XY*, %f64XY*, i32, i32 }, { %f64XY*, %f64XY*, i32, i32 }* %0, i64 0, i32 3
  %10 = load i32, i32* %9, align 4
  %11 = getelementptr inbounds %f64XY, %f64XY* %4, i64 0, i32 3
  %columns = load i32, i32* %11, align 4, !range !0
  %src_y_step = zext i32 %columns to i64
  %12 = getelementptr inbounds %f64XY, %f64XY* %4, i64 0, i32 6, i64 0
  %13 = ptrtoint double* %12 to i64
  %14 = and i64 %13, 31
  %15 = icmp eq i64 %14, 0
  call void @llvm.assume(i1 %15)
  %16 = getelementptr inbounds %f64XY, %f64XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %16, align 4, !range !0
  %padded_y_step = zext i32 %columns1 to i64
  %17 = getelementptr inbounds %f64XY, %f64XY* %6, i64 0, i32 6, i64 0
  %18 = ptrtoint double* %17 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  %21 = sext i32 %8 to i64
  %22 = sext i32 %10 to i64
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %23 = mul nuw nsw i64 %y, %src_y_step
  %24 = add nuw nsw i64 %y, %22
  %25 = mul nuw nsw i64 %24, %padded_y_step
  %26 = add i64 %25, %21
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %27 = add nuw nsw i64 %x, %23
  %28 = getelementptr %f64XY, %f64XY* %4, i64 0, i32 6, i64 %27
  %29 = load double, double* %28, align 8, !llvm.mem.parallel_loop_access !2
  %30 = add i64 %26, %x
  %31 = getelementptr %f64XY, %f64XY* %6, i64 0, i32 6, i64 %30
  store double %29, double* %31, align 8, !llvm.mem.parallel_loop_access !2
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %src_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

; Function Attrs: norecurse nounwind
define private void @filter_2D_tmp_thunk2({ %f64XY*, %f64XY*, %f64XY*, i32, i32 }* noalias nocapture readonly, i64, i64) #2 {
entry:
  %3 = getelementptr inbounds { %f64XY*, %f64XY*, %f64XY*, i32, i32 }, { %f64XY*, %f64XY*, %f64XY*, i32, i32 }* %0, i64 0, i32 0
  %4 = load %f64XY*, %f64XY** %3, align 8
  %5 = getelementptr inbounds { %f64XY*, %f64XY*, %f64XY*, i32, i32 }, { %f64XY*, %f64XY*, %f64XY*, i32, i32 }* %0, i64 0, i32 1
  %6 = load %f64XY*, %f64XY** %5, align 8
  %7 = getelementptr inbounds { %f64XY*, %f64XY*, %f64XY*, i32, i32 }, { %f64XY*, %f64XY*, %f64XY*, i32, i32 }* %0, i64 0, i32 2
  %8 = load %f64XY*, %f64XY** %7, align 8
  %9 = getelementptr inbounds { %f64XY*, %f64XY*, %f64XY*, i32, i32 }, { %f64XY*, %f64XY*, %f64XY*, i32, i32 }* %0, i64 0, i32 3
  %10 = load i32, i32* %9, align 4
  %11 = getelementptr inbounds { %f64XY*, %f64XY*, %f64XY*, i32, i32 }, { %f64XY*, %f64XY*, %f64XY*, i32, i32 }* %0, i64 0, i32 4
  %12 = load i32, i32* %11, align 4
  %13 = getelementptr inbounds %f64XY, %f64XY* %4, i64 0, i32 3
  %columns = load i32, i32* %13, align 4, !range !0
  %dst_y_step = zext i32 %columns to i64
  %14 = getelementptr inbounds %f64XY, %f64XY* %4, i64 0, i32 6, i64 0
  %15 = ptrtoint double* %14 to i64
  %16 = and i64 %15, 31
  %17 = icmp eq i64 %16, 0
  call void @llvm.assume(i1 %17)
  %18 = getelementptr inbounds %f64XY, %f64XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %18, align 4, !range !0
  %padded_y_step = zext i32 %columns1 to i64
  %19 = getelementptr inbounds %f64XY, %f64XY* %6, i64 0, i32 6, i64 0
  %20 = ptrtoint double* %19 to i64
  %21 = and i64 %20, 31
  %22 = icmp eq i64 %21, 0
  call void @llvm.assume(i1 %22)
  %23 = getelementptr inbounds %f64XY, %f64XY* %8, i64 0, i32 3
  %columns3 = load i32, i32* %23, align 4, !range !0
  %kernel_y_step = zext i32 %columns3 to i64
  %24 = getelementptr inbounds %f64XY, %f64XY* %8, i64 0, i32 6, i64 0
  %25 = ptrtoint double* %24 to i64
  %26 = and i64 %25, 31
  %27 = icmp eq i64 %26, 0
  call void @llvm.assume(i1 %27)
  %28 = icmp eq i32 %10, 0
  %29 = icmp eq i32 %12, 0
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %30 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %exit
  %x = phi i64 [ %x_increment, %exit ], [ 0, %y_body ]
  %31 = add nuw nsw i64 %x, %30
  %32 = getelementptr %f64XY, %f64XY* %4, i64 0, i32 6, i64 %31
  br i1 %29, label %exit, label %loop6.preheader

loop6.preheader:                                  ; preds = %x_body, %exit8
  %33 = phi i32 [ %54, %exit8 ], [ 0, %x_body ]
  %34 = phi double [ %53, %exit8 ], [ 0.000000e+00, %x_body ]
  br i1 %28, label %exit8, label %true_entry7.lr.ph

true_entry7.lr.ph:                                ; preds = %loop6.preheader
  %35 = sext i32 %33 to i64
  %36 = add nuw nsw i64 %35, %y
  %37 = mul nuw nsw i64 %36, %padded_y_step
  %38 = add i64 %37, %x
  %39 = mul nuw nsw i64 %35, %kernel_y_step
  br label %true_entry7

exit:                                             ; preds = %exit8, %x_body
  %.lcssa9 = phi double [ 0.000000e+00, %x_body ], [ %53, %exit8 ]
  store double %.lcssa9, double* %32, align 8, !llvm.mem.parallel_loop_access !3
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void

true_entry7:                                      ; preds = %true_entry7.lr.ph, %true_entry7
  %40 = phi double [ %50, %true_entry7 ], [ %34, %true_entry7.lr.ph ]
  %41 = phi i32 [ %51, %true_entry7 ], [ 0, %true_entry7.lr.ph ]
  %42 = sext i32 %41 to i64
  %43 = add i64 %38, %42
  %44 = getelementptr %f64XY, %f64XY* %6, i64 0, i32 6, i64 %43
  %45 = load double, double* %44, align 8, !llvm.mem.parallel_loop_access !3
  %46 = add nuw nsw i64 %42, %39
  %47 = getelementptr %f64XY, %f64XY* %8, i64 0, i32 6, i64 %46
  %48 = load double, double* %47, align 8, !llvm.mem.parallel_loop_access !3
  %49 = fmul fast double %48, %45
  %50 = fadd fast double %49, %40
  %51 = add nuw nsw i32 %41, 1
  %52 = icmp eq i32 %51, %10
  br i1 %52, label %exit8, label %true_entry7

exit8:                                            ; preds = %true_entry7, %loop6.preheader
  %53 = phi double [ %34, %loop6.preheader ], [ %50, %true_entry7 ]
  %54 = add nuw nsw i32 %33, 1
  %55 = icmp eq i32 %54, %12
  br i1 %55, label %exit, label %loop6.preheader
}

define %f64XY* @filter_2D(%f64XY*, %f64XY*) {
entry:
  %2 = getelementptr inbounds %f64XY, %f64XY* %1, i64 0, i32 3
  %width = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f64XY, %f64XY* %1, i64 0, i32 4
  %height = load i32, i32* %3, align 4, !range !0
  %4 = srem i32 %width, 2
  %5 = icmp eq i32 %4, 1
  call void @llvm.assume(i1 %5)
  %6 = srem i32 %height, 2
  %7 = icmp eq i32 %6, 1
  call void @llvm.assume(i1 %7)
  %8 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 3
  %columns = load i32, i32* %8, align 4, !range !0
  %9 = add i32 %width, -1
  %10 = add nuw nsw i32 %columns, %9
  %11 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 4
  %rows = load i32, i32* %11, align 4, !range !0
  %12 = add i32 %height, -1
  %13 = add nuw nsw i32 %rows, %12
  %14 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %10, i32 %13, i32 1, i8* null)
  %15 = zext i32 %13 to i64
  %16 = alloca { %f64XY*, i32 }, align 8
  %17 = bitcast { %f64XY*, i32 }* %16 to %u0CXYT**
  store %u0CXYT* %14, %u0CXYT** %17, align 8
  %18 = getelementptr inbounds { %f64XY*, i32 }, { %f64XY*, i32 }* %16, i64 0, i32 1
  store i32 0, i32* %18, align 8
  %19 = bitcast { %f64XY*, i32 }* %16 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64XY*, i32 }*, i64, i64)* @filter_2D_tmp_thunk0 to i8*), i8* %19, i64 %15)
  %pad-columns = sdiv i32 %9, 2
  %pad-rows = sdiv i32 %12, 2
  %rows2 = load i32, i32* %11, align 4, !range !0
  %20 = zext i32 %rows2 to i64
  %21 = alloca { %f64XY*, %f64XY*, i32, i32 }, align 8
  %22 = getelementptr inbounds { %f64XY*, %f64XY*, i32, i32 }, { %f64XY*, %f64XY*, i32, i32 }* %21, i64 0, i32 0
  store %f64XY* %0, %f64XY** %22, align 8
  %23 = getelementptr inbounds { %f64XY*, %f64XY*, i32, i32 }, { %f64XY*, %f64XY*, i32, i32 }* %21, i64 0, i32 1
  %24 = bitcast %f64XY** %23 to %u0CXYT**
  store %u0CXYT* %14, %u0CXYT** %24, align 8
  %25 = getelementptr inbounds { %f64XY*, %f64XY*, i32, i32 }, { %f64XY*, %f64XY*, i32, i32 }* %21, i64 0, i32 2
  store i32 %pad-columns, i32* %25, align 8
  %26 = getelementptr inbounds { %f64XY*, %f64XY*, i32, i32 }, { %f64XY*, %f64XY*, i32, i32 }* %21, i64 0, i32 3
  store i32 %pad-rows, i32* %26, align 4
  %27 = bitcast { %f64XY*, %f64XY*, i32, i32 }* %21 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64XY*, %f64XY*, i32, i32 }*, i64, i64)* @filter_2D_tmp_thunk1 to i8*), i8* %27, i64 %20)
  %columns3 = load i32, i32* %8, align 4, !range !0
  %rows4 = load i32, i32* %11, align 4, !range !0
  %28 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns3, i32 %rows4, i32 1, i8* null)
  %dst = bitcast %u0CXYT* %28 to %f64XY*
  %29 = zext i32 %rows4 to i64
  %30 = alloca { %f64XY*, %f64XY*, %f64XY*, i32, i32 }, align 8
  %31 = bitcast { %f64XY*, %f64XY*, %f64XY*, i32, i32 }* %30 to %u0CXYT**
  store %u0CXYT* %28, %u0CXYT** %31, align 8
  %32 = getelementptr inbounds { %f64XY*, %f64XY*, %f64XY*, i32, i32 }, { %f64XY*, %f64XY*, %f64XY*, i32, i32 }* %30, i64 0, i32 1
  %33 = bitcast %f64XY** %32 to %u0CXYT**
  store %u0CXYT* %14, %u0CXYT** %33, align 8
  %34 = getelementptr inbounds { %f64XY*, %f64XY*, %f64XY*, i32, i32 }, { %f64XY*, %f64XY*, %f64XY*, i32, i32 }* %30, i64 0, i32 2
  store %f64XY* %1, %f64XY** %34, align 8
  %35 = getelementptr inbounds { %f64XY*, %f64XY*, %f64XY*, i32, i32 }, { %f64XY*, %f64XY*, %f64XY*, i32, i32 }* %30, i64 0, i32 3
  store i32 %width, i32* %35, align 8
  %36 = getelementptr inbounds { %f64XY*, %f64XY*, %f64XY*, i32, i32 }, { %f64XY*, %f64XY*, %f64XY*, i32, i32 }* %30, i64 0, i32 4
  store i32 %height, i32* %36, align 4
  %37 = bitcast { %f64XY*, %f64XY*, %f64XY*, i32, i32 }* %30 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64XY*, %f64XY*, %f64XY*, i32, i32 }*, i64, i64)* @filter_2D_tmp_thunk2 to i8*), i8* %37, i64 %29)
  %38 = bitcast %u0CXYT* %14 to i8*
  call void @likely_release_mat(i8* %38)
  ret %f64XY* %dst
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind }
attributes #1 = { argmemonly nounwind }
attributes #2 = { norecurse nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
