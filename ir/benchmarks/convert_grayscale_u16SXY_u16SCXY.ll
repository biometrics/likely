; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u16SXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }
%u16SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: nounwind argmemonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

define %u16SXY* @convert_grayscale(%u16SCXY*) {
entry:
  %1 = getelementptr inbounds %u16SCXY, %u16SCXY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %u16SCXY, %u16SCXY* %0, i64 0, i32 4
  %rows = load i32, i32* %2, align 4, !range !0
  %3 = call %u0CXYT* @likely_new(i32 25616, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %4 = zext i32 %rows to i64
  %dst_y_step = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0CXYT, %u0CXYT* %3, i64 1
  %6 = bitcast %u0CXYT* %5 to i16*
  %7 = ptrtoint %u0CXYT* %5 to i64
  %8 = and i64 %7, 31
  %9 = icmp eq i64 %8, 0
  call void @llvm.assume(i1 %9)
  %10 = getelementptr inbounds %u16SCXY, %u16SCXY* %0, i64 0, i32 2
  %channels4 = load i32, i32* %10, align 4, !range !0
  %src_c = zext i32 %channels4 to i64
  %11 = getelementptr inbounds %u16SCXY, %u16SCXY* %0, i64 1
  %12 = bitcast %u16SCXY* %11 to i16*
  %13 = ptrtoint %u16SCXY* %11 to i64
  %14 = and i64 %13, 31
  %15 = icmp eq i64 %14, 0
  call void @llvm.assume(i1 %15)
  %16 = mul nuw nsw i64 %4, %dst_y_step
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %17 = mul nuw nsw i64 %y, %src_c
  %18 = getelementptr i16, i16* %12, i64 %17
  %19 = load i16, i16* %18, align 2, !llvm.mem.parallel_loop_access !1
  %20 = add nuw nsw i64 %17, 1
  %21 = getelementptr i16, i16* %12, i64 %20
  %22 = load i16, i16* %21, align 2, !llvm.mem.parallel_loop_access !1
  %23 = add nuw nsw i64 %17, 2
  %24 = getelementptr i16, i16* %12, i64 %23
  %25 = load i16, i16* %24, align 2, !llvm.mem.parallel_loop_access !1
  %26 = zext i16 %19 to i32
  %27 = mul nuw nsw i32 %26, 1868
  %28 = zext i16 %22 to i32
  %29 = mul nuw nsw i32 %28, 9617
  %30 = zext i16 %25 to i32
  %31 = mul nuw nsw i32 %30, 4899
  %32 = add nuw nsw i32 %27, 8192
  %33 = add nuw nsw i32 %32, %29
  %34 = add nuw i32 %33, %31
  %35 = lshr i32 %34, 14
  %36 = getelementptr i16, i16* %6, i64 %y
  %37 = trunc i32 %35 to i16
  store i16 %37, i16* %36, align 2, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %16
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %dst = bitcast %u0CXYT* %3 to %u16SXY*
  ret %u16SXY* %dst
}

attributes #0 = { nounwind }
attributes #1 = { nounwind argmemonly }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
