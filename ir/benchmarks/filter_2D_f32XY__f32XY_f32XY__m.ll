; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: norecurse nounwind
define private void @filter_2D_tmp_thunk0({ %f32XY*, i32 }* noalias nocapture readonly, i64, i64) #2 {
entry:
  %3 = getelementptr inbounds { %f32XY*, i32 }, { %f32XY*, i32 }* %0, i64 0, i32 0
  %4 = load %f32XY*, %f32XY** %3, align 8
  %5 = getelementptr inbounds { %f32XY*, i32 }, { %f32XY*, i32 }* %0, i64 0, i32 1
  %6 = load i32, i32* %5, align 4
  %7 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !0
  %mat_y_step = zext i32 %columns to i64
  %8 = sitofp i32 %6 to float
  %9 = mul nuw nsw i64 %mat_y_step, %2
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %y_body ]
  %10 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %y
  store float %8, float* %10, align 4, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %9
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  ret void
}

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: norecurse nounwind
define private void @filter_2D_tmp_thunk1({ %f32XY*, %f32XY*, i32, i32 }* noalias nocapture readonly, i64, i64) #2 {
entry:
  %3 = getelementptr inbounds { %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, i32, i32 }* %0, i64 0, i32 0
  %4 = load %f32XY*, %f32XY** %3, align 8
  %5 = getelementptr inbounds { %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, i32, i32 }* %0, i64 0, i32 1
  %6 = load %f32XY*, %f32XY** %5, align 8
  %7 = getelementptr inbounds { %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, i32, i32 }* %0, i64 0, i32 2
  %8 = load i32, i32* %7, align 4
  %9 = getelementptr inbounds { %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, i32, i32 }* %0, i64 0, i32 3
  %10 = load i32, i32* %9, align 4
  %11 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 3
  %columns = load i32, i32* %11, align 4, !range !0
  %src_y_step = zext i32 %columns to i64
  %12 = getelementptr inbounds %f32XY, %f32XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %12, align 4, !range !0
  %padded_y_step = zext i32 %columns1 to i64
  %13 = sext i32 %8 to i64
  %14 = sext i32 %10 to i64
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %15 = mul nuw nsw i64 %y, %src_y_step
  %16 = add nuw nsw i64 %y, %14
  %17 = mul nuw nsw i64 %16, %padded_y_step
  %18 = add i64 %17, %13
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %19 = add nuw nsw i64 %x, %15
  %20 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %19
  %21 = load float, float* %20, align 4, !llvm.mem.parallel_loop_access !2
  %22 = add i64 %18, %x
  %23 = getelementptr %f32XY, %f32XY* %6, i64 0, i32 6, i64 %22
  store float %21, float* %23, align 4, !llvm.mem.parallel_loop_access !2
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
define private void @filter_2D_tmp_thunk2({ %f32XY*, %f32XY*, %f32XY*, i32, i32 }* noalias nocapture readonly, i64, i64) #2 {
entry:
  %3 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %0, i64 0, i32 0
  %4 = load %f32XY*, %f32XY** %3, align 8
  %5 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %0, i64 0, i32 1
  %6 = load %f32XY*, %f32XY** %5, align 8
  %7 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %0, i64 0, i32 2
  %8 = load %f32XY*, %f32XY** %7, align 8
  %9 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %0, i64 0, i32 3
  %10 = load i32, i32* %9, align 4
  %11 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %0, i64 0, i32 4
  %12 = load i32, i32* %11, align 4
  %13 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 3
  %columns = load i32, i32* %13, align 4, !range !0
  %dst_y_step = zext i32 %columns to i64
  %14 = getelementptr inbounds %f32XY, %f32XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %14, align 4, !range !0
  %padded_y_step = zext i32 %columns1 to i64
  %15 = getelementptr inbounds %f32XY, %f32XY* %8, i64 0, i32 3
  %columns3 = load i32, i32* %15, align 4, !range !0
  %kernel_y_step = zext i32 %columns3 to i64
  %16 = icmp eq i32 %10, 0
  %17 = icmp eq i32 %12, 0
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %18 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %exit
  %x = phi i64 [ %x_increment, %exit ], [ 0, %y_body ]
  %19 = add nuw nsw i64 %x, %18
  %20 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %19
  br i1 %17, label %exit, label %loop6.preheader

loop6.preheader:                                  ; preds = %x_body, %exit8
  %21 = phi i32 [ %42, %exit8 ], [ 0, %x_body ]
  %22 = phi float [ %41, %exit8 ], [ 0.000000e+00, %x_body ]
  br i1 %16, label %exit8, label %true_entry7.lr.ph

true_entry7.lr.ph:                                ; preds = %loop6.preheader
  %23 = sext i32 %21 to i64
  %24 = add nuw nsw i64 %23, %y
  %25 = mul nuw nsw i64 %24, %padded_y_step
  %26 = add i64 %25, %x
  %27 = mul nuw nsw i64 %23, %kernel_y_step
  br label %true_entry7

exit:                                             ; preds = %exit8, %x_body
  %.lcssa9 = phi float [ 0.000000e+00, %x_body ], [ %41, %exit8 ]
  store float %.lcssa9, float* %20, align 4, !llvm.mem.parallel_loop_access !3
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
  %28 = phi float [ %38, %true_entry7 ], [ %22, %true_entry7.lr.ph ]
  %29 = phi i32 [ %39, %true_entry7 ], [ 0, %true_entry7.lr.ph ]
  %30 = sext i32 %29 to i64
  %31 = add i64 %26, %30
  %32 = getelementptr %f32XY, %f32XY* %6, i64 0, i32 6, i64 %31
  %33 = load float, float* %32, align 4, !llvm.mem.parallel_loop_access !3
  %34 = add nuw nsw i64 %30, %27
  %35 = getelementptr %f32XY, %f32XY* %8, i64 0, i32 6, i64 %34
  %36 = load float, float* %35, align 4, !llvm.mem.parallel_loop_access !3
  %37 = fmul fast float %36, %33
  %38 = fadd fast float %37, %28
  %39 = add nuw nsw i32 %29, 1
  %40 = icmp eq i32 %39, %10
  br i1 %40, label %exit8, label %true_entry7

exit8:                                            ; preds = %true_entry7, %loop6.preheader
  %41 = phi float [ %22, %loop6.preheader ], [ %38, %true_entry7 ]
  %42 = add nuw nsw i32 %21, 1
  %43 = icmp eq i32 %42, %12
  br i1 %43, label %exit, label %loop6.preheader
}

define %f32XY* @filter_2D(%f32XY*, %f32XY*) {
entry:
  %2 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 3
  %width = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 4
  %height = load i32, i32* %3, align 4, !range !0
  %4 = srem i32 %width, 2
  %5 = icmp eq i32 %4, 1
  call void @llvm.assume(i1 %5)
  %6 = srem i32 %height, 2
  %7 = icmp eq i32 %6, 1
  call void @llvm.assume(i1 %7)
  %8 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %8, align 4, !range !0
  %9 = add i32 %width, -1
  %10 = add nuw nsw i32 %columns, %9
  %11 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 4
  %rows = load i32, i32* %11, align 4, !range !0
  %12 = add i32 %height, -1
  %13 = add nuw nsw i32 %rows, %12
  %14 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %10, i32 %13, i32 1, i8* null)
  %15 = zext i32 %13 to i64
  %16 = alloca { %f32XY*, i32 }, align 8
  %17 = bitcast { %f32XY*, i32 }* %16 to %u0CXYT**
  store %u0CXYT* %14, %u0CXYT** %17, align 8
  %18 = getelementptr inbounds { %f32XY*, i32 }, { %f32XY*, i32 }* %16, i64 0, i32 1
  store i32 0, i32* %18, align 8
  %19 = bitcast { %f32XY*, i32 }* %16 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32XY*, i32 }*, i64, i64)* @filter_2D_tmp_thunk0 to i8*), i8* %19, i64 %15)
  %pad-columns = sdiv i32 %9, 2
  %pad-rows = sdiv i32 %12, 2
  %rows2 = load i32, i32* %11, align 4, !range !0
  %20 = zext i32 %rows2 to i64
  %21 = alloca { %f32XY*, %f32XY*, i32, i32 }, align 8
  %22 = getelementptr inbounds { %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, i32, i32 }* %21, i64 0, i32 0
  store %f32XY* %0, %f32XY** %22, align 8
  %23 = getelementptr inbounds { %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, i32, i32 }* %21, i64 0, i32 1
  %24 = bitcast %f32XY** %23 to %u0CXYT**
  store %u0CXYT* %14, %u0CXYT** %24, align 8
  %25 = getelementptr inbounds { %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, i32, i32 }* %21, i64 0, i32 2
  store i32 %pad-columns, i32* %25, align 8
  %26 = getelementptr inbounds { %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, i32, i32 }* %21, i64 0, i32 3
  store i32 %pad-rows, i32* %26, align 4
  %27 = bitcast { %f32XY*, %f32XY*, i32, i32 }* %21 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32XY*, %f32XY*, i32, i32 }*, i64, i64)* @filter_2D_tmp_thunk1 to i8*), i8* %27, i64 %20)
  %columns3 = load i32, i32* %8, align 4, !range !0
  %rows4 = load i32, i32* %11, align 4, !range !0
  %28 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns3, i32 %rows4, i32 1, i8* null)
  %dst = bitcast %u0CXYT* %28 to %f32XY*
  %29 = zext i32 %rows4 to i64
  %30 = alloca { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, align 8
  %31 = bitcast { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %30 to %u0CXYT**
  store %u0CXYT* %28, %u0CXYT** %31, align 8
  %32 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %30, i64 0, i32 1
  %33 = bitcast %f32XY** %32 to %u0CXYT**
  store %u0CXYT* %14, %u0CXYT** %33, align 8
  %34 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %30, i64 0, i32 2
  store %f32XY* %1, %f32XY** %34, align 8
  %35 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %30, i64 0, i32 3
  store i32 %width, i32* %35, align 8
  %36 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %30, i64 0, i32 4
  store i32 %height, i32* %36, align 4
  %37 = bitcast { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %30 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32XY*, %f32XY*, %f32XY*, i32, i32 }*, i64, i64)* @filter_2D_tmp_thunk2 to i8*), i8* %37, i64 %29)
  %38 = bitcast %u0CXYT* %14 to i8*
  call void @likely_release_mat(i8* %38)
  ret %f32XY* %dst
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind }
attributes #1 = { argmemonly nounwind }
attributes #2 = { norecurse nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
