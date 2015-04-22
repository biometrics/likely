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
  %1 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = mul nuw nsw i32 %columns, %channels
  %4 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !0
  %5 = mul nuw nsw i32 %3, %rows
  br label %then

then:                                             ; preds = %entry, %then
  %storemerge5 = phi i32 [ 0, %entry ], [ %13, %then ]
  %6 = phi i32 [ 0, %entry ], [ %12, %then ]
  %7 = sext i32 %storemerge5 to i64
  %8 = getelementptr %u8SCXY, %u8SCXY* %0, i64 0, i32 6, i64 %7
  %9 = load i8, i8* %8, align 1
  %10 = zext i8 %9 to i32
  %11 = mul nuw nsw i32 %10, %10
  %12 = add nuw nsw i32 %11, %6
  %13 = add nuw nsw i32 %storemerge5, 1
  %14 = icmp eq i32 %13, %5
  br i1 %14, label %end, label %then

end:                                              ; preds = %then
  %15 = sitofp i32 %12 to float
  %16 = tail call float @llvm.sqrt.f32(float %15)
  %17 = fdiv float 1.000000e+00, %16
  %18 = fptosi float %17 to i32
  %19 = tail call %u0CXYT* @likely_new(i32 29704, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %20 = bitcast %u0CXYT* %19 to %u8SCXY*
  %21 = zext i32 %rows to i64
  %22 = alloca { %u8SCXY*, %u8SCXY*, i32 }, align 8
  %23 = bitcast { %u8SCXY*, %u8SCXY*, i32 }* %22 to %u0CXYT**
  store %u0CXYT* %19, %u0CXYT** %23, align 8
  %24 = getelementptr inbounds { %u8SCXY*, %u8SCXY*, i32 }, { %u8SCXY*, %u8SCXY*, i32 }* %22, i64 0, i32 1
  store %u8SCXY* %0, %u8SCXY** %24, align 8
  %25 = getelementptr inbounds { %u8SCXY*, %u8SCXY*, i32 }, { %u8SCXY*, %u8SCXY*, i32 }* %22, i64 0, i32 2
  store i32 %18, i32* %25, align 8
  %26 = bitcast { %u8SCXY*, %u8SCXY*, i32 }* %22 to i8*
  call void @likely_fork(i8* bitcast (void ({ %u8SCXY*, %u8SCXY*, i32 }*, i64, i64)* @normalize_l2_tmp_thunk0 to i8*), i8* %26, i64 %21)
  ret %u8SCXY* %20
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
