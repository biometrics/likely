; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64Matrix = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define noalias %f64Matrix* @min_max_loc(%f32Matrix* noalias nocapture readonly) #1 {
entry:
  br label %entry.split

entry.split:                                      ; preds = %entry
  %1 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = mul nuw nsw i32 %columns, %channels
  %4 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !0
  %5 = mul nuw nsw i32 %3, %rows
  br label %true_entry

true_entry:                                       ; preds = %true_entry, %entry.split
  %6 = phi i32 [ 0, %entry.split ], [ %19, %true_entry ]
  %7 = phi float [ 0x47EFFFFFE0000000, %entry.split ], [ %15, %true_entry ]
  %8 = phi i32 [ 0, %entry.split ], [ %14, %true_entry ]
  %9 = phi float [ 0xC7EFFFFFE0000000, %entry.split ], [ %18, %true_entry ]
  %10 = phi i32 [ 0, %entry.split ], [ %17, %true_entry ]
  %11 = zext i32 %6 to i64
  %12 = getelementptr %f32Matrix, %f32Matrix* %0, i64 0, i32 6, i64 %11
  %current-value = load float, float* %12, align 4
  %13 = fcmp fast olt float %current-value, %7
  %14 = select i1 %13, i32 %6, i32 %8
  %15 = select i1 %13, float %current-value, float %7
  %16 = fcmp fast ogt float %current-value, %9
  %17 = select i1 %16, i32 %6, i32 %10
  %18 = select i1 %16, float %current-value, float %9
  %19 = add nuw nsw i32 %6, 1
  %20 = icmp eq i32 %19, %5
  br i1 %20, label %exit, label %true_entry

exit:                                             ; preds = %true_entry
  %21 = call %u0Matrix* @likely_new(i32 24896, i32 1, i32 3, i32 2, i32 1, i8* null)
  %dst = bitcast %u0Matrix* %21 to %f64Matrix*
  %22 = getelementptr inbounds %u0Matrix, %u0Matrix* %21, i64 1
  %23 = bitcast %u0Matrix* %22 to double*
  %24 = fpext float %15 to double
  store double %24, double* %23, align 8
  %25 = srem i32 %14, %columns
  %26 = getelementptr %u0Matrix, %u0Matrix* %21, i64 1, i32 2
  %27 = bitcast i32* %26 to double*
  %28 = sitofp i32 %25 to double
  store double %28, double* %27, align 8
  %29 = sdiv i32 %14, %columns
  %30 = getelementptr %u0Matrix, %u0Matrix* %21, i64 1, i32 4
  %31 = bitcast i32* %30 to double*
  %32 = sitofp i32 %29 to double
  store double %32, double* %31, align 8
  %33 = getelementptr %u0Matrix, %u0Matrix* %21, i64 2
  %34 = bitcast %u0Matrix* %33 to double*
  %35 = fpext float %18 to double
  store double %35, double* %34, align 8
  %36 = srem i32 %17, %columns
  %37 = getelementptr %u0Matrix, %u0Matrix* %21, i64 2, i32 2
  %38 = bitcast i32* %37 to double*
  %39 = sitofp i32 %36 to double
  store double %39, double* %38, align 8
  %40 = sdiv i32 %17, %columns
  %41 = getelementptr %u0Matrix, %u0Matrix* %21, i64 2, i32 4
  %42 = bitcast i32* %41 to double*
  %43 = sitofp i32 %40 to double
  store double %43, double* %42, align 8
  ret %f64Matrix* %dst
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
