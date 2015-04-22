; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: nounwind readnone
declare float @llvm.sqrt.f32(float) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
define private void @normalize_l2_tmp_thunk0({ %u8SCXY*, %u8SCXY*, i32 }* noalias nocapture readonly, i64, i64) #2 {
entry:
  %3 = getelementptr inbounds { %u8SCXY*, %u8SCXY*, i32 }, { %u8SCXY*, %u8SCXY*, i32 }* %0, i64 0, i32 0
  %4 = load %u8SCXY*, %u8SCXY** %3, align 8
  %5 = getelementptr inbounds { %u8SCXY*, %u8SCXY*, i32 }, { %u8SCXY*, %u8SCXY*, i32 }* %0, i64 0, i32 1
  %6 = load %u8SCXY*, %u8SCXY** %5, align 8
  %7 = getelementptr inbounds { %u8SCXY*, %u8SCXY*, i32 }, { %u8SCXY*, %u8SCXY*, i32 }* %0, i64 0, i32 2
  %8 = load i32, i32* %7, align 4
  %9 = getelementptr inbounds %u8SCXY, %u8SCXY* %4, i64 0, i32 2
  %channels = load i32, i32* %9, align 4, !range !0
  %dst_c = zext i32 %channels to i64
  %10 = getelementptr inbounds %u8SCXY, %u8SCXY* %4, i64 0, i32 3
  %columns = load i32, i32* %10, align 4, !range !0
  %dst_x = zext i32 %columns to i64
  %11 = getelementptr inbounds %u8SCXY, %u8SCXY* %4, i64 0, i32 6, i64 0
  %12 = ptrtoint i8* %11 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  tail call void @llvm.assume(i1 %14)
  %15 = getelementptr inbounds %u8SCXY, %u8SCXY* %6, i64 0, i32 6, i64 0
  %16 = ptrtoint i8* %15 to i64
  %17 = and i64 %16, 31
  %18 = icmp eq i64 %17, 0
  tail call void @llvm.assume(i1 %18)
  %19 = mul nuw nsw i64 %dst_x, %dst_c
  %20 = mul nuw nsw i64 %19, %1
  %21 = mul nuw nsw i64 %19, %2
  %22 = lshr i32 %8, 31
  %23 = add nuw i32 %22, 2147483647
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ %20, %entry ], [ %y_increment, %y_body ]
  %24 = getelementptr %u8SCXY, %u8SCXY* %6, i64 0, i32 6, i64 %y
  %25 = load i8, i8* %24, align 1, !llvm.mem.parallel_loop_access !1
  %26 = zext i8 %25 to i32
  %27 = tail call { i32, i1 } @llvm.smul.with.overflow.i32(i32 %26, i32 %8)
  %28 = extractvalue { i32, i1 } %27, 1
  %29 = extractvalue { i32, i1 } %27, 0
  %30 = select i1 %28, i32 %23, i32 %29
  %31 = trunc i32 %30 to i8
  %32 = icmp slt i32 %30, 0
  %33 = select i1 %32, i8 0, i8 %31
  %34 = icmp sgt i32 %30, 255
  %35 = select i1 %34, i8 -1, i8 %33
  %36 = getelementptr %u8SCXY, %u8SCXY* %4, i64 0, i32 6, i64 %y
  store i8 %35, i8* %36, align 1, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %21
  br i1 %y_postcondition, label %y_exit, label %y_body, !llvm.loop !1

y_exit:                                           ; preds = %y_body
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #2

; Function Attrs: nounwind readnone
declare { i32, i1 } @llvm.smul.with.overflow.i32(i32, i32) #0

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %u8SCXY* @normalize_l2(%u8SCXY*) {
entry:
  %1 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 4
  %2 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 3
  %3 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 2
  br label %then

then:                                             ; preds = %entry, %end3
  %storemerge30 = phi i32 [ 0, %entry ], [ %23, %end3 ]
  %4 = phi i32 [ 0, %entry ], [ %42, %end3 ]
  %rows = load i32, i32* %1, align 4, !range !0
  %5 = sext i32 %storemerge30 to i64
  %6 = zext i32 %rows to i64
  %7 = mul nsw i64 %6, %5
  br label %then2

end:                                              ; preds = %end3
  %8 = sitofp i32 %42 to float
  %9 = tail call float @llvm.sqrt.f32(float %8)
  %10 = fdiv float 1.000000e+00, %9
  %11 = fptosi float %10 to i32
  %channels13 = load i32, i32* %3, align 4, !range !0
  %columns14 = load i32, i32* %2, align 4, !range !0
  %rows15 = load i32, i32* %1, align 4, !range !0
  %12 = tail call %u0CXYT* @likely_new(i32 29704, i32 %channels13, i32 %columns14, i32 %rows15, i32 1, i8* null)
  %13 = bitcast %u0CXYT* %12 to %u8SCXY*
  %14 = zext i32 %rows15 to i64
  %15 = alloca { %u8SCXY*, %u8SCXY*, i32 }, align 8
  %16 = bitcast { %u8SCXY*, %u8SCXY*, i32 }* %15 to %u0CXYT**
  store %u0CXYT* %12, %u0CXYT** %16, align 8
  %17 = getelementptr inbounds { %u8SCXY*, %u8SCXY*, i32 }, { %u8SCXY*, %u8SCXY*, i32 }* %15, i64 0, i32 1
  store %u8SCXY* %0, %u8SCXY** %17, align 8
  %18 = getelementptr inbounds { %u8SCXY*, %u8SCXY*, i32 }, { %u8SCXY*, %u8SCXY*, i32 }* %15, i64 0, i32 2
  store i32 %11, i32* %18, align 8
  %19 = bitcast { %u8SCXY*, %u8SCXY*, i32 }* %15 to i8*
  call void @likely_fork(i8* bitcast (void ({ %u8SCXY*, %u8SCXY*, i32 }*, i64, i64)* @normalize_l2_tmp_thunk0 to i8*), i8* %19, i64 %14)
  ret %u8SCXY* %13

then2:                                            ; preds = %then, %end6
  %storemerge129 = phi i32 [ 0, %then ], [ %30, %end6 ]
  %20 = phi i32 [ %4, %then ], [ %42, %end6 ]
  %columns = load i32, i32* %2, align 4, !range !0
  %21 = sext i32 %storemerge129 to i64
  %22 = zext i32 %columns to i64
  %tmp = add i64 %7, %21
  br label %then5

end3:                                             ; preds = %end6
  %23 = add nuw nsw i32 %storemerge30, 1
  %24 = icmp eq i32 %storemerge30, 0
  br i1 %24, label %end, label %then

then5:                                            ; preds = %then2, %end9
  %storemerge228 = phi i32 [ 0, %then2 ], [ %45, %end9 ]
  %25 = phi i32 [ %20, %then2 ], [ %42, %end9 ]
  %channels = load i32, i32* %3, align 4, !range !0
  %26 = sext i32 %storemerge228 to i64
  %27 = zext i32 %channels to i64
  %28 = mul nuw nsw i64 %22, %27
  %29 = mul nsw i64 %27, %26
  %tmp8 = mul i64 %28, %tmp
  br label %then8

end6:                                             ; preds = %end9
  %30 = add nuw nsw i32 %storemerge129, 1
  %31 = icmp eq i32 %30, %rows
  br i1 %31, label %end3, label %then2

then8:                                            ; preds = %then5, %then8
  %storemerge327 = phi i32 [ 0, %then5 ], [ %43, %then8 ]
  %32 = phi i32 [ %25, %then5 ], [ %42, %then8 ]
  %33 = sext i32 %storemerge327 to i64
  %34 = add i64 %29, %33
  %35 = add i64 %34, %tmp8
  %36 = getelementptr %u8SCXY, %u8SCXY* %0, i64 0, i32 6, i64 %35
  %37 = load i8, i8* %36, align 1
  %38 = uitofp i8 %37 to float
  %39 = fmul float %38, %38
  %40 = sitofp i32 %32 to float
  %41 = fadd float %40, %39
  %42 = fptosi float %41 to i32
  %43 = add nuw nsw i32 %storemerge327, 1
  %44 = icmp eq i32 %43, %channels
  br i1 %44, label %end9, label %then8

end9:                                             ; preds = %then8
  %45 = add nuw nsw i32 %storemerge228, 1
  %46 = icmp eq i32 %45, %columns
  br i1 %46, label %end6, label %then5
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
