; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32CXY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readnone
declare double @llvm.sqrt.f64(double) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
declare void @llvm.assume(i1) #2

; Function Attrs: nounwind
define %f32CXY* @normalize_l2(%f32CXY*) #2 {
entry:
  %1 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 4
  %2 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 3
  %3 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 2
  br label %then

then:                                             ; preds = %entry, %end3
  %storemerge29 = phi i32 [ 0, %entry ], [ %26, %end3 ]
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
  %10 = tail call %u0CXYT* @likely_new(i32 28960, i32 %channels13, i32 %columns14, i32 %rows15, i32 1, i8* null)
  %11 = zext i32 %rows15 to i64
  %dst_c = zext i32 %channels13 to i64
  %dst_x = zext i32 %columns14 to i64
  %12 = getelementptr inbounds %u0CXYT, %u0CXYT* %10, i64 1
  %13 = bitcast %u0CXYT* %12 to float*
  %14 = ptrtoint %u0CXYT* %12 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  tail call void @llvm.assume(i1 %16)
  %17 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 0
  %18 = ptrtoint float* %17 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  tail call void @llvm.assume(i1 %20)
  %21 = mul nuw nsw i64 %dst_x, %dst_c
  %22 = mul nuw nsw i64 %21, %11
  br label %y_body

then2:                                            ; preds = %then, %end6
  %storemerge128 = phi i32 [ 0, %then ], [ %33, %end6 ]
  %23 = phi double [ %4, %then ], [ %43, %end6 ]
  %columns = load i32, i32* %2, align 4, !range !0
  %24 = sext i32 %storemerge128 to i64
  %25 = zext i32 %columns to i64
  %tmp = add i64 %7, %24
  br label %then5

end3:                                             ; preds = %end6
  %26 = add nuw nsw i32 %storemerge29, 1
  %27 = icmp eq i32 %storemerge29, 0
  br i1 %27, label %end, label %then

then5:                                            ; preds = %then2, %end9
  %storemerge227 = phi i32 [ 0, %then2 ], [ %46, %end9 ]
  %28 = phi double [ %23, %then2 ], [ %43, %end9 ]
  %channels = load i32, i32* %3, align 4, !range !0
  %29 = sext i32 %storemerge227 to i64
  %30 = zext i32 %channels to i64
  %31 = mul nuw nsw i64 %25, %30
  %32 = mul nsw i64 %30, %29
  %tmp7 = mul i64 %31, %tmp
  br label %then8

end6:                                             ; preds = %end9
  %33 = add nuw nsw i32 %storemerge128, 1
  %34 = icmp eq i32 %33, %rows
  br i1 %34, label %end3, label %then2

then8:                                            ; preds = %then5, %then8
  %storemerge326 = phi i32 [ 0, %then5 ], [ %44, %then8 ]
  %35 = phi double [ %28, %then5 ], [ %43, %then8 ]
  %36 = sext i32 %storemerge326 to i64
  %37 = add i64 %32, %36
  %38 = add i64 %37, %tmp7
  %39 = getelementptr %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 %38
  %40 = load float, float* %39, align 4
  %41 = fmul float %40, %40
  %42 = fpext float %41 to double
  %43 = fadd double %35, %42
  %44 = add nuw nsw i32 %storemerge326, 1
  %45 = icmp eq i32 %44, %channels
  br i1 %45, label %end9, label %then8

end9:                                             ; preds = %then8
  %46 = add nuw nsw i32 %storemerge227, 1
  %47 = icmp eq i32 %46, %columns
  br i1 %47, label %end6, label %then5

y_body:                                           ; preds = %y_body, %end
  %y = phi i64 [ 0, %end ], [ %y_increment, %y_body ]
  %48 = getelementptr %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 %y
  %49 = load float, float* %48, align 4, !llvm.mem.parallel_loop_access !1
  %50 = fpext float %49 to double
  %51 = fmul double %9, %50
  %52 = fptrunc double %51 to float
  %53 = getelementptr float, float* %13, i64 %y
  store float %52, float* %53, align 4, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %22
  br i1 %y_postcondition, label %y_exit, label %y_body, !llvm.loop !1

y_exit:                                           ; preds = %y_body
  %54 = bitcast %u0CXYT* %10 to %f32CXY*
  ret %f32CXY* %54
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
