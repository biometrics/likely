; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i32] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define noalias %u32Matrix* @transpose(%u32Matrix* noalias nocapture readonly) #1 {
entry:
  %1 = getelementptr inbounds %u32Matrix, %u32Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %u32Matrix, %u32Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = call %u0Matrix* @likely_new(i32 25120, i32 1, i32 %rows, i32 %columns, i32 1, i8* null)
  %4 = zext i32 %columns to i64
  %dst_y_step = zext i32 %rows to i64
  %5 = getelementptr inbounds %u0Matrix, %u0Matrix* %3, i64 1, i32 0
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %6 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %7 = mul nuw nsw i64 %x, %4
  %8 = add nuw nsw i64 %7, %y
  %9 = getelementptr %u32Matrix, %u32Matrix* %0, i64 0, i32 6, i64 %8
  %10 = load i32, i32* %9, align 4, !llvm.mem.parallel_loop_access !1
  %11 = add nuw nsw i64 %x, %6
  %12 = getelementptr i32, i32* %5, i64 %11
  store i32 %10, i32* %12, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %4
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %dst = bitcast %u0Matrix* %3 to %u32Matrix*
  ret %u32Matrix* %dst
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
