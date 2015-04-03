; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8XY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%i32CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i32] }
%f32CXYT = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

; Function Attrs: nounwind
define %u8XY* @likely_test_function(%u0CXYT** nocapture readonly) #1 {
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
  %28 = tail call %u0CXYT* @likely_new(i32 24584, i32 1, i32 %arg_0, i32 %arg_1, i32 1, i8* null)
  %29 = zext i32 %arg_1 to i64
  %dst_y_step = zext i32 %arg_0 to i64
  %30 = getelementptr inbounds %u0CXYT, %u0CXYT* %28, i64 1
  %31 = bitcast %u0CXYT* %30 to i8*
  %32 = ptrtoint %u0CXYT* %30 to i64
  %33 = and i64 %32, 31
  %34 = icmp eq i64 %33, 0
  tail call void @llvm.assume(i1 %34)
  %35 = sitofp i32 %arg_0 to float
  %36 = sitofp i32 %arg_1 to float
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %y_offset = mul nuw nsw i64 %y, %dst_y_step
  %37 = uitofp i64 %y to float
  %38 = fmul float %arg_5, %37
  %39 = fdiv float %38, %36
  %40 = fadd float %arg_3, %39
  br label %x_body

x_body:                                           ; preds = %end, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %end ]
  %41 = uitofp i64 %x to float
  %42 = fmul float %arg_4, %41
  %43 = fdiv float %42, %35
  %44 = fadd float %arg_2, %43
  br label %label

label:                                            ; preds = %label, %x_body
  %45 = phi i32 [ %55, %label ], [ 0, %x_body ]
  %46 = phi float [ %54, %label ], [ 0.000000e+00, %x_body ]
  %47 = phi float [ %51, %label ], [ 0.000000e+00, %x_body ]
  %48 = fmul float %47, %47
  %49 = fmul float %46, %46
  %50 = fsub float %48, %49
  %51 = fadd float %44, %50
  %52 = fmul float %47, %46
  %53 = fmul float %52, 2.000000e+00
  %54 = fadd float %40, %53
  %55 = add nuw nsw i32 %45, 1
  %56 = fmul float %51, %51
  %57 = fmul float %54, %54
  %58 = fadd float %56, %57
  %59 = fcmp olt float %58, 4.000000e+00
  %60 = icmp slt i32 %55, %arg_6
  %61 = and i1 %60, %59
  br i1 %61, label %label, label %end

end:                                              ; preds = %label
  %x_offset = add nuw nsw i64 %x, %y_offset
  %62 = mul nuw nsw i32 %55, 255
  %63 = sdiv i32 %62, %arg_6
  %64 = trunc i32 %63 to i8
  %65 = getelementptr i8, i8* %31, i64 %x_offset
  store i8 %64, i8* %65, align 1, !llvm.mem.parallel_loop_access !0
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !0

x_exit:                                           ; preds = %end
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %29
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %66 = bitcast %u0CXYT* %28 to %u8XY*
  ret %u8XY* %66
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = distinct !{!0}
