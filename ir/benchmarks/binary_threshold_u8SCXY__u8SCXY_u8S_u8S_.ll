; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

define noalias %u8Matrix* @binary_threshold(%u8Matrix* nocapture readonly, i8, i8) {
entry:
  %3 = getelementptr inbounds %u8Matrix, %u8Matrix* %0, i64 0, i32 2
  %channels = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %u8Matrix, %u8Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %4, align 4, !range !0
  %5 = getelementptr inbounds %u8Matrix, %u8Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = call %u0Matrix* @likely_new(i32 29704, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %7 = zext i32 %rows to i64
  %dst_c = zext i32 %channels to i64
  %dst_x = zext i32 %columns to i64
  %8 = getelementptr inbounds %u0Matrix, %u0Matrix* %6, i64 1
  %9 = bitcast %u0Matrix* %8 to i8*
  %10 = mul nuw nsw i64 %dst_x, %dst_c
  %11 = mul nuw nsw i64 %10, %7
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %12 = getelementptr %u8Matrix, %u8Matrix* %0, i64 0, i32 6, i64 %y
  %13 = load i8, i8* %12, align 1, !llvm.mem.parallel_loop_access !1
  %14 = icmp ugt i8 %13, %1
  %. = select i1 %14, i8 %2, i8 0
  %15 = getelementptr i8, i8* %9, i64 %y
  store i8 %., i8* %15, align 1, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %11
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %dst = bitcast %u0Matrix* %6 to %u8Matrix*
  ret %u8Matrix* %dst
}

attributes #0 = { argmemonly nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
