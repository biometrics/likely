; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64Matrix = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define noalias %f64Matrix* @min_max_loc(%f64Matrix* noalias nocapture readonly) #1 {
entry:
  br label %entry.split

entry.split:                                      ; preds = %entry
  %1 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = mul nuw nsw i32 %columns, %channels
  %4 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !0
  %5 = mul nuw nsw i32 %3, %rows
  br label %true_entry

true_entry:                                       ; preds = %true_entry, %entry.split
  %6 = phi i32 [ 0, %entry.split ], [ %19, %true_entry ]
  %7 = phi double [ 0x7FEFFFFFFFFFFFFF, %entry.split ], [ %15, %true_entry ]
  %8 = phi i32 [ 0, %entry.split ], [ %14, %true_entry ]
  %9 = phi double [ 0xFFEFFFFFFFFFFFFF, %entry.split ], [ %18, %true_entry ]
  %10 = phi i32 [ 0, %entry.split ], [ %17, %true_entry ]
  %11 = zext i32 %6 to i64
  %12 = getelementptr %f64Matrix, %f64Matrix* %0, i64 0, i32 6, i64 %11
  %current-value = load double, double* %12, align 8
  %13 = fcmp fast olt double %current-value, %7
  %14 = select i1 %13, i32 %6, i32 %8
  %15 = select i1 %13, double %current-value, double %7
  %16 = fcmp fast ogt double %current-value, %9
  %17 = select i1 %16, i32 %6, i32 %10
  %18 = select i1 %16, double %current-value, double %9
  %19 = add nuw nsw i32 %6, 1
  %20 = icmp eq i32 %19, %5
  br i1 %20, label %exit, label %true_entry

exit:                                             ; preds = %true_entry
  %21 = call %u0Matrix* @likely_new(i32 24896, i32 1, i32 3, i32 2, i32 1, i8* null)
  %dst = bitcast %u0Matrix* %21 to %f64Matrix*
  %22 = getelementptr inbounds %u0Matrix, %u0Matrix* %21, i64 1
  %23 = bitcast %u0Matrix* %22 to double*
  store double %15, double* %23, align 8
  %24 = srem i32 %14, %columns
  %25 = getelementptr %u0Matrix, %u0Matrix* %21, i64 1, i32 2
  %26 = bitcast i32* %25 to double*
  %27 = sitofp i32 %24 to double
  store double %27, double* %26, align 8
  %28 = sdiv i32 %14, %columns
  %29 = getelementptr %u0Matrix, %u0Matrix* %21, i64 1, i32 4
  %30 = bitcast i32* %29 to double*
  %31 = sitofp i32 %28 to double
  store double %31, double* %30, align 8
  %32 = getelementptr %u0Matrix, %u0Matrix* %21, i64 2
  %33 = bitcast %u0Matrix* %32 to double*
  store double %18, double* %33, align 8
  %34 = srem i32 %17, %columns
  %35 = getelementptr %u0Matrix, %u0Matrix* %21, i64 2, i32 2
  %36 = bitcast i32* %35 to double*
  %37 = sitofp i32 %34 to double
  store double %37, double* %36, align 8
  %38 = sdiv i32 %17, %columns
  %39 = getelementptr %u0Matrix, %u0Matrix* %21, i64 2, i32 4
  %40 = bitcast i32* %39 to double*
  %41 = sitofp i32 %38 to double
  store double %41, double* %40, align 8
  ret %f64Matrix* %dst
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
