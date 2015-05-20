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
  %7 = getelementptr inbounds %u8CXY, %u8CXY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %7, align 4, !range !0
  %dst_y_step = zext i32 %columns1 to i64
  %8 = getelementptr inbounds %u8SXY, %u8SXY* %4, i64 0, i32 6, i64 0
  %9 = ptrtoint i8* %8 to i64
  %10 = and i64 %9, 31
  %11 = icmp eq i64 %10, 0
  call void @llvm.assume(i1 %11)
  %12 = getelementptr inbounds %u8CXY, %u8CXY* %6, i64 0, i32 2
  %channels = load i32, i32* %12, align 4, !range !0
  %src_c = zext i32 %channels to i64
  %13 = getelementptr inbounds %u8CXY, %u8CXY* %6, i64 0, i32 6, i64 0
  %14 = ptrtoint i8* %13 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  call void @llvm.assume(i1 %16)
  %17 = mul nuw nsw i64 %dst_y_step, %2
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %y_body ]
  %18 = mul nuw nsw i64 %y, %src_c
  %19 = getelementptr %u8CXY, %u8CXY* %6, i64 0, i32 6, i64 %18
  %20 = load i8, i8* %19, align 1, !llvm.mem.parallel_loop_access !1
  %21 = add nuw nsw i64 %18, 1
  %22 = getelementptr %u8CXY, %u8CXY* %6, i64 0, i32 6, i64 %21
  %23 = load i8, i8* %22, align 1, !llvm.mem.parallel_loop_access !1
  %24 = add nuw nsw i64 %18, 2
  %25 = getelementptr %u8CXY, %u8CXY* %6, i64 0, i32 6, i64 %24
  %26 = load i8, i8* %25, align 1, !llvm.mem.parallel_loop_access !1
  %27 = zext i8 %20 to i32
  %28 = mul nuw nsw i32 %27, 1868
  %29 = zext i8 %23 to i32
  %30 = mul nuw nsw i32 %29, 9617
  %31 = zext i8 %26 to i32
  %32 = mul nuw nsw i32 %31, 4899
  %33 = add nuw nsw i32 %28, 8192
  %34 = add nuw nsw i32 %33, %30
  %35 = add nuw nsw i32 %34, %32
  %36 = lshr i32 %35, 14
  %37 = getelementptr %u8SXY, %u8SXY* %4, i64 0, i32 6, i64 %y
  %38 = trunc i32 %36 to i8
  store i8 %38, i8* %37, align 1, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %17
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  ret void
}

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %u8SXY* @convert_grayscale(%u8SCXY*) {
entry:
  %1 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = icmp eq i32 %channels, 3
  call void @llvm.assume(i1 %2)
  %3 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 3
  %columns = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !0
  %5 = call %u0CXYT* @likely_new(i32 25608, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %dst = bitcast %u0CXYT* %5 to %u8SXY*
  %6 = zext i32 %rows to i64
  %7 = alloca { %u8SXY*, %u8CXY* }, align 8
  %8 = bitcast { %u8SXY*, %u8CXY* }* %7 to %u0CXYT**
  store %u0CXYT* %5, %u0CXYT** %8, align 8
  %9 = getelementptr inbounds { %u8SXY*, %u8CXY* }, { %u8SXY*, %u8CXY* }* %7, i64 0, i32 1
  %10 = bitcast %u8CXY** %9 to %u8SCXY**
  store %u8SCXY* %0, %u8SCXY** %10, align 8
  %11 = bitcast { %u8SXY*, %u8CXY* }* %7 to i8*
  call void @likely_fork(i8* bitcast (void ({ %u8SXY*, %u8CXY* }*, i64, i64)* @convert_grayscale_tmp_thunk0 to i8*), i8* %11, i64 %6)
  ret %u8SXY* %dst
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
