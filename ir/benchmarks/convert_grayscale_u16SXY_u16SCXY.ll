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
  %1 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 2
  %2 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = call %u0Matrix* @likely_new(i32 25616, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %5 = zext i32 %rows to i64
  %dst_y_step = zext i32 %columns to i64
  %6 = getelementptr inbounds %u0Matrix, %u0Matrix* %4, i64 1
  %7 = bitcast %u0Matrix* %6 to i16*
  %channels4 = load i32, i32* %1, align 4, !range !0
  %src_c = zext i32 %channels4 to i64
  %8 = mul nuw nsw i64 %5, %dst_y_step
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %9 = mul nuw nsw i64 %y, %src_c
  %10 = getelementptr %u16Matrix, %u16Matrix* %0, i64 0, i32 6, i64 %9
  %11 = load i16, i16* %10, align 2, !llvm.mem.parallel_loop_access !1
  %12 = add nuw nsw i64 %9, 1
  %13 = getelementptr %u16Matrix, %u16Matrix* %0, i64 0, i32 6, i64 %12
  %14 = load i16, i16* %13, align 2, !llvm.mem.parallel_loop_access !1
  %15 = add nuw nsw i64 %9, 2
  %16 = getelementptr %u16Matrix, %u16Matrix* %0, i64 0, i32 6, i64 %15
  %17 = load i16, i16* %16, align 2, !llvm.mem.parallel_loop_access !1
  %18 = zext i16 %11 to i32
  %19 = mul nuw nsw i32 %18, 1868
  %20 = zext i16 %14 to i32
  %21 = mul nuw nsw i32 %20, 9617
  %22 = zext i16 %17 to i32
  %23 = mul nuw nsw i32 %22, 4899
  %24 = add nuw nsw i32 %19, 8192
  %25 = add nuw nsw i32 %24, %21
  %26 = add nuw i32 %25, %23
  %27 = lshr i32 %26, 14
  %28 = getelementptr i16, i16* %7, i64 %y
  %29 = trunc i32 %27 to i16
  store i16 %29, i16* %28, align 2, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %8
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %dst = bitcast %u0Matrix* %4 to %u16Matrix*
  ret %u16Matrix* %dst
}

attributes #0 = { nounwind }
attributes #1 = { argmemonly nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
