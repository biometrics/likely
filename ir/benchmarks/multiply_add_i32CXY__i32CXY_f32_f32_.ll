; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i32] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define noalias %u32Matrix* @multiply_add(%u32Matrix* noalias nocapture readonly, float, float) #1 {
entry:
  %3 = getelementptr inbounds %u32Matrix, %u32Matrix* %0, i64 0, i32 2
  %channels = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %u32Matrix, %u32Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %4, align 4, !range !0
  %5 = getelementptr inbounds %u32Matrix, %u32Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = call %u0Matrix* @likely_new(i32 29216, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %7 = zext i32 %rows to i64
  %dst_c = zext i32 %channels to i64
  %dst_x = zext i32 %columns to i64
  %8 = getelementptr inbounds %u0Matrix, %u0Matrix* %6, i64 1, i32 0
  %9 = mul nuw nsw i64 %dst_x, %dst_c
  %10 = mul nuw nsw i64 %9, %7
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %11 = getelementptr %u32Matrix, %u32Matrix* %0, i64 0, i32 6, i64 %y
  %12 = load i32, i32* %11, align 4, !llvm.mem.parallel_loop_access !1
  %13 = sitofp i32 %12 to float
  %14 = fmul fast float %13, %1
  %val = fadd fast float %14, %2
  %15 = getelementptr i32, i32* %8, i64 %y
  %16 = fcmp fast olt float %val, 0.000000e+00
  %. = select i1 %16, float -5.000000e-01, float 5.000000e-01
  %17 = fadd fast float %., %val
  %18 = fptosi float %17 to i32
  store i32 %18, i32* %15, align 4, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %10
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %dst = bitcast %u0Matrix* %6 to %u32Matrix*
  ret %u32Matrix* %dst
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
