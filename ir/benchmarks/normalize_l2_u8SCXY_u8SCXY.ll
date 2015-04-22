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
  %1 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 4
  %2 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 3
  %3 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 2
  br label %then

then:                                             ; preds = %entry, %end3
  %storemerge30 = phi i32 [ 0, %entry ], [ %30, %end3 ]
  %4 = phi i32 [ 0, %entry ], [ %49, %end3 ]
  %rows = load i32, i32* %1, align 4, !range !0
  %5 = sext i32 %storemerge30 to i64
  %6 = zext i32 %rows to i64
  %7 = mul nsw i64 %6, %5
  br label %then2

end:                                              ; preds = %end3
  %8 = sitofp i32 %49 to float
  %9 = tail call float @llvm.sqrt.f32(float %8)
  %10 = fdiv float 1.000000e+00, %9
  %11 = fptosi float %10 to i32
  %channels13 = load i32, i32* %3, align 4, !range !0
  %columns14 = load i32, i32* %2, align 4, !range !0
  %rows15 = load i32, i32* %1, align 4, !range !0
  %12 = tail call %u0CXYT* @likely_new(i32 29704, i32 %channels13, i32 %columns14, i32 %rows15, i32 1, i8* null)
  %13 = zext i32 %rows15 to i64
  %dst_c = zext i32 %channels13 to i64
  %dst_x = zext i32 %columns14 to i64
  %14 = getelementptr inbounds %u0CXYT, %u0CXYT* %12, i64 1
  %15 = bitcast %u0CXYT* %14 to i8*
  %16 = ptrtoint %u0CXYT* %14 to i64
  %17 = and i64 %16, 31
  %18 = icmp eq i64 %17, 0
  tail call void @llvm.assume(i1 %18)
  %19 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 6, i64 0
  %20 = ptrtoint i8* %19 to i64
  %21 = and i64 %20, 31
  %22 = icmp eq i64 %21, 0
  tail call void @llvm.assume(i1 %22)
  %23 = mul nuw nsw i64 %dst_x, %dst_c
  %24 = mul nuw nsw i64 %23, %13
  %25 = lshr i32 %11, 31
  %26 = add nuw i32 %25, 2147483647
  br label %y_body

then2:                                            ; preds = %then, %end6
  %storemerge129 = phi i32 [ 0, %then ], [ %37, %end6 ]
  %27 = phi i32 [ %4, %then ], [ %49, %end6 ]
  %columns = load i32, i32* %2, align 4, !range !0
  %28 = sext i32 %storemerge129 to i64
  %29 = zext i32 %columns to i64
  %tmp = add i64 %7, %28
  br label %then5

end3:                                             ; preds = %end6
  %30 = add nuw nsw i32 %storemerge30, 1
  %31 = icmp eq i32 %storemerge30, 0
  br i1 %31, label %end, label %then

then5:                                            ; preds = %then2, %end9
  %storemerge228 = phi i32 [ 0, %then2 ], [ %52, %end9 ]
  %32 = phi i32 [ %27, %then2 ], [ %49, %end9 ]
  %channels = load i32, i32* %3, align 4, !range !0
  %33 = sext i32 %storemerge228 to i64
  %34 = zext i32 %channels to i64
  %35 = mul nuw nsw i64 %29, %34
  %36 = mul nsw i64 %34, %33
  %tmp8 = mul i64 %35, %tmp
  br label %then8

end6:                                             ; preds = %end9
  %37 = add nuw nsw i32 %storemerge129, 1
  %38 = icmp eq i32 %37, %rows
  br i1 %38, label %end3, label %then2

then8:                                            ; preds = %then5, %then8
  %storemerge327 = phi i32 [ 0, %then5 ], [ %50, %then8 ]
  %39 = phi i32 [ %32, %then5 ], [ %49, %then8 ]
  %40 = sext i32 %storemerge327 to i64
  %41 = add i64 %36, %40
  %42 = add i64 %41, %tmp8
  %43 = getelementptr %u8SCXY, %u8SCXY* %0, i64 0, i32 6, i64 %42
  %44 = load i8, i8* %43, align 1
  %45 = uitofp i8 %44 to float
  %46 = fmul float %45, %45
  %47 = sitofp i32 %39 to float
  %48 = fadd float %47, %46
  %49 = fptosi float %48 to i32
  %50 = add nuw nsw i32 %storemerge327, 1
  %51 = icmp eq i32 %50, %channels
  br i1 %51, label %end9, label %then8

end9:                                             ; preds = %then8
  %52 = add nuw nsw i32 %storemerge228, 1
  %53 = icmp eq i32 %52, %columns
  br i1 %53, label %end6, label %then5

y_body:                                           ; preds = %y_body, %end
  %y = phi i64 [ 0, %end ], [ %y_increment, %y_body ]
  %54 = getelementptr %u8SCXY, %u8SCXY* %0, i64 0, i32 6, i64 %y
  %55 = load i8, i8* %54, align 1, !llvm.mem.parallel_loop_access !1
  %56 = zext i8 %55 to i32
  %57 = tail call { i32, i1 } @llvm.smul.with.overflow.i32(i32 %56, i32 %11)
  %58 = extractvalue { i32, i1 } %57, 1
  %59 = extractvalue { i32, i1 } %57, 0
  %60 = select i1 %58, i32 %26, i32 %59
  %61 = trunc i32 %60 to i8
  %62 = icmp slt i32 %60, 0
  %63 = select i1 %62, i8 0, i8 %61
  %64 = icmp sgt i32 %60, 255
  %65 = select i1 %64, i8 -1, i8 %63
  %66 = getelementptr i8, i8* %15, i64 %y
  store i8 %65, i8* %66, align 1, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %24
  br i1 %y_postcondition, label %y_exit, label %y_body, !llvm.loop !1

y_exit:                                           ; preds = %y_body
  %67 = bitcast %u0CXYT* %12 to %u8SCXY*
  ret %u8SCXY* %67
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
