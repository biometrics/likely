; ModuleID = 'likely'
source_filename = "likely"

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64CXY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind readnone
declare double @llvm.sqrt.f64(double) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

define noalias %f64CXY* @normalize_l2(%f64CXY* nocapture readonly) {
entry:
  %1 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = mul nuw nsw i32 %columns, %channels
  %4 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !0
  %5 = mul nuw nsw i32 %3, %rows
  br label %true_entry

true_entry:                                       ; preds = %true_entry, %entry
  %6 = phi i32 [ 0, %entry ], [ %13, %true_entry ]
  %7 = phi double [ 0.000000e+00, %entry ], [ %12, %true_entry ]
  %8 = sext i32 %6 to i64
  %9 = getelementptr %f64CXY, %f64CXY* %0, i64 0, i32 6, i64 %8
  %10 = load double, double* %9, align 8
  %11 = fmul fast double %10, %10
  %12 = fadd fast double %11, %7
  %13 = add nuw nsw i32 %6, 1
  %14 = icmp eq i32 %13, %5
  br i1 %14, label %exit, label %true_entry

exit:                                             ; preds = %true_entry
  %15 = call fast double @llvm.sqrt.f64(double %12)
  %16 = fdiv fast double 1.000000e+00, %15
  %17 = call %u0CXYT* @likely_new(i32 28992, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %18 = zext i32 %rows to i64
  %dst_c = zext i32 %channels to i64
  %dst_x = zext i32 %columns to i64
  %19 = getelementptr inbounds %u0CXYT, %u0CXYT* %17, i64 1
  %20 = bitcast %u0CXYT* %19 to double*
  %21 = mul nuw nsw i64 %dst_x, %dst_c
  %22 = mul nuw nsw i64 %21, %18
  br label %y_body

y_body:                                           ; preds = %y_body, %exit
  %y = phi i64 [ 0, %exit ], [ %y_increment, %y_body ]
  %23 = getelementptr %f64CXY, %f64CXY* %0, i64 0, i32 6, i64 %y
  %24 = load double, double* %23, align 8, !llvm.mem.parallel_loop_access !1
  %25 = fmul fast double %24, %16
  %26 = getelementptr double, double* %20, i64 %y
  store double %25, double* %26, align 8, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %22
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %dst = bitcast %u0CXYT* %17 to %f64CXY*
  ret %f64CXY* %dst
}

attributes #0 = { nounwind readnone }
attributes #1 = { argmemonly nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
