; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8SXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8CXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
define private void @convert_grayscale_tmp_thunk0({ %u8SXY*, %u8CXY* }* noalias nocapture readonly, i64, i64) #0 {
entry:
  %3 = getelementptr inbounds { %u8SXY*, %u8CXY* }, { %u8SXY*, %u8CXY* }* %0, i64 0, i32 0
  %4 = load %u8SXY*, %u8SXY** %3, align 8
  %5 = getelementptr inbounds { %u8SXY*, %u8CXY* }, { %u8SXY*, %u8CXY* }* %0, i64 0, i32 1
  %6 = load %u8CXY*, %u8CXY** %5, align 8
  %7 = getelementptr inbounds %u8SXY, %u8SXY* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !0
  %dst_y_step = zext i32 %columns to i64
  %8 = getelementptr inbounds %u8SXY, %u8SXY* %4, i64 0, i32 6, i64 0
  %9 = ptrtoint i8* %8 to i64
  %10 = and i64 %9, 31
  %11 = icmp eq i64 %10, 0
  tail call void @llvm.assume(i1 %11)
  %12 = getelementptr inbounds %u8CXY, %u8CXY* %6, i64 0, i32 2
  %channels = load i32, i32* %12, align 4, !range !0
  %src_c = zext i32 %channels to i64
  %13 = getelementptr inbounds %u8CXY, %u8CXY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %13, align 4, !range !0
  %src_x = zext i32 %columns1 to i64
  %14 = getelementptr inbounds %u8CXY, %u8CXY* %6, i64 0, i32 6, i64 0
  %15 = ptrtoint i8* %14 to i64
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
  %20 = getelementptr %u8CXY, %u8CXY* %6, i64 0, i32 6, i64 %tmp3
  %21 = load i8, i8* %20, align 1, !llvm.mem.parallel_loop_access !1
  %22 = add nuw nsw i64 %tmp3, 1
  %23 = getelementptr %u8CXY, %u8CXY* %6, i64 0, i32 6, i64 %22
  %24 = load i8, i8* %23, align 1, !llvm.mem.parallel_loop_access !1
  %25 = add nuw nsw i64 %tmp3, 2
  %26 = getelementptr %u8CXY, %u8CXY* %6, i64 0, i32 6, i64 %25
  %27 = load i8, i8* %26, align 1, !llvm.mem.parallel_loop_access !1
  %28 = zext i8 %21 to i32
  %29 = mul nuw nsw i32 %28, 1868
  %30 = zext i8 %24 to i32
  %31 = mul nuw nsw i32 %30, 9617
  %32 = zext i8 %27 to i32
  %33 = mul nuw nsw i32 %32, 4899
  %34 = add nuw nsw i32 %29, 8192
  %35 = add nuw nsw i32 %34, %31
  %36 = add nuw nsw i32 %35, %33
  %37 = lshr i32 %36, 14
  %38 = trunc i32 %37 to i8
  %39 = add nuw nsw i64 %x, %19
  %40 = getelementptr %u8SXY, %u8SXY* %4, i64 0, i32 6, i64 %39
  store i8 %38, i8* %40, align 1, !llvm.mem.parallel_loop_access !1
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

define %u8SXY* @convert_grayscale(%u8SCXY*) {
entry:
  %1 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = icmp eq i32 %channels, 3
  tail call void @llvm.assume(i1 %2)
  %3 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 3
  %columns = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !0
  %5 = tail call %u0CXYT* @likely_new(i32 25608, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %6 = bitcast %u0CXYT* %5 to %u8SXY*
  %7 = zext i32 %rows to i64
  %8 = alloca { %u8SXY*, %u8CXY* }, align 8
  %9 = bitcast { %u8SXY*, %u8CXY* }* %8 to %u0CXYT**
  store %u0CXYT* %5, %u0CXYT** %9, align 8
  %10 = getelementptr inbounds { %u8SXY*, %u8CXY* }, { %u8SXY*, %u8CXY* }* %8, i64 0, i32 1
  %11 = bitcast %u8CXY** %10 to %u8SCXY**
  store %u8SCXY* %0, %u8SCXY** %11, align 8
  %12 = bitcast { %u8SXY*, %u8CXY* }* %8 to i8*
  call void @likely_fork(i8* bitcast (void ({ %u8SXY*, %u8CXY* }*, i64, i64)* @convert_grayscale_tmp_thunk0 to i8*), i8* %12, i64 %7)
  ret %u8SXY* %6
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
