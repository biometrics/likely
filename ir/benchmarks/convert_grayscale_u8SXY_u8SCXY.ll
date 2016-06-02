; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8SXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

define noalias %u8SXY* @convert_grayscale(%u8SCXY* nocapture readonly) {
entry:
  %1 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 4
  %rows = load i32, i32* %2, align 4, !range !0
  %3 = call %u0CXYT* @likely_new(i32 25608, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %4 = zext i32 %rows to i64
  %dst_y_step = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0CXYT, %u0CXYT* %3, i64 1
  %6 = bitcast %u0CXYT* %5 to i8*
  %7 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 2
  %channels4 = load i32, i32* %7, align 4, !range !0
  %src_c = zext i32 %channels4 to i64
  %8 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 1
  %9 = bitcast %u8SCXY* %8 to i8*
  %10 = mul nuw nsw i64 %4, %dst_y_step
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %11 = mul nuw nsw i64 %y, %src_c
  %12 = getelementptr i8, i8* %9, i64 %11
  %13 = load i8, i8* %12, align 1, !llvm.mem.parallel_loop_access !1
  %14 = add nuw nsw i64 %11, 1
  %15 = getelementptr i8, i8* %9, i64 %14
  %16 = load i8, i8* %15, align 1, !llvm.mem.parallel_loop_access !1
  %17 = add nuw nsw i64 %11, 2
  %18 = getelementptr i8, i8* %9, i64 %17
  %19 = load i8, i8* %18, align 1, !llvm.mem.parallel_loop_access !1
  %20 = zext i8 %13 to i32
  %21 = mul nuw nsw i32 %20, 1868
  %22 = zext i8 %16 to i32
  %23 = mul nuw nsw i32 %22, 9617
  %24 = zext i8 %19 to i32
  %25 = mul nuw nsw i32 %24, 4899
  %26 = add nuw nsw i32 %21, 8192
  %27 = add nuw nsw i32 %26, %23
  %28 = add nuw nsw i32 %27, %25
  %29 = lshr i32 %28, 14
  %30 = getelementptr i8, i8* %6, i64 %y
  %31 = trunc i32 %29 to i8
  store i8 %31, i8* %30, align 1, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %10
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %dst = bitcast %u0CXYT* %3 to %u8SXY*
  ret %u8SXY* %dst
}

attributes #0 = { nounwind }
attributes #1 = { argmemonly nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
