; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

define noalias %f32Matrix* @gemm(%f32Matrix* nocapture readonly, %f32Matrix* nocapture readonly, double, %f32Matrix* nocapture readonly, double) {
entry:
  %5 = getelementptr inbounds %f32Matrix, %f32Matrix* %1, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %6, align 4, !range !0
  %7 = icmp eq i32 %rows, %columns
  call void @llvm.assume(i1 %7)
  %8 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 4
  %rows1 = load i32, i32* %8, align 4, !range !0
  %9 = getelementptr inbounds %f32Matrix, %f32Matrix* %3, i64 0, i32 4
  %rows2 = load i32, i32* %9, align 4, !range !0
  %10 = icmp eq i32 %rows1, %rows2
  call void @llvm.assume(i1 %10)
  %11 = getelementptr inbounds %f32Matrix, %f32Matrix* %1, i64 0, i32 3
  %columns3 = load i32, i32* %11, align 4, !range !0
  %12 = getelementptr inbounds %f32Matrix, %f32Matrix* %3, i64 0, i32 3
  %columns4 = load i32, i32* %12, align 4, !range !0
  %13 = icmp eq i32 %columns3, %columns4
  call void @llvm.assume(i1 %13)
  %14 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns4, i32 %rows2, i32 1, i8* null)
  %15 = zext i32 %rows2 to i64
  %dst_y_step = zext i32 %columns4 to i64
  %16 = getelementptr inbounds %u0Matrix, %u0Matrix* %14, i64 1
  %17 = bitcast %u0Matrix* %16 to float*
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
  %22 = phi float [ %32, %true_entry ], [ 0.000000e+00, %x_body ]
  %23 = sext i32 %21 to i64
  %24 = add nuw nsw i64 %23, %19
  %25 = getelementptr %f32Matrix, %f32Matrix* %0, i64 0, i32 6, i64 %24
  %26 = load float, float* %25, align 4, !llvm.mem.parallel_loop_access !1
  %27 = mul nuw nsw i64 %23, %dst_y_step
  %28 = add nuw nsw i64 %27, %x
  %29 = getelementptr %f32Matrix, %f32Matrix* %1, i64 0, i32 6, i64 %28
  %30 = load float, float* %29, align 4, !llvm.mem.parallel_loop_access !1
  %31 = fmul fast float %30, %26
  %32 = fadd fast float %31, %22
  %33 = add nuw nsw i32 %21, 1
  %34 = icmp eq i32 %33, %rows
  br i1 %34, label %exit, label %true_entry

exit:                                             ; preds = %true_entry
  %35 = getelementptr float, float* %17, i64 %20
  %36 = fpext float %32 to double
  %37 = fmul fast double %36, %2
  %38 = fptrunc double %37 to float
  %39 = getelementptr %f32Matrix, %f32Matrix* %3, i64 0, i32 6, i64 %20
  %40 = load float, float* %39, align 4, !llvm.mem.parallel_loop_access !1
  %41 = fpext float %40 to double
  %42 = fmul fast double %41, %4
  %43 = fptrunc double %42 to float
  %44 = fadd fast float %43, %38
  store float %44, float* %35, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %15
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %dst = bitcast %u0Matrix* %14 to %f32Matrix*
  ret %f32Matrix* %dst
}

attributes #0 = { nounwind }
attributes #1 = { argmemonly nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
