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
  br label %y_body

y_body:                                           ; preds = %x_exit, %end
  %y = phi i64 [ 0, %end ], [ %y_increment, %x_exit ]
  %28 = mul i64 %y, %dst_x
  %29 = mul nuw nsw i64 %28, %dst_c
  br label %x_body

x_body:                                           ; preds = %x_body, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %x_body ]
  %30 = add nuw nsw i64 %29, %x
  %31 = getelementptr %f64CXY, %f64CXY* %0, i64 0, i32 6, i64 %30
  %32 = load double, double* %31, align 8, !llvm.mem.parallel_loop_access !1
  %33 = fmul double %15, %32
  %34 = getelementptr double, double* %19, i64 %30
  store double %33, double* %34, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %27
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !1

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %17
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %35 = bitcast %u0CXYT* %16 to %f64CXY*
  ret %f64CXY* %35
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
