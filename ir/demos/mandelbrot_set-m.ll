; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8XY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%i32CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i32] }
%f32CXYT = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind argmemonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @likely_test_function_tmp_thunk0({ %u8XY*, i32, i32, float, float, float, float, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %0, i64 0, i32 0
  %4 = load %u8XY*, %u8XY** %3, align 8
  %5 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %0, i64 0, i32 1
  %6 = load i32, i32* %5, align 4
  %7 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %0, i64 0, i32 2
  %8 = load i32, i32* %7, align 4
  %9 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %0, i64 0, i32 3
  %10 = load float, float* %9, align 4
  %11 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %0, i64 0, i32 4
  %12 = load float, float* %11, align 4
  %13 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %0, i64 0, i32 5
  %14 = load float, float* %13, align 4
  %15 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %0, i64 0, i32 6
  %16 = load float, float* %15, align 4
  %17 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %0, i64 0, i32 7
  %18 = load i32, i32* %17, align 4
  %19 = getelementptr inbounds %u8XY, %u8XY* %4, i64 0, i32 3
  %columns = load i32, i32* %19, align 4, !range !0
  %dst_y_step = zext i32 %columns to i64
  %20 = getelementptr inbounds %u8XY, %u8XY* %4, i64 0, i32 6, i64 0
  %21 = ptrtoint i8* %20 to i64
  %22 = and i64 %21, 31
  %23 = icmp eq i64 %22, 0
  call void @llvm.assume(i1 %23)
  %24 = sitofp i32 %6 to float
  %25 = sitofp i32 %8 to float
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %26 = uitofp i64 %y to float
  %27 = fmul fast float %26, %16
  %28 = fdiv fast float %27, %25
  %zi0 = fadd fast float %28, %12
  %29 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %exit
  %x = phi i64 [ %x_increment, %exit ], [ 0, %y_body ]
  %30 = uitofp i64 %x to float
  %31 = fmul fast float %30, %14
  %32 = fdiv fast float %31, %24
  %zr0 = fadd fast float %32, %10
  br label %loop

loop:                                             ; preds = %x_body, %loop
  %33 = phi i32 [ %42, %loop ], [ 0, %x_body ]
  %34 = phi float [ %41, %loop ], [ 0.000000e+00, %x_body ]
  %35 = phi float [ %tmp, %loop ], [ 0.000000e+00, %x_body ]
  %36 = fmul fast float %35, %35
  %37 = fmul fast float %34, %34
  %38 = fsub fast float %36, %37
  %tmp = fadd fast float %zr0, %38
  %39 = fmul fast float %34, 2.000000e+00
  %40 = fmul fast float %39, %35
  %41 = fadd fast float %zi0, %40
  %42 = add nuw nsw i32 %33, 1
  %43 = fmul fast float %tmp, %tmp
  %44 = fmul fast float %41, %41
  %45 = fadd fast float %43, %44
  %notlhs = icmp sge i32 %42, %18
  %notrhs = fcmp uge float %45, 4.000000e+00
  %46 = or i1 %notlhs, %notrhs
  br i1 %46, label %exit, label %loop

exit:                                             ; preds = %loop
  %47 = mul nuw nsw i32 %42, 255
  %48 = sdiv i32 %47, %18
  %49 = trunc i32 %48 to i8
  %50 = add nuw nsw i64 %x, %29
  %51 = getelementptr %u8XY, %u8XY* %4, i64 0, i32 6, i64 %50
  store i8 %49, i8* %51, align 1, !llvm.mem.parallel_loop_access !1
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

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

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
  %dst = bitcast %u0CXYT* %28 to %u8XY*
  %29 = zext i32 %arg_1 to i64
  %30 = alloca { %u8XY*, i32, i32, float, float, float, float, i32 }, align 8
  %31 = bitcast { %u8XY*, i32, i32, float, float, float, float, i32 }* %30 to %u0CXYT**
  store %u0CXYT* %28, %u0CXYT** %31, align 8
  %32 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %30, i64 0, i32 1
  store i32 %arg_0, i32* %32, align 8
  %33 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %30, i64 0, i32 2
  store i32 %arg_1, i32* %33, align 4
  %34 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %30, i64 0, i32 3
  store float %arg_2, float* %34, align 8
  %35 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %30, i64 0, i32 4
  store float %arg_3, float* %35, align 4
  %36 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %30, i64 0, i32 5
  store float %arg_4, float* %36, align 8
  %37 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %30, i64 0, i32 6
  store float %arg_5, float* %37, align 4
  %38 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %30, i64 0, i32 7
  store i32 %arg_6, i32* %38, align 8
  %39 = bitcast { %u8XY*, i32, i32, float, float, float, float, i32 }* %30 to i8*
  call void @likely_fork(i8* bitcast (void ({ %u8XY*, i32, i32, float, float, float, float, i32 }*, i64, i64)* @likely_test_function_tmp_thunk0 to i8*), i8* %39, i64 %29)
  ret %u8XY* %dst
}

attributes #0 = { nounwind argmemonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
