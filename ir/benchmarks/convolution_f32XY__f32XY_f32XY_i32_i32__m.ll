; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind argmemonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @convolution_tmp_thunk0({ %f32XY*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32XY*, i32 }, { %f32XY*, i32 }* %0, i64 0, i32 0
  %4 = load %f32XY*, %f32XY** %3, align 8
  %5 = getelementptr inbounds { %f32XY*, i32 }, { %f32XY*, i32 }* %0, i64 0, i32 1
  %6 = load i32, i32* %5, align 4
  %7 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !0
  %mat_y_step = zext i32 %columns to i64
  %8 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 6, i64 0
  %9 = ptrtoint float* %8 to i64
  %10 = and i64 %9, 31
  %11 = icmp eq i64 %10, 0
  call void @llvm.assume(i1 %11)
  %12 = sitofp i32 %6 to float
  %13 = mul nuw nsw i64 %mat_y_step, %2
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %y_body ]
  %14 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %y
  store float %12, float* %14, align 4, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %13
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: nounwind
define private void @convolution_tmp_thunk1({ %f32XY*, %f32XY*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %0, i64 0, i32 0
  %4 = load %f32XY*, %f32XY** %3, align 8
  %5 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %0, i64 0, i32 1
  %6 = load %f32XY*, %f32XY** %5, align 8
  %7 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %0, i64 0, i32 2
  %8 = load i32, i32* %7, align 4
  %9 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 3
  %columns = load i32, i32* %9, align 4, !range !0
  %src_y_step = zext i32 %columns to i64
  %10 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 6, i64 0
  %11 = ptrtoint float* %10 to i64
  %12 = and i64 %11, 31
  %13 = icmp eq i64 %12, 0
  call void @llvm.assume(i1 %13)
  %14 = getelementptr inbounds %f32XY, %f32XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %14, align 4, !range !0
  %padded_y_step = zext i32 %columns1 to i64
  %15 = getelementptr inbounds %f32XY, %f32XY* %6, i64 0, i32 6, i64 0
  %16 = ptrtoint float* %15 to i64
  %17 = and i64 %16, 31
  %18 = icmp eq i64 %17, 0
  call void @llvm.assume(i1 %18)
  %19 = sext i32 %8 to i64
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %20 = mul nuw nsw i64 %y, %src_y_step
  %21 = add nuw nsw i64 %y, %19
  %22 = mul nuw nsw i64 %21, %padded_y_step
  %23 = add i64 %22, %19
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %24 = add nuw nsw i64 %x, %20
  %25 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %24
  %26 = load float, float* %25, align 4, !llvm.mem.parallel_loop_access !2
  %27 = add i64 %23, %x
  %28 = getelementptr %f32XY, %f32XY* %6, i64 0, i32 6, i64 %27
  store float %26, float* %28, align 4, !llvm.mem.parallel_loop_access !2
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

; Function Attrs: nounwind
define private void @convolution_tmp_thunk2({ %f32XY*, %f32XY*, %f32XY*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32 }* %0, i64 0, i32 0
  %4 = load %f32XY*, %f32XY** %3, align 8
  %5 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32 }* %0, i64 0, i32 1
  %6 = load %f32XY*, %f32XY** %5, align 8
  %7 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32 }* %0, i64 0, i32 2
  %8 = load %f32XY*, %f32XY** %7, align 8
  %9 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32 }* %0, i64 0, i32 3
  %10 = load i32, i32* %9, align 4
  %11 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 3
  %columns = load i32, i32* %11, align 4, !range !0
  %dst_y_step = zext i32 %columns to i64
  %12 = getelementptr inbounds %f32XY, %f32XY* %6, i64 0, i32 3
  %13 = getelementptr inbounds %f32XY, %f32XY* %8, i64 0, i32 3
  %14 = getelementptr inbounds %f32XY, %f32XY* %8, i64 0, i32 4
  %15 = sext i32 %10 to i64
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment29, %x_exit ]
  %16 = mul nuw nsw i64 %y, %dst_y_step
  %src-y = mul nuw nsw i64 %y, %15
  br label %x_body

x_body:                                           ; preds = %y_body, %y_exit22
  %x = phi i64 [ %x_increment27, %y_exit22 ], [ 0, %y_body ]
  %src-x = mul nuw nsw i64 %x, %15
  %rows5 = load i32, i32* %14, align 4, !range !0
  %17 = zext i32 %rows5 to i64
  %columns11 = load i32, i32* %13, align 4, !range !0
  %kernel_y_step14 = zext i32 %columns11 to i64
  %columns16 = load i32, i32* %12, align 4, !range !0
  %padded_y_step19 = zext i32 %columns16 to i64
  br label %y_body21

y_body21:                                         ; preds = %x_body, %x_exit25
  %18 = phi double [ %32, %x_exit25 ], [ 0.000000e+00, %x_body ]
  %y23 = phi i64 [ %y_increment, %x_exit25 ], [ 0, %x_body ]
  %19 = add nuw nsw i64 %y23, %src-y
  %20 = mul nuw nsw i64 %19, %padded_y_step19
  %21 = add i64 %20, %src-x
  %22 = mul nuw nsw i64 %y23, %kernel_y_step14
  br label %x_body24

x_body24:                                         ; preds = %y_body21, %x_body24
  %23 = phi double [ %32, %x_body24 ], [ %18, %y_body21 ]
  %x26 = phi i64 [ %x_increment, %x_body24 ], [ 0, %y_body21 ]
  %24 = add i64 %21, %x26
  %25 = getelementptr %f32XY, %f32XY* %6, i64 0, i32 6, i64 %24
  %26 = load float, float* %25, align 4
  %27 = add nuw nsw i64 %x26, %22
  %28 = getelementptr %f32XY, %f32XY* %8, i64 0, i32 6, i64 %27
  %29 = load float, float* %28, align 4
  %30 = fmul fast float %29, %26
  %31 = fpext float %30 to double
  %32 = fadd fast double %31, %23
  %x_increment = add nuw nsw i64 %x26, 1
  %x_postcondition = icmp eq i64 %x_increment, %kernel_y_step14
  br i1 %x_postcondition, label %x_exit25, label %x_body24

x_exit25:                                         ; preds = %x_body24
  %y_increment = add nuw nsw i64 %y23, 1
  %y_postcondition = icmp eq i64 %y_increment, %17
  br i1 %y_postcondition, label %y_exit22, label %y_body21

y_exit22:                                         ; preds = %x_exit25
  %33 = add nuw nsw i64 %x, %16
  %34 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %33
  %35 = fptrunc double %32 to float
  store float %35, float* %34, align 4, !llvm.mem.parallel_loop_access !3
  %x_increment27 = add nuw nsw i64 %x, 1
  %x_postcondition28 = icmp eq i64 %x_increment27, %dst_y_step
  br i1 %x_postcondition28, label %x_exit, label %x_body

x_exit:                                           ; preds = %y_exit22
  %y_increment29 = add nuw nsw i64 %y, 1
  %y_postcondition30 = icmp eq i64 %y_increment29, %2
  br i1 %y_postcondition30, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

define %f32XY* @convolution(%f32XY*, %f32XY*, i32, i32) {
entry:
  %4 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %4, align 4, !range !0
  %5 = shl nuw nsw i32 %3, 1
  %6 = add nuw nsw i32 %columns, %5
  %7 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 4
  %rows = load i32, i32* %7, align 4, !range !0
  %8 = add nuw nsw i32 %rows, %5
  %9 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %6, i32 %8, i32 1, i8* null)
  %10 = zext i32 %8 to i64
  %11 = alloca { %f32XY*, i32 }, align 8
  %12 = bitcast { %f32XY*, i32 }* %11 to %u0CXYT**
  store %u0CXYT* %9, %u0CXYT** %12, align 8
  %13 = getelementptr inbounds { %f32XY*, i32 }, { %f32XY*, i32 }* %11, i64 0, i32 1
  store i32 0, i32* %13, align 8
  %14 = bitcast { %f32XY*, i32 }* %11 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32XY*, i32 }*, i64, i64)* @convolution_tmp_thunk0 to i8*), i8* %14, i64 %10)
  %rows2 = load i32, i32* %7, align 4, !range !0
  %15 = zext i32 %rows2 to i64
  %16 = alloca { %f32XY*, %f32XY*, i32 }, align 8
  %17 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %16, i64 0, i32 0
  store %f32XY* %0, %f32XY** %17, align 8
  %18 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %16, i64 0, i32 1
  %19 = bitcast %f32XY** %18 to %u0CXYT**
  store %u0CXYT* %9, %u0CXYT** %19, align 8
  %20 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %16, i64 0, i32 2
  store i32 %3, i32* %20, align 8
  %21 = bitcast { %f32XY*, %f32XY*, i32 }* %16 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32XY*, %f32XY*, i32 }*, i64, i64)* @convolution_tmp_thunk1 to i8*), i8* %21, i64 %15)
  %22 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 3
  %columns4 = load i32, i32* %22, align 4, !range !0
  %23 = sub i32 %6, %columns4
  %24 = sdiv i32 %23, %2
  %25 = add nuw nsw i32 %24, 1
  %26 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 4
  %rows6 = load i32, i32* %26, align 4, !range !0
  %27 = sub i32 %8, %rows6
  %28 = sdiv i32 %27, %2
  %29 = add nuw nsw i32 %28, 1
  %30 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %25, i32 %29, i32 1, i8* null)
  %dst = bitcast %u0CXYT* %30 to %f32XY*
  %31 = zext i32 %29 to i64
  %32 = alloca { %f32XY*, %f32XY*, %f32XY*, i32 }, align 8
  %33 = bitcast { %f32XY*, %f32XY*, %f32XY*, i32 }* %32 to %u0CXYT**
  store %u0CXYT* %30, %u0CXYT** %33, align 8
  %34 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32 }* %32, i64 0, i32 1
  %35 = bitcast %f32XY** %34 to %u0CXYT**
  store %u0CXYT* %9, %u0CXYT** %35, align 8
  %36 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32 }* %32, i64 0, i32 2
  store %f32XY* %1, %f32XY** %36, align 8
  %37 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32 }* %32, i64 0, i32 3
  store i32 %2, i32* %37, align 8
  %38 = bitcast { %f32XY*, %f32XY*, %f32XY*, i32 }* %32 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32XY*, %f32XY*, %f32XY*, i32 }*, i64, i64)* @convolution_tmp_thunk2 to i8*), i8* %38, i64 %31)
  %39 = bitcast %u0CXYT* %9 to i8*
  call void @likely_release_mat(i8* %39)
  ret %f32XY* %dst
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind argmemonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
