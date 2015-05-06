; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
define private void @gemm_tmp_thunk0({ %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }* noalias nocapture readonly, i64, i64) #0 {
entry:
  %3 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }* %0, i64 0, i32 0
  %4 = load %f32XY*, %f32XY** %3, align 8
  %5 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }* %0, i64 0, i32 1
  %6 = load %f32XY*, %f32XY** %5, align 8
  %7 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }* %0, i64 0, i32 2
  %8 = load %f32XY*, %f32XY** %7, align 8
  %9 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }* %0, i64 0, i32 3
  %10 = load double, double* %9, align 8
  %11 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }* %0, i64 0, i32 4
  %12 = load %f32XY*, %f32XY** %11, align 8
  %13 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }* %0, i64 0, i32 5
  %14 = load double, double* %13, align 8
  %15 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }* %0, i64 0, i32 6
  %16 = load i32, i32* %15, align 4
  %17 = getelementptr inbounds %f32XY, %f32XY* %12, i64 0, i32 3
  %columns5 = load i32, i32* %17, align 4, !range !0
  %dst_y_step = zext i32 %columns5 to i64
  %18 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 6, i64 0
  %19 = ptrtoint float* %18 to i64
  %20 = and i64 %19, 31
  %21 = icmp eq i64 %20, 0
  call void @llvm.assume(i1 %21)
  %22 = getelementptr inbounds %f32XY, %f32XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %22, align 4, !range !0
  %src1_y_step = zext i32 %columns1 to i64
  %23 = getelementptr inbounds %f32XY, %f32XY* %6, i64 0, i32 6, i64 0
  %24 = ptrtoint float* %23 to i64
  %25 = and i64 %24, 31
  %26 = icmp eq i64 %25, 0
  call void @llvm.assume(i1 %26)
  %27 = getelementptr inbounds %f32XY, %f32XY* %8, i64 0, i32 3
  %columns3 = load i32, i32* %27, align 4, !range !0
  %src2_y_step = zext i32 %columns3 to i64
  %28 = getelementptr inbounds %f32XY, %f32XY* %8, i64 0, i32 6, i64 0
  %29 = ptrtoint float* %28 to i64
  %30 = and i64 %29, 31
  %31 = icmp eq i64 %30, 0
  call void @llvm.assume(i1 %31)
  %32 = getelementptr inbounds %f32XY, %f32XY* %12, i64 0, i32 6, i64 0
  %33 = ptrtoint float* %32 to i64
  %34 = and i64 %33, 31
  %35 = icmp eq i64 %34, 0
  call void @llvm.assume(i1 %35)
  %36 = icmp eq i32 %16, 0
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %37 = mul nuw nsw i64 %y, %dst_y_step
  %38 = mul nuw nsw i64 %y, %src1_y_step
  br label %x_body

x_body:                                           ; preds = %end, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %end ]
  %39 = add nuw nsw i64 %x, %37
  %40 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %39
  br i1 %36, label %end, label %then

then:                                             ; preds = %x_body, %then
  %41 = phi i32 [ %53, %then ], [ 0, %x_body ]
  %42 = phi float [ %52, %then ], [ 0.000000e+00, %x_body ]
  %43 = sext i32 %41 to i64
  %44 = add nuw nsw i64 %43, %38
  %45 = getelementptr %f32XY, %f32XY* %6, i64 0, i32 6, i64 %44
  %46 = load float, float* %45, align 4, !llvm.mem.parallel_loop_access !1
  %47 = mul nuw nsw i64 %43, %src2_y_step
  %48 = add nuw nsw i64 %47, %x
  %49 = getelementptr %f32XY, %f32XY* %8, i64 0, i32 6, i64 %48
  %50 = load float, float* %49, align 4, !llvm.mem.parallel_loop_access !1
  %51 = fmul float %46, %50
  %52 = fadd float %42, %51
  %53 = add nuw nsw i32 %41, 1
  %54 = icmp eq i32 %53, %16
  br i1 %54, label %end, label %then

end:                                              ; preds = %then, %x_body
  %.lcssa = phi float [ 0.000000e+00, %x_body ], [ %52, %then ]
  %55 = fpext float %.lcssa to double
  %56 = fmul double %10, %55
  %57 = fptrunc double %56 to float
  %58 = getelementptr %f32XY, %f32XY* %12, i64 0, i32 6, i64 %39
  %59 = load float, float* %58, align 4, !llvm.mem.parallel_loop_access !1
  %60 = fpext float %59 to double
  %61 = fmul double %14, %60
  %62 = fptrunc double %61 to float
  %63 = fadd float %57, %62
  store float %63, float* %40, align 4, !llvm.mem.parallel_loop_access !1
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

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %f32XY* @gemm(%f32XY*, %f32XY*, double, %f32XY*, double) {
entry:
  %5 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %6, align 4, !range !0
  %7 = icmp eq i32 %rows, %columns
  call void @llvm.assume(i1 %7)
  %8 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 4
  %rows1 = load i32, i32* %8, align 4, !range !0
  %9 = getelementptr inbounds %f32XY, %f32XY* %3, i64 0, i32 4
  %rows2 = load i32, i32* %9, align 4, !range !0
  %10 = icmp eq i32 %rows1, %rows2
  call void @llvm.assume(i1 %10)
  %11 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 3
  %columns3 = load i32, i32* %11, align 4, !range !0
  %12 = getelementptr inbounds %f32XY, %f32XY* %3, i64 0, i32 3
  %columns4 = load i32, i32* %12, align 4, !range !0
  %13 = icmp eq i32 %columns3, %columns4
  call void @llvm.assume(i1 %13)
  %14 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns4, i32 %rows2, i32 1, i8* null)
  %15 = bitcast %u0CXYT* %14 to %f32XY*
  %16 = zext i32 %rows2 to i64
  %17 = alloca { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }, align 8
  %18 = bitcast { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }* %17 to %u0CXYT**
  store %u0CXYT* %14, %u0CXYT** %18, align 8
  %19 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }* %17, i64 0, i32 1
  store %f32XY* %0, %f32XY** %19, align 8
  %20 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }* %17, i64 0, i32 2
  store %f32XY* %1, %f32XY** %20, align 8
  %21 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }* %17, i64 0, i32 3
  store double %2, double* %21, align 8
  %22 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }* %17, i64 0, i32 4
  store %f32XY* %3, %f32XY** %22, align 8
  %23 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }* %17, i64 0, i32 5
  store double %4, double* %23, align 8
  %24 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }* %17, i64 0, i32 6
  store i32 %columns, i32* %24, align 8
  %25 = bitcast { %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }* %17 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32XY*, %f32XY*, %f32XY*, double, %f32XY*, double, i32 }*, i64, i64)* @gemm_tmp_thunk0 to i8*), i8* %25, i64 %16)
  ret %f32XY* %15
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
