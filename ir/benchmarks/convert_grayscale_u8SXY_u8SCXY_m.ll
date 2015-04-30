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
  call void @llvm.assume(i1 %11)
  %12 = getelementptr %u8CXY, %u8CXY* %6, i64 0, i32 2
  %13 = bitcast i32* %12 to i64*
  %channels.combined = load i64, i64* %13, align 4
  %src_c = and i64 %channels.combined, 4294967295
  %combine.extract.shift = lshr i64 %channels.combined, 32
  %src_y_step = mul nuw nsw i64 %combine.extract.shift, %src_c
  %14 = getelementptr inbounds %u8CXY, %u8CXY* %6, i64 0, i32 6, i64 0
  %15 = ptrtoint i8* %14 to i64
  %16 = and i64 %15, 31
  %17 = icmp eq i64 %16, 0
  call void @llvm.assume(i1 %17)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %18 = mul nuw nsw i64 %src_y_step, %y
  %19 = add i64 %18, 1
  %20 = add i64 %18, 2
  %21 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %x_body, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %x_body ]
  %22 = mul nuw nsw i64 %x, %src_c
  %23 = add nuw nsw i64 %22, %18
  %24 = getelementptr %u8CXY, %u8CXY* %6, i64 0, i32 6, i64 %23
  %25 = load i8, i8* %24, align 1, !llvm.mem.parallel_loop_access !1
  %26 = add i64 %19, %22
  %27 = getelementptr %u8CXY, %u8CXY* %6, i64 0, i32 6, i64 %26
  %28 = load i8, i8* %27, align 1, !llvm.mem.parallel_loop_access !1
  %29 = add i64 %20, %22
  %30 = getelementptr %u8CXY, %u8CXY* %6, i64 0, i32 6, i64 %29
  %31 = load i8, i8* %30, align 1, !llvm.mem.parallel_loop_access !1
  %32 = zext i8 %25 to i32
  %33 = mul nuw nsw i32 %32, 1868
  %34 = zext i8 %28 to i32
  %35 = mul nuw nsw i32 %34, 9617
  %36 = zext i8 %31 to i32
  %37 = mul nuw nsw i32 %36, 4899
  %38 = add nuw nsw i32 %33, 8192
  %39 = add nuw nsw i32 %38, %35
  %40 = add nuw nsw i32 %39, %37
  %41 = lshr i32 %40, 14
  %42 = trunc i32 %41 to i8
  %43 = add nuw nsw i64 %x, %21
  %44 = getelementptr %u8SXY, %u8SXY* %4, i64 0, i32 6, i64 %43
  store i8 %42, i8* %44, align 1, !llvm.mem.parallel_loop_access !1
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
  %1 = getelementptr %u8SCXY, %u8SCXY* %0, i64 0, i32 2
  %2 = bitcast i32* %1 to i64*
  %channels.combined = load i64, i64* %2, align 4
  %combine.extract.trunc = trunc i64 %channels.combined to i32
  %3 = icmp eq i32 %combine.extract.trunc, 3
  call void @llvm.assume(i1 %3)
  %combine.extract.shift = lshr i64 %channels.combined, 32
  %combine.extract.trunc1 = trunc i64 %combine.extract.shift to i32
  %4 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !0
  %5 = call %u0CXYT* @likely_new(i32 25608, i32 1, i32 %combine.extract.trunc1, i32 %rows, i32 1, i8* null)
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
