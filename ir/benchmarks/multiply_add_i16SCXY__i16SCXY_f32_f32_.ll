; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u16Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define noalias %u16Matrix* @multiply_add(%u16Matrix* noalias nocapture readonly, float, float) #1 {
entry:
  br label %entry.split

entry.split:                                      ; preds = %entry
  %3 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 2
  %channels = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %4, align 4, !range !0
  %5 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = call %u0Matrix* @likely_new(i32 30224, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %7 = zext i32 %rows to i64
  %dst_c = zext i32 %channels to i64
  %dst_x = zext i32 %columns to i64
  %8 = getelementptr inbounds %u0Matrix, %u0Matrix* %6, i64 1
  %9 = bitcast %u0Matrix* %8 to i16*
  %10 = mul nuw nsw i64 %dst_x, %dst_c
  %11 = mul nuw nsw i64 %10, %7
  br label %y_body

y_body:                                           ; preds = %y_body, %entry.split
  %y = phi i64 [ 0, %entry.split ], [ %y_increment, %y_body ]
  %12 = getelementptr %u16Matrix, %u16Matrix* %0, i64 0, i32 6, i64 %y
  %13 = load i16, i16* %12, align 2, !llvm.mem.parallel_loop_access !1
  %14 = sitofp i16 %13 to float
  %15 = fmul fast float %14, %1
  %val = fadd fast float %15, %2
  %16 = getelementptr i16, i16* %9, i64 %y
  %17 = fcmp fast olt float %val, 0.000000e+00
  %. = select i1 %17, float -5.000000e-01, float 5.000000e-01
  %18 = fadd fast float %., %val
  %19 = fcmp fast ogt float %18, -3.276800e+04
  %20 = select i1 %19, float %18, float -3.276800e+04
  %21 = fptosi float %20 to i16
  %22 = fcmp fast ogt float %18, 3.276700e+04
  %23 = select i1 %22, i16 32767, i16 %21
  store i16 %23, i16* %16, align 2, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %11
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %dst = bitcast %u0Matrix* %6 to %u16Matrix*
  ret %u16Matrix* %dst
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
