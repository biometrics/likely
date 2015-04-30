; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%i32CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i32] }
%f32CXYT = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @likely_test_function_tmp_thunk0({ %f32XY*, i32, i32, float, float, float, float, float }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32XY*, i32, i32, float, float, float, float, float }, { %f32XY*, i32, i32, float, float, float, float, float }* %0, i64 0, i32 0
  %4 = load %f32XY*, %f32XY** %3, align 8
  %5 = getelementptr { %f32XY*, i32, i32, float, float, float, float, float }, { %f32XY*, i32, i32, float, float, float, float, float }* %0, i64 0, i32 1
  %6 = bitcast i32* %5 to i64*
  %.combined = load i64, i64* %6, align 4
  %combine.extract.trunc = trunc i64 %.combined to i32
  %combine.extract.shift = lshr i64 %.combined, 32
  %combine.extract.trunc1 = trunc i64 %combine.extract.shift to i32
  %7 = getelementptr inbounds { %f32XY*, i32, i32, float, float, float, float, float }, { %f32XY*, i32, i32, float, float, float, float, float }* %0, i64 0, i32 3
  %8 = load float, float* %7, align 4
  %9 = getelementptr inbounds { %f32XY*, i32, i32, float, float, float, float, float }, { %f32XY*, i32, i32, float, float, float, float, float }* %0, i64 0, i32 4
  %10 = load float, float* %9, align 4
  %11 = getelementptr inbounds { %f32XY*, i32, i32, float, float, float, float, float }, { %f32XY*, i32, i32, float, float, float, float, float }* %0, i64 0, i32 5
  %12 = load float, float* %11, align 4
  %13 = getelementptr inbounds { %f32XY*, i32, i32, float, float, float, float, float }, { %f32XY*, i32, i32, float, float, float, float, float }* %0, i64 0, i32 6
  %14 = load float, float* %13, align 4
  %15 = getelementptr inbounds { %f32XY*, i32, i32, float, float, float, float, float }, { %f32XY*, i32, i32, float, float, float, float, float }* %0, i64 0, i32 7
  %16 = load float, float* %15, align 4
  %17 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 3
  %columns = load i32, i32* %17, align 4, !range !0
  %dst_y_step = zext i32 %columns to i64
  %18 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 6, i64 0
  %19 = ptrtoint float* %18 to i64
  %20 = and i64 %19, 31
  %21 = icmp eq i64 %20, 0
  call void @llvm.assume(i1 %21)
  %22 = call float @llvm.cos.f32(float %12)
  %23 = call float @llvm.sin.f32(float %12)
  %24 = fdiv float 0x401921FB60000000, %14
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %25 = trunc i64 %y to i32
  %26 = sub i32 %25, %combine.extract.trunc1
  %27 = sitofp i32 %26 to float
  %28 = fmul float %27, %23
  %29 = fmul float %27, %22
  %30 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %x_body, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %x_body ]
  %31 = trunc i64 %x to i32
  %32 = sub i32 %31, %combine.extract.trunc
  %33 = sitofp i32 %32 to float
  %34 = fmul float %22, %33
  %35 = fadd float %28, %34
  %36 = sub nsw i32 0, %32
  %37 = sitofp i32 %36 to float
  %38 = fmul float %23, %37
  %39 = fadd float %29, %38
  %40 = fdiv float %35, %8
  %41 = fmul float %40, %40
  %42 = fdiv float %39, %10
  %43 = fmul float %42, %42
  %44 = fadd float %41, %43
  %45 = fmul float %44, -5.000000e-01
  %46 = call float @llvm.exp.f32(float %45)
  %47 = fmul float %35, %24
  %48 = fadd float %16, %47
  %49 = call float @llvm.cos.f32(float %48)
  %50 = fmul float %46, %49
  %51 = add nuw nsw i64 %x, %30
  %52 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %51
  store float %50, float* %52, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !1

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

; Function Attrs: nounwind readnone
declare float @llvm.cos.f32(float) #2

; Function Attrs: nounwind readnone
declare float @llvm.sin.f32(float) #2

; Function Attrs: nounwind readnone
declare float @llvm.exp.f32(float) #2

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %f32XY* @likely_test_function(%u0CXYT** nocapture readonly) {
entry:
  %1 = bitcast %u0CXYT** %0 to %i32CXYT**
  %2 = load %i32CXYT*, %i32CXYT** %1, align 8
  %3 = getelementptr inbounds %i32CXYT, %i32CXYT* %2, i64 0, i32 6, i64 0
  %arg_0 = load i32, i32* %3, align 4
  %4 = getelementptr %u0CXYT*, %u0CXYT** %0, i64 1
  %5 = bitcast %u0CXYT** %4 to %i32CXYT**
  %6 = load %i32CXYT*, %i32CXYT** %5, align 8
  %7 = getelementptr inbounds %i32CXYT, %i32CXYT* %6, i64 0, i32 6, i64 0
  %arg_1 = load i32, i32* %7, align 4
  %8 = getelementptr %u0CXYT*, %u0CXYT** %0, i64 2
  %9 = bitcast %u0CXYT** %8 to %f32CXYT**
  %10 = load %f32CXYT*, %f32CXYT** %9, align 8
  %11 = getelementptr inbounds %f32CXYT, %f32CXYT* %10, i64 0, i32 6, i64 0
  %arg_2 = load float, float* %11, align 4
  %12 = getelementptr %u0CXYT*, %u0CXYT** %0, i64 3
  %13 = bitcast %u0CXYT** %12 to %f32CXYT**
  %14 = load %f32CXYT*, %f32CXYT** %13, align 8
  %15 = getelementptr inbounds %f32CXYT, %f32CXYT* %14, i64 0, i32 6, i64 0
  %arg_3 = load float, float* %15, align 4
  %16 = getelementptr %u0CXYT*, %u0CXYT** %0, i64 4
  %17 = bitcast %u0CXYT** %16 to %f32CXYT**
  %18 = load %f32CXYT*, %f32CXYT** %17, align 8
  %19 = getelementptr inbounds %f32CXYT, %f32CXYT* %18, i64 0, i32 6, i64 0
  %arg_4 = load float, float* %19, align 4
  %20 = getelementptr %u0CXYT*, %u0CXYT** %0, i64 5
  %21 = bitcast %u0CXYT** %20 to %f32CXYT**
  %22 = load %f32CXYT*, %f32CXYT** %21, align 8
  %23 = getelementptr inbounds %f32CXYT, %f32CXYT* %22, i64 0, i32 6, i64 0
  %arg_5 = load float, float* %23, align 4
  %24 = getelementptr %u0CXYT*, %u0CXYT** %0, i64 6
  %25 = bitcast %u0CXYT** %24 to %f32CXYT**
  %26 = load %f32CXYT*, %f32CXYT** %25, align 8
  %27 = getelementptr inbounds %f32CXYT, %f32CXYT* %26, i64 0, i32 6, i64 0
  %arg_6 = load float, float* %27, align 4
  %28 = shl nuw nsw i32 %arg_0, 1
  %29 = or i32 %28, 1
  %30 = shl nuw nsw i32 %arg_1, 1
  %31 = or i32 %30, 1
  %32 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %29, i32 %31, i32 1, i8* null)
  %33 = bitcast %u0CXYT* %32 to %f32XY*
  %34 = zext i32 %31 to i64
  %35 = alloca { %f32XY*, i32, i32, float, float, float, float, float }, align 8
  %36 = bitcast { %f32XY*, i32, i32, float, float, float, float, float }* %35 to %u0CXYT**
  store %u0CXYT* %32, %u0CXYT** %36, align 8
  %37 = getelementptr inbounds { %f32XY*, i32, i32, float, float, float, float, float }, { %f32XY*, i32, i32, float, float, float, float, float }* %35, i64 0, i32 1
  store i32 %arg_0, i32* %37, align 8
  %38 = getelementptr inbounds { %f32XY*, i32, i32, float, float, float, float, float }, { %f32XY*, i32, i32, float, float, float, float, float }* %35, i64 0, i32 2
  store i32 %arg_1, i32* %38, align 4
  %39 = getelementptr inbounds { %f32XY*, i32, i32, float, float, float, float, float }, { %f32XY*, i32, i32, float, float, float, float, float }* %35, i64 0, i32 3
  store float %arg_2, float* %39, align 8
  %40 = getelementptr inbounds { %f32XY*, i32, i32, float, float, float, float, float }, { %f32XY*, i32, i32, float, float, float, float, float }* %35, i64 0, i32 4
  store float %arg_3, float* %40, align 4
  %41 = getelementptr inbounds { %f32XY*, i32, i32, float, float, float, float, float }, { %f32XY*, i32, i32, float, float, float, float, float }* %35, i64 0, i32 5
  store float %arg_4, float* %41, align 8
  %42 = getelementptr inbounds { %f32XY*, i32, i32, float, float, float, float, float }, { %f32XY*, i32, i32, float, float, float, float, float }* %35, i64 0, i32 6
  store float %arg_5, float* %42, align 4
  %43 = getelementptr inbounds { %f32XY*, i32, i32, float, float, float, float, float }, { %f32XY*, i32, i32, float, float, float, float, float }* %35, i64 0, i32 7
  store float %arg_6, float* %43, align 8
  %44 = bitcast { %f32XY*, i32, i32, float, float, float, float, float }* %35 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32XY*, i32, i32, float, float, float, float, float }*, i64, i64)* @likely_test_function_tmp_thunk0 to i8*), i8* %44, i64 %34)
  ret %f32XY* %33
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }
attributes #2 = { nounwind readnone }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
