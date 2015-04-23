; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u16SXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }
%u16CXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }
%u16SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
define private void @convert_grayscale_tmp_thunk0({ %u16SXY*, %u16CXY* }* noalias nocapture readonly, i64, i64) #0 {
entry:
  %3 = getelementptr inbounds { %u16SXY*, %u16CXY* }, { %u16SXY*, %u16CXY* }* %0, i64 0, i32 0
  %4 = load %u16SXY*, %u16SXY** %3, align 8
  %5 = getelementptr inbounds { %u16SXY*, %u16CXY* }, { %u16SXY*, %u16CXY* }* %0, i64 0, i32 1
  %6 = load %u16CXY*, %u16CXY** %5, align 8
  %7 = getelementptr inbounds %u16SXY, %u16SXY* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !0
  %dst_y_step = zext i32 %columns to i64
  %8 = getelementptr inbounds %u16SXY, %u16SXY* %4, i64 0, i32 6, i64 0
  %9 = ptrtoint i16* %8 to i64
  %10 = and i64 %9, 31
  %11 = icmp eq i64 %10, 0
  tail call void @llvm.assume(i1 %11)
  %12 = getelementptr inbounds %u16CXY, %u16CXY* %6, i64 0, i32 2
  %channels = load i32, i32* %12, align 4, !range !0
  %src_c = zext i32 %channels to i64
  %13 = getelementptr inbounds %u16CXY, %u16CXY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %13, align 4, !range !0
  %src_x = zext i32 %columns1 to i64
  %14 = getelementptr inbounds %u16CXY, %u16CXY* %6, i64 0, i32 6, i64 0
  %15 = ptrtoint i16* %14 to i64
  %16 = and i64 %15, 31
  %17 = icmp eq i64 %16, 0
  tail call void @llvm.assume(i1 %17)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %18 = mul i64 %y, %src_x
  %19 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %x_body, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %x_body ]
  %tmp = add i64 %x, %18
  %tmp3 = mul i64 %tmp, %src_c
  %20 = getelementptr %u16CXY, %u16CXY* %6, i64 0, i32 6, i64 %tmp3
  %21 = load i16, i16* %20, align 2, !llvm.mem.parallel_loop_access !1
  %22 = add nuw nsw i64 %tmp3, 1
  %23 = getelementptr %u16CXY, %u16CXY* %6, i64 0, i32 6, i64 %22
  %24 = load i16, i16* %23, align 2, !llvm.mem.parallel_loop_access !1
  %25 = add nuw nsw i64 %tmp3, 2
  %26 = getelementptr %u16CXY, %u16CXY* %6, i64 0, i32 6, i64 %25
  %27 = load i16, i16* %26, align 2, !llvm.mem.parallel_loop_access !1
  %28 = zext i16 %21 to i32
  %29 = mul nuw nsw i32 %28, 1868
  %30 = zext i16 %24 to i32
  %31 = mul nuw nsw i32 %30, 9617
  %32 = zext i16 %27 to i32
  %33 = mul nuw nsw i32 %32, 4899
  %34 = add nuw nsw i32 %29, 8192
  %35 = add nuw nsw i32 %34, %31
  %36 = add nuw i32 %35, %33
  %37 = lshr i32 %36, 14
  %38 = trunc i32 %37 to i16
  %39 = add nuw nsw i64 %x, %19
  %40 = getelementptr %u16SXY, %u16SXY* %4, i64 0, i32 6, i64 %39
  store i16 %38, i16* %40, align 2, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !1

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %u16SXY* @convert_grayscale(%u16SCXY*) {
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
  %6 = bitcast %u0CXYT* %5 to %u16SXY*
  %7 = zext i32 %rows to i64
  %8 = alloca { %u16SXY*, %u16CXY* }, align 8
  %9 = bitcast { %u16SXY*, %u16CXY* }* %8 to %u0CXYT**
  store %u0CXYT* %5, %u0CXYT** %9, align 8
  %10 = getelementptr inbounds { %u16SXY*, %u16CXY* }, { %u16SXY*, %u16CXY* }* %8, i64 0, i32 1
  %11 = bitcast %u16CXY** %10 to %u16SCXY**
  store %u16SCXY* %0, %u16SCXY** %11, align 8
  %12 = bitcast { %u16SXY*, %u16CXY* }* %8 to i8*
  call void @likely_fork(i8* bitcast (void ({ %u16SXY*, %u16CXY* }*, i64, i64)* @convert_grayscale_tmp_thunk0 to i8*), i8* %12, i64 %7)
  ret %u16SXY* %6
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
