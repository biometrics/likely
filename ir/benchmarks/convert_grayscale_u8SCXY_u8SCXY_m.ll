; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8SXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8CXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

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
  %13 = getelementptr inbounds %u8CXY, %u8CXY* %6, i64 0, i32 6, i64 0
  %14 = ptrtoint i8* %13 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  tail call void @llvm.assume(i1 %16)
  %17 = mul nuw nsw i64 %dst_y_step, %1
  %18 = mul nuw nsw i64 %dst_y_step, %2
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ %17, %entry ], [ %y_increment, %y_body ]
  %19 = mul nuw nsw i64 %y, %src_c
  %20 = getelementptr %u8CXY, %u8CXY* %6, i64 0, i32 6, i64 %19
  %21 = load i8, i8* %20, align 1, !llvm.mem.parallel_loop_access !1
  %22 = add nuw nsw i64 %19, 1
  %23 = getelementptr %u8CXY, %u8CXY* %6, i64 0, i32 6, i64 %22
  %24 = load i8, i8* %23, align 1, !llvm.mem.parallel_loop_access !1
  %25 = add nuw nsw i64 %19, 2
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
  %38 = sitofp i32 %37 to float
  %39 = fadd float %38, 5.000000e-01
  %40 = fptoui float %39 to i8
  %41 = getelementptr %u8SXY, %u8SXY* %4, i64 0, i32 6, i64 %y
  store i8 %40, i8* %41, align 1, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %18
  br i1 %y_postcondition, label %y_exit, label %y_body, !llvm.loop !1

y_exit:                                           ; preds = %y_body
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %u8SXY* @convert_grayscale(%u8SCXY*) {
entry:
  %1 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 4
  %rows = load i32, i32* %2, align 4, !range !0
  %3 = tail call %u0CXYT* @likely_new(i32 25608, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %4 = bitcast %u0CXYT* %3 to %u8SXY*
  %5 = zext i32 %rows to i64
  %6 = alloca { %u8SXY*, %u8CXY* }, align 8
  %7 = bitcast { %u8SXY*, %u8CXY* }* %6 to %u0CXYT**
  store %u0CXYT* %3, %u0CXYT** %7, align 8
  %8 = getelementptr inbounds { %u8SXY*, %u8CXY* }, { %u8SXY*, %u8CXY* }* %6, i64 0, i32 1
  %9 = bitcast %u8CXY** %8 to %u8SCXY**
  store %u8SCXY* %0, %u8SCXY** %9, align 8
  %10 = bitcast { %u8SXY*, %u8CXY* }* %6 to i8*
  call void @likely_fork(i8* bitcast (void ({ %u8SXY*, %u8CXY* }*, i64, i64)* @convert_grayscale_tmp_thunk0 to i8*), i8* %10, i64 %5)
  ret %u8SXY* %4
}

attributes #0 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
