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
  %src_y_step = mul nuw nsw i64 %dst_y_step, %src_c
  %12 = getelementptr inbounds %u16SCXY, %u16SCXY* %0, i64 1
  %13 = bitcast %u16SCXY* %12 to i16*
  %14 = ptrtoint %u16SCXY* %12 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  tail call void @llvm.assume(i1 %16)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %17 = mul nuw nsw i64 %src_y_step, %y
  %18 = add i64 %17, 1
  %19 = add i64 %17, 2
  %20 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %x_body, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %x_body ]
  %21 = mul nuw nsw i64 %x, %src_c
  %22 = add nuw nsw i64 %21, %17
  %23 = getelementptr i16, i16* %13, i64 %22
  %24 = load i16, i16* %23, align 2, !llvm.mem.parallel_loop_access !1
  %25 = add i64 %18, %21
  %26 = getelementptr i16, i16* %13, i64 %25
  %27 = load i16, i16* %26, align 2, !llvm.mem.parallel_loop_access !1
  %28 = add i64 %19, %21
  %29 = getelementptr i16, i16* %13, i64 %28
  %30 = load i16, i16* %29, align 2, !llvm.mem.parallel_loop_access !1
  %31 = zext i16 %24 to i32
  %32 = mul nuw nsw i32 %31, 1868
  %33 = zext i16 %27 to i32
  %34 = mul nuw nsw i32 %33, 9617
  %35 = zext i16 %30 to i32
  %36 = mul nuw nsw i32 %35, 4899
  %37 = add nuw nsw i32 %32, 8192
  %38 = add nuw nsw i32 %37, %34
  %39 = add nuw i32 %38, %36
  %40 = lshr i32 %39, 14
  %41 = trunc i32 %40 to i16
  %42 = add nuw nsw i64 %x, %20
  %43 = getelementptr i16, i16* %8, i64 %42
  store i16 %41, i16* %43, align 2, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !1

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %6
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %44 = bitcast %u0CXYT* %5 to %u16SXY*
  ret %u16SXY* %44
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
