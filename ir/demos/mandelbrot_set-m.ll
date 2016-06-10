; ModuleID = 'library/mandelbrot_set.md'
source_filename = "library/mandelbrot_set.md"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i32] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: norecurse nounwind
define private void @likely_test_function_tmp_thunk0({ %u8Matrix*, i32, i32, float, float, float, float, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %u8Matrix*, i32, i32, float, float, float, float, i32 }, { %u8Matrix*, i32, i32, float, float, float, float, i32 }* %0, i64 0, i32 0
  %4 = load %u8Matrix*, %u8Matrix** %3, align 8
  %5 = getelementptr inbounds { %u8Matrix*, i32, i32, float, float, float, float, i32 }, { %u8Matrix*, i32, i32, float, float, float, float, i32 }* %0, i64 0, i32 1
  %6 = load i32, i32* %5, align 4
  %7 = getelementptr inbounds { %u8Matrix*, i32, i32, float, float, float, float, i32 }, { %u8Matrix*, i32, i32, float, float, float, float, i32 }* %0, i64 0, i32 2
  %8 = load i32, i32* %7, align 4
  %9 = getelementptr inbounds { %u8Matrix*, i32, i32, float, float, float, float, i32 }, { %u8Matrix*, i32, i32, float, float, float, float, i32 }* %0, i64 0, i32 3
  %10 = load float, float* %9, align 4
  %11 = getelementptr inbounds { %u8Matrix*, i32, i32, float, float, float, float, i32 }, { %u8Matrix*, i32, i32, float, float, float, float, i32 }* %0, i64 0, i32 4
  %12 = load float, float* %11, align 4
  %13 = getelementptr inbounds { %u8Matrix*, i32, i32, float, float, float, float, i32 }, { %u8Matrix*, i32, i32, float, float, float, float, i32 }* %0, i64 0, i32 5
  %14 = load float, float* %13, align 4
  %15 = getelementptr inbounds { %u8Matrix*, i32, i32, float, float, float, float, i32 }, { %u8Matrix*, i32, i32, float, float, float, float, i32 }* %0, i64 0, i32 6
  %16 = load float, float* %15, align 4
  %17 = getelementptr inbounds { %u8Matrix*, i32, i32, float, float, float, float, i32 }, { %u8Matrix*, i32, i32, float, float, float, float, i32 }* %0, i64 0, i32 7
  %18 = load i32, i32* %17, align 4
  %19 = getelementptr inbounds %u8Matrix, %u8Matrix* %4, i64 0, i32 3
  %columns = load i32, i32* %19, align 4, !range !0
  %dst_y_step = zext i32 %columns to i64
  %20 = sitofp i32 %6 to float
  %21 = sitofp i32 %8 to float
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %22 = uitofp i64 %y to float
  %23 = fmul fast float %22, %16
  %24 = fdiv fast float %23, %21
  %zi0 = fadd fast float %12, %24
  %25 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %exit
  %x = phi i64 [ %x_increment, %exit ], [ 0, %y_body ]
  %26 = uitofp i64 %x to float
  %27 = fmul fast float %26, %14
  %28 = fdiv fast float %27, %20
  %zr0 = fadd fast float %10, %28
  br label %loop

loop:                                             ; preds = %x_body, %loop
  %29 = phi i32 [ %38, %loop ], [ 0, %x_body ]
  %30 = phi float [ %37, %loop ], [ 0.000000e+00, %x_body ]
  %31 = phi float [ %tmp, %loop ], [ 0.000000e+00, %x_body ]
  %32 = fmul fast float %31, %31
  %33 = fmul fast float %30, %30
  %34 = fsub fast float %32, %33
  %tmp = fadd float %zr0, %34
  %35 = fmul fast float %30, 2.000000e+00
  %36 = fmul fast float %35, %31
  %37 = fadd float %zi0, %36
  %38 = add nuw nsw i32 %29, 1
  %39 = fmul fast float %tmp, %tmp
  %40 = fmul fast float %37, %37
  %41 = fadd fast float %39, %40
  %notlhs = icmp sge i32 %38, %18
  %notrhs = fcmp uge float %41, 4.000000e+00
  %42 = or i1 %notlhs, %notrhs
  br i1 %42, label %exit, label %loop

exit:                                             ; preds = %loop
  %43 = mul nuw nsw i32 %38, 255
  %44 = sdiv i32 %43, %18
  %45 = trunc i32 %44 to i8
  %46 = add nuw nsw i64 %x, %25
  %47 = getelementptr %u8Matrix, %u8Matrix* %4, i64 0, i32 6, i64 %46
  store i8 %45, i8* %47, align 1, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %u8Matrix* @likely_test_function(%u0Matrix** nocapture readonly) {
entry:
  %1 = bitcast %u0Matrix** %0 to %u32Matrix**
  %2 = load %u32Matrix*, %u32Matrix** %1, align 8
  %3 = getelementptr inbounds %u32Matrix, %u32Matrix* %2, i64 0, i32 6, i64 0
  %arg_0 = load i32, i32* %3, align 4
  %4 = getelementptr %u0Matrix*, %u0Matrix** %0, i64 1
  %5 = bitcast %u0Matrix** %4 to %u32Matrix**
  %6 = load %u32Matrix*, %u32Matrix** %5, align 8
  %7 = getelementptr inbounds %u32Matrix, %u32Matrix* %6, i64 0, i32 6, i64 0
  %arg_1 = load i32, i32* %7, align 4
  %8 = getelementptr %u0Matrix*, %u0Matrix** %0, i64 2
  %9 = bitcast %u0Matrix** %8 to %f32Matrix**
  %10 = load %f32Matrix*, %f32Matrix** %9, align 8
  %11 = getelementptr inbounds %f32Matrix, %f32Matrix* %10, i64 0, i32 6, i64 0
  %arg_2 = load float, float* %11, align 4
  %12 = getelementptr %u0Matrix*, %u0Matrix** %0, i64 3
  %13 = bitcast %u0Matrix** %12 to %f32Matrix**
  %14 = load %f32Matrix*, %f32Matrix** %13, align 8
  %15 = getelementptr inbounds %f32Matrix, %f32Matrix* %14, i64 0, i32 6, i64 0
  %arg_3 = load float, float* %15, align 4
  %16 = getelementptr %u0Matrix*, %u0Matrix** %0, i64 4
  %17 = bitcast %u0Matrix** %16 to %f32Matrix**
  %18 = load %f32Matrix*, %f32Matrix** %17, align 8
  %19 = getelementptr inbounds %f32Matrix, %f32Matrix* %18, i64 0, i32 6, i64 0
  %arg_4 = load float, float* %19, align 4
  %20 = getelementptr %u0Matrix*, %u0Matrix** %0, i64 5
  %21 = bitcast %u0Matrix** %20 to %f32Matrix**
  %22 = load %f32Matrix*, %f32Matrix** %21, align 8
  %23 = getelementptr inbounds %f32Matrix, %f32Matrix* %22, i64 0, i32 6, i64 0
  %arg_5 = load float, float* %23, align 4
  %24 = getelementptr %u0Matrix*, %u0Matrix** %0, i64 6
  %25 = bitcast %u0Matrix** %24 to %u32Matrix**
  %26 = load %u32Matrix*, %u32Matrix** %25, align 8
  %27 = getelementptr inbounds %u32Matrix, %u32Matrix* %26, i64 0, i32 6, i64 0
  %arg_6 = load i32, i32* %27, align 4
  %28 = call %u0Matrix* @likely_new(i32 24584, i32 1, i32 %arg_0, i32 %arg_1, i32 1, i8* null)
  %dst = bitcast %u0Matrix* %28 to %u8Matrix*
  %29 = zext i32 %arg_1 to i64
  %30 = alloca { %u8Matrix*, i32, i32, float, float, float, float, i32 }, align 8
  %31 = bitcast { %u8Matrix*, i32, i32, float, float, float, float, i32 }* %30 to %u0Matrix**
  store %u0Matrix* %28, %u0Matrix** %31, align 8
  %32 = getelementptr inbounds { %u8Matrix*, i32, i32, float, float, float, float, i32 }, { %u8Matrix*, i32, i32, float, float, float, float, i32 }* %30, i64 0, i32 1
  store i32 %arg_0, i32* %32, align 8
  %33 = getelementptr inbounds { %u8Matrix*, i32, i32, float, float, float, float, i32 }, { %u8Matrix*, i32, i32, float, float, float, float, i32 }* %30, i64 0, i32 2
  store i32 %arg_1, i32* %33, align 4
  %34 = getelementptr inbounds { %u8Matrix*, i32, i32, float, float, float, float, i32 }, { %u8Matrix*, i32, i32, float, float, float, float, i32 }* %30, i64 0, i32 3
  store float %arg_2, float* %34, align 8
  %35 = getelementptr inbounds { %u8Matrix*, i32, i32, float, float, float, float, i32 }, { %u8Matrix*, i32, i32, float, float, float, float, i32 }* %30, i64 0, i32 4
  store float %arg_3, float* %35, align 4
  %36 = getelementptr inbounds { %u8Matrix*, i32, i32, float, float, float, float, i32 }, { %u8Matrix*, i32, i32, float, float, float, float, i32 }* %30, i64 0, i32 5
  store float %arg_4, float* %36, align 8
  %37 = getelementptr inbounds { %u8Matrix*, i32, i32, float, float, float, float, i32 }, { %u8Matrix*, i32, i32, float, float, float, float, i32 }* %30, i64 0, i32 6
  store float %arg_5, float* %37, align 4
  %38 = getelementptr inbounds { %u8Matrix*, i32, i32, float, float, float, float, i32 }, { %u8Matrix*, i32, i32, float, float, float, float, i32 }* %30, i64 0, i32 7
  store i32 %arg_6, i32* %38, align 8
  %39 = bitcast { %u8Matrix*, i32, i32, float, float, float, float, i32 }* %30 to i8*
  call void @likely_fork(i8* bitcast (void ({ %u8Matrix*, i32, i32, float, float, float, float, i32 }*, i64, i64)* @likely_test_function_tmp_thunk0 to i8*), i8* %39, i64 %29)
  ret %u8Matrix* %dst
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { norecurse nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
