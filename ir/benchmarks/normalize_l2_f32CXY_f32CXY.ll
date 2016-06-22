; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readnone
declare double @llvm.sqrt.f64(double) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
define noalias %f32Matrix* @normalize_l2(%f32Matrix* noalias nocapture readonly) #2 {
entry:
  %1 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = mul nuw nsw i32 %columns, %channels
  %4 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !0
  %5 = mul nuw nsw i32 %3, %rows
  br label %true_entry

true_entry:                                       ; preds = %true_entry, %entry
  %6 = phi i32 [ 0, %entry ], [ %14, %true_entry ]
  %7 = phi double [ 0.000000e+00, %entry ], [ %13, %true_entry ]
  %8 = zext i32 %6 to i64
  %9 = getelementptr %f32Matrix, %f32Matrix* %0, i64 0, i32 6, i64 %8
  %10 = load float, float* %9, align 4
  %11 = fpext float %10 to double
  %12 = fmul fast double %11, %11
  %13 = fadd fast double %12, %7
  %14 = add nuw nsw i32 %6, 1
  %15 = icmp eq i32 %14, %5
  br i1 %15, label %exit, label %true_entry

exit:                                             ; preds = %true_entry
  %16 = call fast double @llvm.sqrt.f64(double %13)
  %17 = fdiv fast double 1.000000e+00, %16
  %norm4 = fptrunc double %17 to float
  %18 = call %u0Matrix* @likely_new(i32 28960, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %19 = zext i32 %rows to i64
  %dst_c = zext i32 %channels to i64
  %dst_x = zext i32 %columns to i64
  %20 = getelementptr inbounds %u0Matrix, %u0Matrix* %18, i64 1
  %21 = bitcast %u0Matrix* %20 to float*
  %22 = mul nuw nsw i64 %dst_x, %dst_c
  %23 = mul nuw nsw i64 %22, %19
  br label %y_body

y_body:                                           ; preds = %y_body, %exit
  %y = phi i64 [ 0, %exit ], [ %y_increment, %y_body ]
  %24 = getelementptr %f32Matrix, %f32Matrix* %0, i64 0, i32 6, i64 %y
  %25 = load float, float* %24, align 4, !llvm.mem.parallel_loop_access !1
  %26 = fmul fast float %25, %norm4
  %27 = getelementptr float, float* %21, i64 %y
  store float %26, float* %27, align 4, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %23
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %dst = bitcast %u0Matrix* %18 to %f32Matrix*
  ret %f32Matrix* %dst
}

attributes #0 = { nounwind readnone }
attributes #1 = { argmemonly nounwind }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
