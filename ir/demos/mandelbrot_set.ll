; ModuleID = 'library/mandelbrot_set.md'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8XY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%i32CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i32] }
%f32CXYT = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %u8XY* @likely_test_function(%u0CXYT** nocapture readonly) {
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
  %32 = ptrtoint %u0CXYT* %30 to i64
  %33 = and i64 %32, 31
  %34 = icmp eq i64 %33, 0
  call void @llvm.assume(i1 %34)
  %35 = sitofp i32 %arg_0 to float
  %36 = sitofp i32 %arg_1 to float
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %37 = uitofp i64 %y to float
  %38 = fmul fast float %37, %arg_5
  %39 = fdiv fast float %38, %36
  %zi0 = fadd fast float %39, %arg_3
  %40 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %exit
  %x = phi i64 [ %x_increment, %exit ], [ 0, %y_body ]
  %41 = uitofp i64 %x to float
  %42 = fmul fast float %41, %arg_4
  %43 = fdiv fast float %42, %35
  %zr0 = fadd fast float %43, %arg_2
  br label %loop

loop:                                             ; preds = %x_body, %loop
  %44 = phi i32 [ %53, %loop ], [ 0, %x_body ]
  %45 = phi float [ %52, %loop ], [ 0.000000e+00, %x_body ]
  %46 = phi float [ %tmp, %loop ], [ 0.000000e+00, %x_body ]
  %47 = fmul fast float %46, %46
  %48 = fmul fast float %45, %45
  %49 = fsub fast float %47, %48
  %tmp = fadd fast float %zr0, %49
  %50 = fmul fast float %45, 2.000000e+00
  %51 = fmul fast float %50, %46
  %52 = fadd fast float %zi0, %51
  %53 = add nuw nsw i32 %44, 1
  %54 = fmul fast float %tmp, %tmp
  %55 = fmul fast float %52, %52
  %56 = fadd fast float %54, %55
  %notlhs = icmp sge i32 %53, %arg_6
  %notrhs = fcmp uge float %56, 4.000000e+00
  %57 = or i1 %notlhs, %notrhs
  br i1 %57, label %exit, label %loop

exit:                                             ; preds = %loop
  %58 = mul nuw nsw i32 %53, 255
  %59 = sdiv i32 %58, %arg_6
  %60 = trunc i32 %59 to i8
  %61 = add nuw nsw i64 %x, %40
  %62 = getelementptr i8, i8* %31, i64 %61
  store i8 %60, i8* %62, align 1, !llvm.mem.parallel_loop_access !0
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
attributes #1 = { nounwind }

!0 = distinct !{!0}
