; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
define private void @gemm_tmp_thunk0({ %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }* noalias nocapture readonly, i64, i64) #0 {
entry:
  %3 = getelementptr inbounds { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }* %0, i64 0, i32 0
  %4 = load %f32XY*, %f32XY** %3, align 8
  %5 = getelementptr inbounds { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }* %0, i64 0, i32 1
  %6 = load %f64XY*, %f64XY** %5, align 8
  %7 = getelementptr inbounds { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }* %0, i64 0, i32 2
  %8 = load %f64XY*, %f64XY** %7, align 8
  %9 = getelementptr inbounds { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }* %0, i64 0, i32 3
  %10 = load double, double* %9, align 8
  %11 = getelementptr inbounds { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }* %0, i64 0, i32 4
  %12 = load %f32XY*, %f32XY** %11, align 8
  %13 = getelementptr inbounds { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }* %0, i64 0, i32 5
  %14 = load double, double* %13, align 8
  %15 = getelementptr inbounds { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }* %0, i64 0, i32 6
  %16 = load i32, i32* %15, align 4
  %17 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 3
  %columns = load i32, i32* %17, align 4, !range !0
  %dst_y_step = zext i32 %columns to i64
  %18 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 6, i64 0
  %19 = ptrtoint float* %18 to i64
  %20 = and i64 %19, 31
  %21 = icmp eq i64 %20, 0
  call void @llvm.assume(i1 %21)
  %22 = getelementptr inbounds %f64XY, %f64XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %22, align 4, !range !0
  %src1_y_step = zext i32 %columns1 to i64
  %23 = getelementptr inbounds %f64XY, %f64XY* %6, i64 0, i32 6, i64 0
  %24 = ptrtoint double* %23 to i64
  %25 = and i64 %24, 31
  %26 = icmp eq i64 %25, 0
  call void @llvm.assume(i1 %26)
  %27 = getelementptr inbounds %f64XY, %f64XY* %8, i64 0, i32 3
  %columns3 = load i32, i32* %27, align 4, !range !0
  %src2_y_step = zext i32 %columns3 to i64
  %28 = getelementptr inbounds %f64XY, %f64XY* %8, i64 0, i32 6, i64 0
  %29 = ptrtoint double* %28 to i64
  %30 = and i64 %29, 31
  %31 = icmp eq i64 %30, 0
  call void @llvm.assume(i1 %31)
  %32 = getelementptr inbounds %f32XY, %f32XY* %12, i64 0, i32 3
  %columns5 = load i32, i32* %32, align 4, !range !0
  %src3_y_step = zext i32 %columns5 to i64
  %33 = getelementptr inbounds %f32XY, %f32XY* %12, i64 0, i32 6, i64 0
  %34 = ptrtoint float* %33 to i64
  %35 = and i64 %34, 31
  %36 = icmp eq i64 %35, 0
  call void @llvm.assume(i1 %36)
  %37 = icmp eq i32 %16, 0
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %38 = mul nuw nsw i64 %y, %dst_y_step
  %39 = mul nuw nsw i64 %y, %src3_y_step
  %40 = mul nuw nsw i64 %y, %src1_y_step
  br label %x_body

x_body:                                           ; preds = %end, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %end ]
  %41 = add nuw nsw i64 %x, %38
  %42 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %41
  br i1 %37, label %end, label %then

then:                                             ; preds = %x_body, %then
  %43 = phi i32 [ %57, %then ], [ 0, %x_body ]
  %44 = phi float [ %56, %then ], [ 0.000000e+00, %x_body ]
  %45 = sext i32 %43 to i64
  %46 = add nuw nsw i64 %45, %40
  %47 = getelementptr %f64XY, %f64XY* %6, i64 0, i32 6, i64 %46
  %48 = load double, double* %47, align 8, !llvm.mem.parallel_loop_access !1
  %49 = mul nuw nsw i64 %45, %src2_y_step
  %50 = add nuw nsw i64 %49, %x
  %51 = getelementptr %f64XY, %f64XY* %8, i64 0, i32 6, i64 %50
  %52 = load double, double* %51, align 8, !llvm.mem.parallel_loop_access !1
  %53 = fmul double %48, %52
  %54 = fpext float %44 to double
  %55 = fadd double %54, %53
  %56 = fptrunc double %55 to float
  %57 = add nuw nsw i32 %43, 1
  %58 = icmp eq i32 %57, %16
  br i1 %58, label %end, label %then

end:                                              ; preds = %then, %x_body
  %.lcssa = phi float [ 0.000000e+00, %x_body ], [ %56, %then ]
  %59 = fpext float %.lcssa to double
  %60 = fmul double %10, %59
  %61 = fptrunc double %60 to float
  %62 = add nuw nsw i64 %x, %39
  %63 = getelementptr %f32XY, %f32XY* %12, i64 0, i32 6, i64 %62
  %64 = load float, float* %63, align 4, !llvm.mem.parallel_loop_access !1
  %65 = fpext float %64 to double
  %66 = fmul double %14, %65
  %67 = fptrunc double %66 to float
  %68 = fadd float %61, %67
  store float %68, float* %42, align 4, !llvm.mem.parallel_loop_access !1
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

define %f32XY* @gemm(%f64XY*, %f64XY*, double, %f32XY*, double) {
entry:
  %5 = getelementptr inbounds %f64XY, %f64XY* %1, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 3
  %columns = load i32, i32* %6, align 4, !range !0
  %7 = icmp eq i32 %rows, %columns
  call void @llvm.assume(i1 %7)
  %8 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 4
  %rows1 = load i32, i32* %8, align 4, !range !0
  %9 = getelementptr inbounds %f32XY, %f32XY* %3, i64 0, i32 4
  %rows2 = load i32, i32* %9, align 4, !range !0
  %10 = icmp eq i32 %rows1, %rows2
  call void @llvm.assume(i1 %10)
  %11 = getelementptr inbounds %f64XY, %f64XY* %1, i64 0, i32 3
  %columns3 = load i32, i32* %11, align 4, !range !0
  %12 = getelementptr inbounds %f32XY, %f32XY* %3, i64 0, i32 3
  %columns4 = load i32, i32* %12, align 4, !range !0
  %13 = icmp eq i32 %columns3, %columns4
  call void @llvm.assume(i1 %13)
  %14 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns4, i32 %rows2, i32 1, i8* null)
  %15 = bitcast %u0CXYT* %14 to %f32XY*
  %16 = zext i32 %rows2 to i64
  %17 = alloca { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }, align 8
  %18 = bitcast { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }* %17 to %u0CXYT**
  store %u0CXYT* %14, %u0CXYT** %18, align 8
  %19 = getelementptr inbounds { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }* %17, i64 0, i32 1
  store %f64XY* %0, %f64XY** %19, align 8
  %20 = getelementptr inbounds { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }* %17, i64 0, i32 2
  store %f64XY* %1, %f64XY** %20, align 8
  %21 = getelementptr inbounds { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }* %17, i64 0, i32 3
  store double %2, double* %21, align 8
  %22 = getelementptr inbounds { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }* %17, i64 0, i32 4
  store %f32XY* %3, %f32XY** %22, align 8
  %23 = getelementptr inbounds { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }* %17, i64 0, i32 5
  store double %4, double* %23, align 8
  %24 = getelementptr inbounds { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }, { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }* %17, i64 0, i32 6
  store i32 %columns, i32* %24, align 8
  %25 = bitcast { %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }* %17 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32XY*, %f64XY*, %f64XY*, double, %f32XY*, double, i32 }*, i64, i64)* @gemm_tmp_thunk0 to i8*), i8* %25, i64 %16)
  ret %f32XY* %15
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
