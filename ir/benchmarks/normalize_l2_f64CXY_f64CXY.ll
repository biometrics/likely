; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64CXY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind readnone
declare double @llvm.sqrt.f64(double) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
declare void @llvm.assume(i1) #2

; Function Attrs: nounwind
define %f64CXY* @normalize_l2(%f64CXY*) #2 {
entry:
  %1 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 4
  %2 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 3
  %3 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 2
  br label %then

then:                                             ; preds = %entry, %end3
  %storemerge29 = phi i32 [ 0, %entry ], [ %26, %end3 ]
  %4 = phi double [ 0.000000e+00, %entry ], [ %44, %end3 ]
  %rows = load i32, i32* %1, align 4, !range !0
  %5 = sext i32 %storemerge29 to i64
  %6 = zext i32 %rows to i64
  %7 = mul nsw i64 %6, %5
  br label %then2

end:                                              ; preds = %end3
  %8 = tail call double @llvm.sqrt.f64(double %44)
  %9 = fdiv double 1.000000e+00, %8
  %channels13 = load i32, i32* %3, align 4, !range !0
  %columns14 = load i32, i32* %2, align 4, !range !0
  %rows15 = load i32, i32* %1, align 4, !range !0
  %10 = tail call %u0CXYT* @likely_new(i32 28992, i32 %channels13, i32 %columns14, i32 %rows15, i32 1, i8* null)
  %11 = zext i32 %rows15 to i64
  %dst_c = zext i32 %channels13 to i64
  %dst_x = zext i32 %columns14 to i64
  %12 = getelementptr inbounds %u0CXYT, %u0CXYT* %10, i64 1
  %13 = bitcast %u0CXYT* %12 to double*
  %14 = ptrtoint %u0CXYT* %12 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  tail call void @llvm.assume(i1 %16)
  %17 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 6, i64 0
  %18 = ptrtoint double* %17 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  tail call void @llvm.assume(i1 %20)
  %21 = mul nuw nsw i64 %dst_x, %dst_c
  %22 = mul nuw nsw i64 %21, %11
  br label %y_body

then2:                                            ; preds = %then, %end6
  %storemerge128 = phi i32 [ 0, %then ], [ %33, %end6 ]
  %23 = phi double [ %4, %then ], [ %44, %end6 ]
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
  %storemerge227 = phi i32 [ 0, %then2 ], [ %47, %end9 ]
  %28 = phi double [ %23, %then2 ], [ %44, %end9 ]
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
  %storemerge326 = phi i32 [ 0, %then5 ], [ %45, %then8 ]
  %35 = phi double [ %28, %then5 ], [ %44, %then8 ]
  %36 = sext i32 %storemerge326 to i64
  %37 = add i64 %32, %36
  %38 = add i64 %37, %tmp7
  %39 = getelementptr %f64CXY, %f64CXY* %0, i64 0, i32 6, i64 %38
  %40 = load double, double* %39, align 8
  %41 = fptrunc double %40 to float
  %42 = fmul float %41, %41
  %43 = fpext float %42 to double
  %44 = fadd double %35, %43
  %45 = add nuw nsw i32 %storemerge326, 1
  %46 = icmp eq i32 %45, %channels
  br i1 %46, label %end9, label %then8

end9:                                             ; preds = %then8
  %47 = add nuw nsw i32 %storemerge227, 1
  %48 = icmp eq i32 %47, %columns
  br i1 %48, label %end6, label %then5

y_body:                                           ; preds = %y_body, %end
  %y = phi i64 [ 0, %end ], [ %y_increment, %y_body ]
  %49 = getelementptr %f64CXY, %f64CXY* %0, i64 0, i32 6, i64 %y
  %50 = load double, double* %49, align 8, !llvm.mem.parallel_loop_access !1
  %51 = fmul double %9, %50
  %52 = getelementptr double, double* %13, i64 %y
  store double %51, double* %52, align 8, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %22
  br i1 %y_postcondition, label %y_exit, label %y_body, !llvm.loop !1

y_exit:                                           ; preds = %y_body
  %53 = bitcast %u0CXYT* %10 to %f64CXY*
  ret %f64CXY* %53
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
