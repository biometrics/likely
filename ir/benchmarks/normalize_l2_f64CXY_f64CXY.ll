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
  %1 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = mul nuw nsw i32 %columns, %channels
  %4 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !0
  %5 = mul nuw nsw i32 %3, %rows
  br label %then

then:                                             ; preds = %entry, %then
  %storemerge4 = phi i32 [ 0, %entry ], [ %12, %then ]
  %6 = phi double [ 0.000000e+00, %entry ], [ %11, %then ]
  %7 = sext i32 %storemerge4 to i64
  %8 = getelementptr %f64CXY, %f64CXY* %0, i64 0, i32 6, i64 %7
  %9 = load double, double* %8, align 8
  %10 = fmul double %9, %9
  %11 = fadd double %6, %10
  %12 = add nuw nsw i32 %storemerge4, 1
  %13 = icmp eq i32 %12, %5
  br i1 %13, label %end, label %then

end:                                              ; preds = %then
  %14 = tail call double @llvm.sqrt.f64(double %11)
  %15 = fdiv double 1.000000e+00, %14
  %16 = tail call %u0CXYT* @likely_new(i32 28992, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %17 = zext i32 %rows to i64
  %dst_c = zext i32 %channels to i64
  %dst_x = zext i32 %columns to i64
  %18 = getelementptr inbounds %u0CXYT, %u0CXYT* %16, i64 1
  %19 = bitcast %u0CXYT* %18 to double*
  %20 = ptrtoint %u0CXYT* %18 to i64
  %21 = and i64 %20, 31
  %22 = icmp eq i64 %21, 0
  tail call void @llvm.assume(i1 %22)
  %23 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 6, i64 0
  %24 = ptrtoint double* %23 to i64
  %25 = and i64 %24, 31
  %26 = icmp eq i64 %25, 0
  tail call void @llvm.assume(i1 %26)
  %27 = mul nuw nsw i64 %dst_x, %dst_c
  %28 = mul nuw nsw i64 %17, %27
  br label %y_body

y_body:                                           ; preds = %y_body, %end
  %y = phi i64 [ 0, %end ], [ %y_increment, %y_body ]
  %29 = getelementptr %f64CXY, %f64CXY* %0, i64 0, i32 6, i64 %y
  %30 = load double, double* %29, align 8, !llvm.mem.parallel_loop_access !1
  %31 = fmul double %15, %30
  %32 = getelementptr double, double* %19, i64 %y
  store double %31, double* %32, align 8, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %28
  br i1 %y_postcondition, label %y_exit, label %y_body, !llvm.loop !1

y_exit:                                           ; preds = %y_body
  %33 = bitcast %u0CXYT* %16 to %f64CXY*
  ret %f64CXY* %33
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
