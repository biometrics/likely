; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8SXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: nounwind
define %u8SXY* @convert_grayscale(%u8SCXY*) #0 {
entry:
  %1 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 4
  %rows = load i32, i32* %2, align 4, !range !0
  %3 = tail call %u0CXYT* @likely_new(i32 25608, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %4 = zext i32 %rows to i64
  %dst_y_step = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0CXYT, %u0CXYT* %3, i64 1
  %6 = bitcast %u0CXYT* %5 to i8*
  %7 = ptrtoint %u0CXYT* %5 to i64
  %8 = and i64 %7, 31
  %9 = icmp eq i64 %8, 0
  tail call void @llvm.assume(i1 %9)
  %10 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 2
  %channels = load i32, i32* %10, align 4, !range !0
  %src_c = zext i32 %channels to i64
  %11 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 1
  %12 = bitcast %u8SCXY* %11 to i8*
  %13 = ptrtoint %u8SCXY* %11 to i64
  %14 = and i64 %13, 31
  %15 = icmp eq i64 %14, 0
  tail call void @llvm.assume(i1 %15)
  %16 = mul nuw nsw i64 %4, %dst_y_step
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %17 = mul nuw nsw i64 %y, %src_c
  %18 = getelementptr i8, i8* %12, i64 %17
  %19 = load i8, i8* %18, align 1, !llvm.mem.parallel_loop_access !1
  %20 = add nuw nsw i64 %17, 1
  %21 = getelementptr i8, i8* %12, i64 %20
  %22 = load i8, i8* %21, align 1, !llvm.mem.parallel_loop_access !1
  %23 = add nuw nsw i64 %17, 2
  %24 = getelementptr i8, i8* %12, i64 %23
  %25 = load i8, i8* %24, align 1, !llvm.mem.parallel_loop_access !1
  %26 = zext i8 %19 to i32
  %27 = mul nuw nsw i32 %26, 1868
  %28 = zext i8 %22 to i32
  %29 = mul nuw nsw i32 %28, 9617
  %30 = zext i8 %25 to i32
  %31 = mul nuw nsw i32 %30, 4899
  %32 = add nuw nsw i32 %27, 8192
  %33 = add nuw nsw i32 %32, %29
  %34 = add nuw nsw i32 %33, %31
  %35 = lshr i32 %34, 14
  %36 = sitofp i32 %35 to float
  %37 = fadd float %36, 5.000000e-01
  %38 = fptoui float %37 to i8
  %39 = getelementptr i8, i8* %6, i64 %y
  store i8 %38, i8* %39, align 1, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %16
  br i1 %y_postcondition, label %y_exit, label %y_body, !llvm.loop !1

y_exit:                                           ; preds = %y_body
  %40 = bitcast %u0CXYT* %3 to %u8SXY*
  ret %u8SXY* %40
}

attributes #0 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
