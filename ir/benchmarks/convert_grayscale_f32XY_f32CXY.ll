; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
define noalias %f32Matrix* @convert_grayscale(%f32Matrix* noalias nocapture readonly) #0 {
entry:
  %1 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %2, align 4, !range !0
  %3 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %4 = zext i32 %rows to i64
  %dst_y_step = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0Matrix, %u0Matrix* %3, i64 1
  %6 = bitcast %u0Matrix* %5 to float*
  %7 = mul nuw nsw i64 %4, %dst_y_step
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %8 = mul nuw nsw i64 %y, 3
  %9 = getelementptr %f32Matrix, %f32Matrix* %0, i64 0, i32 6, i64 %8
  %10 = load float, float* %9, align 4, !llvm.mem.parallel_loop_access !1
  %11 = add nuw nsw i64 %8, 1
  %12 = getelementptr %f32Matrix, %f32Matrix* %0, i64 0, i32 6, i64 %11
  %13 = load float, float* %12, align 4, !llvm.mem.parallel_loop_access !1
  %14 = add nuw nsw i64 %8, 2
  %15 = getelementptr %f32Matrix, %f32Matrix* %0, i64 0, i32 6, i64 %14
  %16 = load float, float* %15, align 4, !llvm.mem.parallel_loop_access !1
  %17 = fmul fast float %10, 0x3FBD2F1AA0000000
  %18 = fmul fast float %13, 0x3FE2C8B440000000
  %19 = fadd fast float %18, %17
  %20 = fmul fast float %16, 0x3FD322D0E0000000
  %21 = fadd fast float %19, %20
  %22 = getelementptr float, float* %6, i64 %y
  store float %21, float* %22, align 4, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %7
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %dst = bitcast %u0Matrix* %3 to %f32Matrix*
  ret %f32Matrix* %dst
}

attributes #0 = { nounwind }
attributes #1 = { argmemonly nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
