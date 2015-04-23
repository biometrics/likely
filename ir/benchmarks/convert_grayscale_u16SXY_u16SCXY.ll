; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u16SXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }
%u16SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
define %u16SXY* @convert_grayscale(%u16SCXY*) #0 {
entry:
  %1 = getelementptr inbounds %u16SCXY, %u16SCXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = icmp eq i32 %channels, 3
  tail call void @llvm.assume(i1 %2)
  %3 = getelementptr inbounds %u16SCXY, %u16SCXY* %0, i64 0, i32 3
  %columns = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %u16SCXY, %u16SCXY* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !0
  %5 = tail call %u0CXYT* @likely_new(i32 25616, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %6 = zext i32 %rows to i64
  %dst_y_step = zext i32 %columns to i64
  %7 = getelementptr inbounds %u0CXYT, %u0CXYT* %5, i64 1
  %8 = bitcast %u0CXYT* %7 to i16*
  %9 = ptrtoint %u0CXYT* %7 to i64
  %10 = and i64 %9, 31
  %11 = icmp eq i64 %10, 0
  tail call void @llvm.assume(i1 %11)
  %src_c = zext i32 %channels to i64
  %12 = getelementptr inbounds %u16SCXY, %u16SCXY* %0, i64 1
  %13 = bitcast %u16SCXY* %12 to i16*
  %14 = ptrtoint %u16SCXY* %12 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  tail call void @llvm.assume(i1 %16)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %17 = mul i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %x_body, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %x_body ]
  %tmp = add i64 %x, %17
  %tmp1 = mul i64 %tmp, %src_c
  %18 = getelementptr i16, i16* %13, i64 %tmp1
  %19 = load i16, i16* %18, align 2, !llvm.mem.parallel_loop_access !1
  %20 = add nuw nsw i64 %tmp1, 1
  %21 = getelementptr i16, i16* %13, i64 %20
  %22 = load i16, i16* %21, align 2, !llvm.mem.parallel_loop_access !1
  %23 = add nuw nsw i64 %tmp1, 2
  %24 = getelementptr i16, i16* %13, i64 %23
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
  %36 = trunc i32 %35 to i16
  %37 = getelementptr i16, i16* %8, i64 %tmp
  store i16 %36, i16* %37, align 2, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !1

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %6
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %38 = bitcast %u0CXYT* %5 to %u16SXY*
  ret %u16SXY* %38
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
