; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%i32CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i32] }
%f32CXYT = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

; Function Attrs: nounwind readnone
declare float @llvm.cos.f32(float) #2

; Function Attrs: nounwind readnone
declare float @llvm.sin.f32(float) #2

; Function Attrs: nounwind readnone
declare float @llvm.exp.f32(float) #2

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
  %33 = zext i32 %31 to i64
  %dst_y_step = zext i32 %29 to i64
  %34 = getelementptr inbounds %u0CXYT, %u0CXYT* %32, i64 1
  %35 = bitcast %u0CXYT* %34 to float*
  %36 = ptrtoint %u0CXYT* %34 to i64
  %37 = and i64 %36, 31
  %38 = icmp eq i64 %37, 0
  call void @llvm.assume(i1 %38)
  %39 = call fast float @llvm.cos.f32(float %arg_4)
  %40 = call fast float @llvm.sin.f32(float %arg_4)
  %41 = fdiv fast float 0x401921FB60000000, %arg_5
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %42 = trunc i64 %y to i32
  %dy = sub i32 %42, %arg_1
  %43 = sitofp i32 %dy to float
  %44 = fmul fast float %43, %40
  %45 = fmul fast float %43, %39
  %46 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %47 = trunc i64 %x to i32
  %dx = sub i32 %47, %arg_0
  %48 = sitofp i32 %dx to float
  %49 = fmul fast float %48, %39
  %xp = fadd fast float %49, %44
  %50 = sub nsw i32 0, %dx
  %51 = sitofp i32 %50 to float
  %52 = fmul fast float %51, %40
  %yp = fadd fast float %52, %45
  %53 = fdiv fast float %xp, %arg_2
  %54 = fmul fast float %53, %53
  %55 = fdiv fast float %yp, %arg_3
  %56 = fmul fast float %55, %55
  %57 = fadd fast float %54, %56
  %58 = fmul fast float %57, -5.000000e-01
  %59 = call fast float @llvm.exp.f32(float %58)
  %60 = fmul fast float %xp, %41
  %61 = fadd fast float %60, %arg_6
  %62 = call fast float @llvm.cos.f32(float %61)
  %63 = fmul fast float %62, %59
  %64 = add nuw nsw i64 %x, %46
  %65 = getelementptr float, float* %35, i64 %64
  store float %63, float* %65, align 4, !llvm.mem.parallel_loop_access !0
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %33
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %dst = bitcast %u0CXYT* %32 to %f32XY*
  ret %f32XY* %dst
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind }
attributes #2 = { nounwind readnone }

!0 = distinct !{!0}
