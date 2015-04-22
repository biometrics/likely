; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: nounwind readnone
declare float @llvm.sqrt.f32(float) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
declare void @llvm.assume(i1) #2

; Function Attrs: nounwind readnone
declare { i32, i1 } @llvm.smul.with.overflow.i32(i32, i32) #0

; Function Attrs: nounwind
define %u8SCXY* @normalize_l2(%u8SCXY*) #2 {
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
  %20 = zext i32 %rows to i64
  %dst_c = zext i32 %channels to i64
  %dst_x = zext i32 %columns to i64
  %21 = getelementptr inbounds %u0CXYT, %u0CXYT* %19, i64 1
  %22 = bitcast %u0CXYT* %21 to i8*
  %23 = ptrtoint %u0CXYT* %21 to i64
  %24 = and i64 %23, 31
  %25 = icmp eq i64 %24, 0
  tail call void @llvm.assume(i1 %25)
  %26 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 6, i64 0
  %27 = ptrtoint i8* %26 to i64
  %28 = and i64 %27, 31
  %29 = icmp eq i64 %28, 0
  tail call void @llvm.assume(i1 %29)
  %30 = mul nuw nsw i64 %dst_x, %dst_c
  %31 = mul nuw nsw i64 %30, %20
  %32 = lshr i32 %18, 31
  %33 = add nuw i32 %32, 2147483647
  br label %y_body

y_body:                                           ; preds = %y_body, %end
  %y = phi i64 [ 0, %end ], [ %y_increment, %y_body ]
  %34 = getelementptr %u8SCXY, %u8SCXY* %0, i64 0, i32 6, i64 %y
  %35 = load i8, i8* %34, align 1, !llvm.mem.parallel_loop_access !1
  %36 = zext i8 %35 to i32
  %37 = tail call { i32, i1 } @llvm.smul.with.overflow.i32(i32 %36, i32 %18)
  %38 = extractvalue { i32, i1 } %37, 1
  %39 = extractvalue { i32, i1 } %37, 0
  %40 = select i1 %38, i32 %33, i32 %39
  %41 = trunc i32 %40 to i8
  %42 = icmp slt i32 %40, 0
  %43 = select i1 %42, i8 0, i8 %41
  %44 = icmp sgt i32 %40, 255
  %45 = select i1 %44, i8 -1, i8 %43
  %46 = getelementptr i8, i8* %22, i64 %y
  store i8 %45, i8* %46, align 1, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %31
  br i1 %y_postcondition, label %y_exit, label %y_body, !llvm.loop !1

y_exit:                                           ; preds = %y_body
  %47 = bitcast %u0CXYT* %19 to %u8SCXY*
  ret %u8SCXY* %47
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
