; ModuleID = 'library/mandelbrot_set.md'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8XY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%i32CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i32] }
%f32CXYT = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

define noalias %u8XY* @likely_test_function(%u0CXYT** nocapture readonly) {
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
  %25 = bitcast %u0CXYT** %24 to %i32CXYT**
  %26 = load %i32CXYT*, %i32CXYT** %25, align 8
  %27 = getelementptr inbounds %i32CXYT, %i32CXYT* %26, i64 0, i32 6, i64 0
  %arg_6 = load i32, i32* %27, align 4
  %28 = call %u0CXYT* @likely_new(i32 24584, i32 1, i32 %arg_0, i32 %arg_1, i32 1, i8* null)
  %29 = zext i32 %arg_1 to i64
  %dst_y_step = zext i32 %arg_0 to i64
  %30 = getelementptr inbounds %u0CXYT, %u0CXYT* %28, i64 1
  %31 = bitcast %u0CXYT* %30 to i8*
  %32 = sitofp i32 %arg_0 to float
  %33 = sitofp i32 %arg_1 to float
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %34 = uitofp i64 %y to float
  %35 = fmul fast float %34, %arg_5
  %36 = fdiv fast float %35, %33
  %zi0 = fadd fast float %36, %arg_3
  %37 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %exit
  %x = phi i64 [ %x_increment, %exit ], [ 0, %y_body ]
  %38 = uitofp i64 %x to float
  %39 = fmul fast float %38, %arg_4
  %40 = fdiv fast float %39, %32
  %zr0 = fadd fast float %40, %arg_2
  br label %loop

loop:                                             ; preds = %x_body, %loop
  %41 = phi i32 [ %50, %loop ], [ 0, %x_body ]
  %42 = phi float [ %49, %loop ], [ 0.000000e+00, %x_body ]
  %43 = phi float [ %tmp, %loop ], [ 0.000000e+00, %x_body ]
  %44 = fmul fast float %43, %43
  %45 = fmul fast float %42, %42
  %46 = fsub fast float %44, %45
  %tmp = fadd fast float %zr0, %46
  %47 = fmul fast float %42, 2.000000e+00
  %48 = fmul fast float %47, %43
  %49 = fadd fast float %zi0, %48
  %50 = add nuw nsw i32 %41, 1
  %51 = fmul fast float %tmp, %tmp
  %52 = fmul fast float %49, %49
  %53 = fadd fast float %51, %52
  %notlhs = icmp sge i32 %50, %arg_6
  %notrhs = fcmp uge float %53, 4.000000e+00
  %54 = or i1 %notlhs, %notrhs
  br i1 %54, label %exit, label %loop

exit:                                             ; preds = %loop
  %55 = mul nuw nsw i32 %50, 255
  %56 = sdiv i32 %55, %arg_6
  %57 = trunc i32 %56 to i8
  %58 = add nuw nsw i64 %x, %37
  %59 = getelementptr i8, i8* %31, i64 %58
  store i8 %57, i8* %59, align 1, !llvm.mem.parallel_loop_access !0
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %29
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %dst = bitcast %u0CXYT* %28 to %u8XY*
  ret %u8XY* %dst
}

attributes #0 = { argmemonly nounwind }

!0 = distinct !{!0}
