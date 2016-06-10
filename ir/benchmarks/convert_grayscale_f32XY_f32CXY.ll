; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

define noalias %f32Matrix* @convert_grayscale(%f32Matrix* nocapture readonly) {
entry:
  %1 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 2
  %2 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %5 = zext i32 %rows to i64
  %dst_y_step = zext i32 %columns to i64
  %6 = getelementptr inbounds %u0Matrix, %u0Matrix* %4, i64 1
  %7 = bitcast %u0Matrix* %6 to float*
  %channels4 = load i32, i32* %1, align 4, !range !0
  %src_c = zext i32 %channels4 to i64
  %8 = mul nuw nsw i64 %5, %dst_y_step
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %9 = mul nuw nsw i64 %y, %src_c
  %10 = getelementptr %f32Matrix, %f32Matrix* %0, i64 0, i32 6, i64 %9
  %11 = load float, float* %10, align 4, !llvm.mem.parallel_loop_access !1
  %12 = add nuw nsw i64 %9, 1
  %13 = getelementptr %f32Matrix, %f32Matrix* %0, i64 0, i32 6, i64 %12
  %14 = load float, float* %13, align 4, !llvm.mem.parallel_loop_access !1
  %15 = add nuw nsw i64 %9, 2
  %16 = getelementptr %f32Matrix, %f32Matrix* %0, i64 0, i32 6, i64 %15
  %17 = load float, float* %16, align 4, !llvm.mem.parallel_loop_access !1
  %18 = fmul fast float %11, 0x3FBD2F1AA0000000
  %19 = fmul fast float %14, 0x3FE2C8B440000000
  %20 = fadd fast float %19, %18
  %21 = fmul fast float %17, 0x3FD322D0E0000000
  %22 = fadd fast float %20, %21
  %23 = getelementptr float, float* %7, i64 %y
  store float %22, float* %23, align 4, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %8
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %dst = bitcast %u0Matrix* %4 to %f32Matrix*
  ret %f32Matrix* %dst
}

attributes #0 = { nounwind }
attributes #1 = { argmemonly nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
