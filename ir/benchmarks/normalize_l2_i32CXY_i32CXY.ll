; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%i32CXY = type { i32, i32, i32, i32, i32, i32, [0 x i32] }

; Function Attrs: nounwind readnone
declare double @llvm.sqrt.f64(double) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
declare void @llvm.assume(i1) #2

; Function Attrs: nounwind
define %i32CXY* @normalize_l2(%i32CXY*) #2 {
entry:
  %1 = getelementptr inbounds %i32CXY, %i32CXY* %0, i64 0, i32 4
  %2 = getelementptr inbounds %i32CXY, %i32CXY* %0, i64 0, i32 3
  %3 = getelementptr inbounds %i32CXY, %i32CXY* %0, i64 0, i32 2
  br label %then

then:                                             ; preds = %entry, %end3
  %storemerge29 = phi i32 [ 0, %entry ], [ %25, %end3 ]
  %4 = phi double [ 0.000000e+00, %entry ], [ %43, %end3 ]
  %rows = load i32, i32* %1, align 4, !range !0
  %5 = sext i32 %storemerge29 to i64
  %6 = zext i32 %rows to i64
  %7 = mul nsw i64 %6, %5
  br label %then2

end:                                              ; preds = %end3
  %8 = tail call double @llvm.sqrt.f64(double %43)
  %9 = fdiv double 1.000000e+00, %8
  %channels13 = load i32, i32* %3, align 4, !range !0
  %columns14 = load i32, i32* %2, align 4, !range !0
  %rows15 = load i32, i32* %1, align 4, !range !0
  %10 = tail call %u0CXYT* @likely_new(i32 29216, i32 %channels13, i32 %columns14, i32 %rows15, i32 1, i8* null)
  %11 = zext i32 %rows15 to i64
  %dst_c = zext i32 %channels13 to i64
  %dst_x = zext i32 %columns14 to i64
  %12 = getelementptr inbounds %u0CXYT, %u0CXYT* %10, i64 1, i32 0
  %13 = ptrtoint i32* %12 to i64
  %14 = and i64 %13, 31
  %15 = icmp eq i64 %14, 0
  tail call void @llvm.assume(i1 %15)
  %16 = getelementptr inbounds %i32CXY, %i32CXY* %0, i64 0, i32 6, i64 0
  %17 = ptrtoint i32* %16 to i64
  %18 = and i64 %17, 31
  %19 = icmp eq i64 %18, 0
  tail call void @llvm.assume(i1 %19)
  %20 = mul nuw nsw i64 %dst_x, %dst_c
  %21 = mul nuw nsw i64 %20, %11
  br label %y_body

then2:                                            ; preds = %then, %end6
  %storemerge128 = phi i32 [ 0, %then ], [ %32, %end6 ]
  %22 = phi double [ %4, %then ], [ %43, %end6 ]
  %columns = load i32, i32* %2, align 4, !range !0
  %23 = sext i32 %storemerge128 to i64
  %24 = zext i32 %columns to i64
  %tmp = add i64 %7, %23
  br label %then5

end3:                                             ; preds = %end6
  %25 = add nuw nsw i32 %storemerge29, 1
  %26 = icmp eq i32 %storemerge29, 0
  br i1 %26, label %end, label %then

then5:                                            ; preds = %then2, %end9
  %storemerge227 = phi i32 [ 0, %then2 ], [ %46, %end9 ]
  %27 = phi double [ %22, %then2 ], [ %43, %end9 ]
  %channels = load i32, i32* %3, align 4, !range !0
  %28 = sext i32 %storemerge227 to i64
  %29 = zext i32 %channels to i64
  %30 = mul nuw nsw i64 %24, %29
  %31 = mul nsw i64 %29, %28
  %tmp7 = mul i64 %30, %tmp
  br label %then8

end6:                                             ; preds = %end9
  %32 = add nuw nsw i32 %storemerge128, 1
  %33 = icmp eq i32 %32, %rows
  br i1 %33, label %end3, label %then2

then8:                                            ; preds = %then5, %then8
  %storemerge326 = phi i32 [ 0, %then5 ], [ %44, %then8 ]
  %34 = phi double [ %27, %then5 ], [ %43, %then8 ]
  %35 = sext i32 %storemerge326 to i64
  %36 = add i64 %31, %35
  %37 = add i64 %36, %tmp7
  %38 = getelementptr %i32CXY, %i32CXY* %0, i64 0, i32 6, i64 %37
  %39 = load i32, i32* %38, align 4
  %40 = sitofp i32 %39 to float
  %41 = fmul float %40, %40
  %42 = fpext float %41 to double
  %43 = fadd double %34, %42
  %44 = add nuw nsw i32 %storemerge326, 1
  %45 = icmp eq i32 %44, %channels
  br i1 %45, label %end9, label %then8

end9:                                             ; preds = %then8
  %46 = add nuw nsw i32 %storemerge227, 1
  %47 = icmp eq i32 %46, %columns
  br i1 %47, label %end6, label %then5

y_body:                                           ; preds = %y_body, %end
  %y = phi i64 [ 0, %end ], [ %y_increment, %y_body ]
  %48 = getelementptr %i32CXY, %i32CXY* %0, i64 0, i32 6, i64 %y
  %49 = load i32, i32* %48, align 4, !llvm.mem.parallel_loop_access !1
  %50 = sitofp i32 %49 to double
  %51 = fmul double %9, %50
  %52 = fptosi double %51 to i32
  %53 = getelementptr i32, i32* %12, i64 %y
  store i32 %52, i32* %53, align 4, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %21
  br i1 %y_postcondition, label %y_exit, label %y_body, !llvm.loop !1

y_exit:                                           ; preds = %y_body
  %54 = bitcast %u0CXYT* %10 to %i32CXY*
  ret %i32CXY* %54
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
