; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8SXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

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
  %6 = zext i32 %rows to i64
  %7 = getelementptr inbounds %u0CXYT, %u0CXYT* %5, i64 1
  %8 = bitcast %u0CXYT* %7 to i8*
  %9 = ptrtoint %u0CXYT* %7 to i64
  %10 = and i64 %9, 31
  %11 = icmp eq i64 %10, 0
  call void @llvm.assume(i1 %11)
  %12 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 1
  %13 = bitcast %u8SCXY* %12 to i8*
  %14 = ptrtoint %u8SCXY* %12 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  call void @llvm.assume(i1 %16)
  %17 = mul nuw nsw i64 %6, %combine.extract.shift
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %18 = mul nuw nsw i64 %y, 3
  %19 = getelementptr i8, i8* %13, i64 %18
  %20 = load i8, i8* %19, align 1, !llvm.mem.parallel_loop_access !1
  %21 = add nuw nsw i64 %18, 1
  %22 = getelementptr i8, i8* %13, i64 %21
  %23 = load i8, i8* %22, align 1, !llvm.mem.parallel_loop_access !1
  %24 = add nuw nsw i64 %18, 2
  %25 = getelementptr i8, i8* %13, i64 %24
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
  %37 = trunc i32 %36 to i8
  %38 = getelementptr i8, i8* %8, i64 %y
  store i8 %37, i8* %38, align 1, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %17
  br i1 %y_postcondition, label %y_exit, label %y_body, !llvm.loop !1

y_exit:                                           ; preds = %y_body
  %39 = bitcast %u0CXYT* %5 to %u8SXY*
  ret %u8SXY* %39
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
