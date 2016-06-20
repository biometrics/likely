; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u16Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

define noalias %u16Matrix* @convert_grayscale(%u16Matrix* nocapture readonly) {
entry:
  %1 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %2, align 4, !range !0
  %3 = call %u0Matrix* @likely_new(i32 25616, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %4 = zext i32 %rows to i64
  %dst_y_step = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0Matrix, %u0Matrix* %3, i64 1
  %6 = bitcast %u0Matrix* %5 to i16*
  %7 = mul nuw nsw i64 %4, %dst_y_step
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %8 = mul nuw nsw i64 %y, 3
  %9 = getelementptr %u16Matrix, %u16Matrix* %0, i64 0, i32 6, i64 %8
  %10 = load i16, i16* %9, align 2, !llvm.mem.parallel_loop_access !1
  %11 = add nuw nsw i64 %8, 1
  %12 = getelementptr %u16Matrix, %u16Matrix* %0, i64 0, i32 6, i64 %11
  %13 = load i16, i16* %12, align 2, !llvm.mem.parallel_loop_access !1
  %14 = add nuw nsw i64 %8, 2
  %15 = getelementptr %u16Matrix, %u16Matrix* %0, i64 0, i32 6, i64 %14
  %16 = load i16, i16* %15, align 2, !llvm.mem.parallel_loop_access !1
  %17 = zext i16 %10 to i32
  %18 = mul nuw nsw i32 %17, 1868
  %19 = zext i16 %13 to i32
  %20 = mul nuw nsw i32 %19, 9617
  %21 = zext i16 %16 to i32
  %22 = mul nuw nsw i32 %21, 4899
  %23 = add nuw nsw i32 %18, 8192
  %24 = add nuw nsw i32 %23, %20
  %25 = add nuw i32 %24, %22
  %26 = lshr i32 %25, 14
  %27 = getelementptr i16, i16* %6, i64 %y
  %28 = trunc i32 %26 to i16
  store i16 %28, i16* %27, align 2, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %7
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %dst = bitcast %u0Matrix* %3 to %u16Matrix*
  ret %u16Matrix* %dst
}

attributes #0 = { nounwind }
attributes #1 = { argmemonly nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
