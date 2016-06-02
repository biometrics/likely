; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

define noalias %f32XY* @gemm(%f64XY* nocapture readonly, %f64XY* nocapture readonly, double, %f32XY* nocapture readonly, double) {
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
  %src1_y_step = zext i32 %columns to i64
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %18 = mul nuw nsw i64 %y, %dst_y_step
  %19 = mul nuw nsw i64 %y, %src1_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %exit
  %x = phi i64 [ %x_increment, %exit ], [ 0, %y_body ]
  %20 = add nuw nsw i64 %x, %18
  br label %true_entry

true_entry:                                       ; preds = %x_body, %true_entry
  %21 = phi i32 [ %33, %true_entry ], [ 0, %x_body ]
  %22 = phi double [ %32, %true_entry ], [ 0.000000e+00, %x_body ]
  %23 = sext i32 %21 to i64
  %24 = add nuw nsw i64 %23, %19
  %25 = getelementptr %f64XY, %f64XY* %0, i64 0, i32 6, i64 %24
  %26 = load double, double* %25, align 8, !llvm.mem.parallel_loop_access !1
  %27 = mul nuw nsw i64 %23, %dst_y_step
  %28 = add nuw nsw i64 %27, %x
  %29 = getelementptr %f64XY, %f64XY* %1, i64 0, i32 6, i64 %28
  %30 = load double, double* %29, align 8, !llvm.mem.parallel_loop_access !1
  %31 = fmul fast double %30, %26
  %32 = fadd fast double %31, %22
  %33 = add nuw nsw i32 %21, 1
  %34 = icmp eq i32 %33, %rows
  br i1 %34, label %exit, label %true_entry

exit:                                             ; preds = %true_entry
  %35 = getelementptr float, float* %17, i64 %20
  %36 = fmul fast double %32, %2
  %37 = fptrunc double %36 to float
  %38 = getelementptr %f32XY, %f32XY* %3, i64 0, i32 6, i64 %20
  %39 = load float, float* %38, align 4, !llvm.mem.parallel_loop_access !1
  %40 = fpext float %39 to double
  %41 = fmul fast double %40, %4
  %42 = fptrunc double %41 to float
  %43 = fadd fast float %42, %37
  store float %43, float* %35, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %15
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %dst = bitcast %u0CXYT* %14 to %f32XY*
  ret %f32XY* %dst
}

attributes #0 = { nounwind }
attributes #1 = { argmemonly nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
