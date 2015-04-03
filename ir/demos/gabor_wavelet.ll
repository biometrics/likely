; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%i32CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i32] }
%f32CXYT = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

; Function Attrs: nounwind readnone
declare float @llvm.cos.f32(float) #2

; Function Attrs: nounwind readnone
declare float @llvm.sin.f32(float) #2

; Function Attrs: nounwind readnone
declare float @llvm.exp.f32(float) #2

; Function Attrs: nounwind
define %f32XY* @likely_test_function(%u0CXYT** nocapture readonly) #1 {
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
  %32 = tail call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %29, i32 %31, i32 1, i8* null)
  %33 = zext i32 %31 to i64
  %dst_y_step = zext i32 %29 to i64
  %34 = getelementptr inbounds %u0CXYT, %u0CXYT* %32, i64 1
  %35 = bitcast %u0CXYT* %34 to float*
  %36 = ptrtoint %u0CXYT* %34 to i64
  %37 = and i64 %36, 31
  %38 = icmp eq i64 %37, 0
  tail call void @llvm.assume(i1 %38)
  %39 = tail call float @llvm.cos.f32(float %arg_4)
  %40 = tail call float @llvm.sin.f32(float %arg_4)
  %41 = fdiv float 0x401921FB60000000, %arg_5
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %y_offset = mul nuw nsw i64 %y, %dst_y_step
  %42 = trunc i64 %y to i32
  %43 = sub i32 %42, %arg_1
  %44 = sitofp i32 %43 to float
  %45 = fmul float %44, %40
  %46 = fmul float %44, %39
  br label %x_body

x_body:                                           ; preds = %x_body, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %x_body ]
  %x_offset = add nuw nsw i64 %x, %y_offset
  %47 = trunc i64 %x to i32
  %48 = sub i32 %47, %arg_0
  %49 = sitofp i32 %48 to float
  %50 = fmul float %39, %49
  %51 = fadd float %45, %50
  %52 = sub nsw i32 0, %48
  %53 = sitofp i32 %52 to float
  %54 = fmul float %40, %53
  %55 = fadd float %46, %54
  %56 = fdiv float %51, %arg_2
  %57 = fmul float %56, %56
  %58 = fdiv float %55, %arg_3
  %59 = fmul float %58, %58
  %60 = fadd float %57, %59
  %61 = fmul float %60, -5.000000e-01
  %62 = tail call float @llvm.exp.f32(float %61)
  %63 = fmul float %51, %41
  %64 = fadd float %arg_6, %63
  %65 = tail call float @llvm.cos.f32(float %64)
  %66 = fmul float %62, %65
  %67 = getelementptr float, float* %35, i64 %x_offset
  store float %66, float* %67, align 4, !llvm.mem.parallel_loop_access !0
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !0

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %33
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %68 = bitcast %u0CXYT* %32 to %f32XY*
  ret %f32XY* %68
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }
attributes #2 = { nounwind readnone }

!0 = distinct !{!0}
