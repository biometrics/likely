; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: norecurse nounwind
define private void @gemm_tmp_thunk0({ %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }* noalias nocapture readonly, i64, i64) #2 {
entry:
  %3 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }* %0, i64 0, i32 0
  %4 = load %f32Matrix*, %f32Matrix** %3, align 8
  %5 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }* %0, i64 0, i32 1
  %6 = load %f32Matrix*, %f32Matrix** %5, align 8
  %7 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }* %0, i64 0, i32 2
  %8 = load %f32Matrix*, %f32Matrix** %7, align 8
  %9 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }* %0, i64 0, i32 3
  %10 = load double, double* %9, align 8
  %11 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }* %0, i64 0, i32 4
  %12 = load %f32Matrix*, %f32Matrix** %11, align 8
  %13 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }* %0, i64 0, i32 5
  %14 = load double, double* %13, align 8
  %15 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }* %0, i64 0, i32 6
  %16 = load i32, i32* %15, align 4
  %17 = getelementptr inbounds %f32Matrix, %f32Matrix* %12, i64 0, i32 3
  %columns5 = load i32, i32* %17, align 4, !range !0
  %dst_y_step = zext i32 %columns5 to i64
  %18 = getelementptr inbounds %f32Matrix, %f32Matrix* %6, i64 0, i32 3
  %columns1 = load i32, i32* %18, align 4, !range !0
  %src1_y_step = zext i32 %columns1 to i64
  %19 = getelementptr inbounds %f32Matrix, %f32Matrix* %8, i64 0, i32 3
  %columns3 = load i32, i32* %19, align 4, !range !0
  %src2_y_step = zext i32 %columns3 to i64
  %20 = icmp eq i32 %16, 0
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %21 = mul nuw nsw i64 %y, %dst_y_step
  %22 = mul nuw nsw i64 %y, %src1_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %exit
  %x = phi i64 [ %x_increment, %exit ], [ 0, %y_body ]
  %23 = add nuw nsw i64 %x, %21
  %24 = getelementptr %f32Matrix, %f32Matrix* %4, i64 0, i32 6, i64 %23
  br i1 %20, label %exit, label %true_entry

true_entry:                                       ; preds = %x_body, %true_entry
  %25 = phi i32 [ %37, %true_entry ], [ 0, %x_body ]
  %26 = phi float [ %36, %true_entry ], [ 0.000000e+00, %x_body ]
  %27 = sext i32 %25 to i64
  %28 = add nuw nsw i64 %27, %22
  %29 = getelementptr %f32Matrix, %f32Matrix* %6, i64 0, i32 6, i64 %28
  %30 = load float, float* %29, align 4, !llvm.mem.parallel_loop_access !1
  %31 = mul nuw nsw i64 %27, %src2_y_step
  %32 = add nuw nsw i64 %31, %x
  %33 = getelementptr %f32Matrix, %f32Matrix* %8, i64 0, i32 6, i64 %32
  %34 = load float, float* %33, align 4, !llvm.mem.parallel_loop_access !1
  %35 = fmul fast float %34, %30
  %36 = fadd fast float %35, %26
  %37 = add nuw nsw i32 %25, 1
  %38 = icmp eq i32 %37, %16
  br i1 %38, label %exit, label %true_entry

exit:                                             ; preds = %true_entry, %x_body
  %.lcssa = phi float [ 0.000000e+00, %x_body ], [ %36, %true_entry ]
  %39 = fpext float %.lcssa to double
  %40 = fmul fast double %39, %10
  %41 = fptrunc double %40 to float
  %42 = getelementptr %f32Matrix, %f32Matrix* %12, i64 0, i32 6, i64 %23
  %43 = load float, float* %42, align 4, !llvm.mem.parallel_loop_access !1
  %44 = fpext float %43 to double
  %45 = fmul fast double %44, %14
  %46 = fptrunc double %45 to float
  %47 = fadd fast float %46, %41
  store float %47, float* %24, align 4, !llvm.mem.parallel_loop_access !1
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

; Function Attrs: nounwind
define noalias %f32Matrix* @gemm(%f32Matrix* noalias nocapture, %f32Matrix* noalias nocapture, double, %f32Matrix* noalias nocapture, double) #0 {
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
  %dst = bitcast %u0Matrix* %14 to %f32Matrix*
  %15 = zext i32 %rows2 to i64
  %16 = alloca { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }, align 8
  %17 = bitcast { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }* %16 to %u0Matrix**
  store %u0Matrix* %14, %u0Matrix** %17, align 8
  %18 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }* %16, i64 0, i32 1
  store %f32Matrix* %0, %f32Matrix** %18, align 8
  %19 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }* %16, i64 0, i32 2
  store %f32Matrix* %1, %f32Matrix** %19, align 8
  %20 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }* %16, i64 0, i32 3
  store double %2, double* %20, align 8
  %21 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }* %16, i64 0, i32 4
  store %f32Matrix* %3, %f32Matrix** %21, align 8
  %22 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }* %16, i64 0, i32 5
  store double %4, double* %22, align 8
  %23 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }* %16, i64 0, i32 6
  store i32 %columns, i32* %23, align 8
  %24 = bitcast { %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }* %16 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, %f32Matrix*, %f32Matrix*, double, %f32Matrix*, double, i32 }*, i64, i64)* @gemm_tmp_thunk0 to i8*), i8* %24, i64 %15) #0
  ret %f32Matrix* %dst
}

attributes #0 = { nounwind }
attributes #1 = { argmemonly nounwind }
attributes #2 = { norecurse nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
