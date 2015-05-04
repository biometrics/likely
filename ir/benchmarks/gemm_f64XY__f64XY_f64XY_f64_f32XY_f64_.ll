; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

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
  %15 = zext i32 %rows2 to i64
  %dst_y_step = zext i32 %columns4 to i64
  %16 = getelementptr inbounds %u0CXYT, %u0CXYT* %14, i64 1
  %17 = bitcast %u0CXYT* %16 to float*
  %18 = ptrtoint %u0CXYT* %16 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  %src1_y_step = zext i32 %columns to i64
  %21 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 6, i64 0
  %22 = ptrtoint double* %21 to i64
  %23 = and i64 %22, 31
  %24 = icmp eq i64 %23, 0
  call void @llvm.assume(i1 %24)
  %25 = getelementptr inbounds %f64XY, %f64XY* %1, i64 0, i32 6, i64 0
  %26 = ptrtoint double* %25 to i64
  %27 = and i64 %26, 31
  %28 = icmp eq i64 %27, 0
  call void @llvm.assume(i1 %28)
  %29 = getelementptr inbounds %f32XY, %f32XY* %3, i64 0, i32 6, i64 0
  %30 = ptrtoint float* %29 to i64
  %31 = and i64 %30, 31
  %32 = icmp eq i64 %31, 0
  call void @llvm.assume(i1 %32)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %33 = mul nuw nsw i64 %y, %dst_y_step
  %34 = mul nuw nsw i64 %y, %src1_y_step
  br label %x_body

x_body:                                           ; preds = %end, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %end ]
  %35 = add nuw nsw i64 %x, %33
  br label %then

then:                                             ; preds = %x_body, %then
  %36 = phi i32 [ 0, %x_body ], [ %50, %then ]
  %37 = phi float [ 0.000000e+00, %x_body ], [ %49, %then ]
  %38 = sext i32 %36 to i64
  %39 = add nuw nsw i64 %38, %34
  %40 = getelementptr %f64XY, %f64XY* %0, i64 0, i32 6, i64 %39
  %41 = load double, double* %40, align 8, !llvm.mem.parallel_loop_access !1
  %42 = mul nuw nsw i64 %38, %dst_y_step
  %43 = add nuw nsw i64 %42, %x
  %44 = getelementptr %f64XY, %f64XY* %1, i64 0, i32 6, i64 %43
  %45 = load double, double* %44, align 8, !llvm.mem.parallel_loop_access !1
  %46 = fmul double %41, %45
  %47 = fpext float %37 to double
  %48 = fadd double %47, %46
  %49 = fptrunc double %48 to float
  %50 = add nuw nsw i32 %36, 1
  %51 = icmp eq i32 %50, %columns
  br i1 %51, label %end, label %then

end:                                              ; preds = %then
  %52 = getelementptr float, float* %17, i64 %35
  %53 = fpext float %49 to double
  %54 = fmul double %2, %53
  %55 = fptrunc double %54 to float
  %56 = getelementptr %f32XY, %f32XY* %3, i64 0, i32 6, i64 %35
  %57 = load float, float* %56, align 4, !llvm.mem.parallel_loop_access !1
  %58 = fpext float %57 to double
  %59 = fmul double %4, %58
  %60 = fptrunc double %59 to float
  %61 = fadd float %55, %60
  store float %61, float* %52, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !1

x_exit:                                           ; preds = %end
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %15
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %62 = bitcast %u0CXYT* %14 to %f32XY*
  ret %f32XY* %62
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
