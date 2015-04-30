; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8XY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%i32CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i32] }
%f32CXYT = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readonly
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
  %27 = fmul float %16, %26
  %28 = fdiv float %27, %25
  %29 = fadd float %12, %28
  %30 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %end, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %end ]
  %31 = uitofp i64 %x to float
  %32 = fmul float %14, %31
  %33 = fdiv float %32, %24
  %34 = fadd float %10, %33
  br label %label

label:                                            ; preds = %label, %x_body
  %35 = phi i32 [ %45, %label ], [ 0, %x_body ]
  %36 = phi float [ %44, %label ], [ 0.000000e+00, %x_body ]
  %37 = phi float [ %41, %label ], [ 0.000000e+00, %x_body ]
  %38 = fmul float %37, %37
  %39 = fmul float %36, %36
  %40 = fsub float %38, %39
  %41 = fadd float %34, %40
  %42 = fmul float %36, %37
  %43 = fmul float %42, 2.000000e+00
  %44 = fadd float %29, %43
  %45 = add nuw nsw i32 %35, 1
  %46 = fmul float %41, %41
  %47 = fmul float %44, %44
  %48 = fadd float %46, %47
  %49 = fcmp olt float %48, 4.000000e+00
  %50 = icmp slt i32 %45, %18
  %51 = and i1 %50, %49
  br i1 %51, label %label, label %end

end:                                              ; preds = %label
  %52 = mul nuw nsw i32 %45, 255
  %53 = sdiv i32 %52, %18
  %54 = trunc i32 %53 to i8
  %55 = add nuw nsw i64 %x, %30
  %56 = getelementptr %u8XY, %u8XY* %4, i64 0, i32 6, i64 %55
  store i8 %54, i8* %56, align 1, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !1

x_exit:                                           ; preds = %end
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
  %29 = bitcast %u0CXYT* %28 to %u8XY*
  %30 = zext i32 %arg_1 to i64
  %31 = alloca { %u8XY*, i32, i32, float, float, float, float, i32 }, align 8
  %32 = bitcast { %u8XY*, i32, i32, float, float, float, float, i32 }* %31 to %u0CXYT**
  store %u0CXYT* %28, %u0CXYT** %32, align 8
  %33 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %31, i64 0, i32 1
  store i32 %arg_0, i32* %33, align 8
  %34 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %31, i64 0, i32 2
  store i32 %arg_1, i32* %34, align 4
  %35 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %31, i64 0, i32 3
  store float %arg_2, float* %35, align 8
  %36 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %31, i64 0, i32 4
  store float %arg_3, float* %36, align 4
  %37 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %31, i64 0, i32 5
  store float %arg_4, float* %37, align 8
  %38 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %31, i64 0, i32 6
  store float %arg_5, float* %38, align 4
  %39 = getelementptr inbounds { %u8XY*, i32, i32, float, float, float, float, i32 }, { %u8XY*, i32, i32, float, float, float, float, i32 }* %31, i64 0, i32 7
  store i32 %arg_6, i32* %39, align 8
  %40 = bitcast { %u8XY*, i32, i32, float, float, float, float, i32 }* %31 to i8*
  call void @likely_fork(i8* bitcast (void ({ %u8XY*, i32, i32, float, float, float, float, i32 }*, i64, i64)* @likely_test_function_tmp_thunk0 to i8*), i8* %40, i64 %30)
  ret %u8XY* %29
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
