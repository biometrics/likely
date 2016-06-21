; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64Matrix = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%u8Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define noalias %f64Matrix* @min_max_loc(%u8Matrix* noalias nocapture readonly) #1 {
entry:
  %1 = getelementptr inbounds %u8Matrix, %u8Matrix* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %u8Matrix, %u8Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = mul nuw nsw i32 %columns, %channels
  %4 = getelementptr inbounds %u8Matrix, %u8Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !0
  %5 = mul nuw nsw i32 %3, %rows
  br label %true_entry

true_entry:                                       ; preds = %true_entry, %entry
  %6 = phi i32 [ 0, %entry ], [ %17, %true_entry ]
  %7 = phi i8 [ -1, %entry ], [ %current-value., %true_entry ]
  %8 = phi i32 [ 0, %entry ], [ %., %true_entry ]
  %9 = phi i8 [ 0, %entry ], [ %16, %true_entry ]
  %10 = phi i32 [ 0, %entry ], [ %15, %true_entry ]
  %11 = sext i32 %6 to i64
  %12 = getelementptr %u8Matrix, %u8Matrix* %0, i64 0, i32 6, i64 %11
  %current-value = load i8, i8* %12, align 1
  %13 = icmp ult i8 %current-value, %7
  %. = select i1 %13, i32 %6, i32 %8
  %current-value. = select i1 %13, i8 %current-value, i8 %7
  %14 = icmp ugt i8 %current-value, %9
  %15 = select i1 %14, i32 %6, i32 %10
  %16 = select i1 %14, i8 %current-value, i8 %9
  %17 = add nuw nsw i32 %6, 1
  %18 = icmp eq i32 %17, %5
  br i1 %18, label %exit, label %true_entry

exit:                                             ; preds = %true_entry
  %19 = call %u0Matrix* @likely_new(i32 24896, i32 1, i32 3, i32 2, i32 1, i8* null)
  %dst = bitcast %u0Matrix* %19 to %f64Matrix*
  %20 = getelementptr inbounds %u0Matrix, %u0Matrix* %19, i64 1
  %21 = bitcast %u0Matrix* %20 to double*
  %22 = uitofp i8 %current-value. to double
  store double %22, double* %21, align 8
  %23 = srem i32 %., %columns
  %24 = getelementptr %u0Matrix, %u0Matrix* %19, i64 1, i32 2
  %25 = bitcast i32* %24 to double*
  %26 = sitofp i32 %23 to double
  store double %26, double* %25, align 8
  %27 = sdiv i32 %., %columns
  %28 = getelementptr %u0Matrix, %u0Matrix* %19, i64 1, i32 4
  %29 = bitcast i32* %28 to double*
  %30 = sitofp i32 %27 to double
  store double %30, double* %29, align 8
  %31 = getelementptr %u0Matrix, %u0Matrix* %19, i64 2
  %32 = bitcast %u0Matrix* %31 to double*
  %33 = uitofp i8 %16 to double
  store double %33, double* %32, align 8
  %34 = srem i32 %15, %columns
  %35 = getelementptr %u0Matrix, %u0Matrix* %19, i64 2, i32 2
  %36 = bitcast i32* %35 to double*
  %37 = sitofp i32 %34 to double
  store double %37, double* %36, align 8
  %38 = sdiv i32 %15, %columns
  %39 = getelementptr %u0Matrix, %u0Matrix* %19, i64 2, i32 4
  %40 = bitcast i32* %39 to double*
  %41 = sitofp i32 %38 to double
  store double %41, double* %40, align 8
  ret %f64Matrix* %dst
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
