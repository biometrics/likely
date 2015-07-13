; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64CXY = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%f32CXY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f64CXY* @min_max_loc(%f32CXY*) {
entry:
  %1 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 28992, i32 %channels, i32 3, i32 2, i32 1, i8* null)
  %3 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 3
  %width = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 4
  %height = load i32, i32* %4, align 4, !range !0
  %5 = zext i32 %channels to i64
  %dst_y_step = mul nuw nsw i64 %5, 3
  %6 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %7 = bitcast %u0CXYT* %6 to double*
  %8 = ptrtoint %u0CXYT* %6 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  %11 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 0
  %12 = ptrtoint float* %11 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  call void @llvm.assume(i1 %14)
  %15 = mul nuw nsw i32 %height, %width
  %16 = shl nuw nsw i64 %5, 1
  br label %c_body

c_body:                                           ; preds = %exit, %entry
  %c = phi i64 [ 0, %entry ], [ %c_increment, %exit ]
  br label %true_entry

true_entry:                                       ; preds = %c_body, %true_entry
  %17 = phi i32 [ %32, %true_entry ], [ 0, %c_body ]
  %18 = phi float [ %28, %true_entry ], [ 0x47EFFFFFE0000000, %c_body ]
  %19 = phi i32 [ %27, %true_entry ], [ 0, %c_body ]
  %20 = phi float [ %31, %true_entry ], [ 0xC7EFFFFFE0000000, %c_body ]
  %21 = phi i32 [ %30, %true_entry ], [ 0, %c_body ]
  %22 = sext i32 %17 to i64
  %23 = mul nuw nsw i64 %22, %5
  %24 = add nuw nsw i64 %23, %c
  %25 = getelementptr %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 %24
  %current-value = load float, float* %25, align 4, !llvm.mem.parallel_loop_access !1
  %26 = fcmp fast olt float %current-value, %18
  %27 = select i1 %26, i32 %17, i32 %19
  %28 = select i1 %26, float %current-value, float %18
  %29 = fcmp fast ogt float %current-value, %20
  %30 = select i1 %29, i32 %17, i32 %21
  %31 = select i1 %29, float %current-value, float %20
  %32 = add nuw nsw i32 %17, 1
  %33 = icmp eq i32 %32, %15
  br i1 %33, label %exit, label %true_entry

exit:                                             ; preds = %true_entry
  %34 = getelementptr double, double* %7, i64 %c
  %35 = fpext float %28 to double
  store double %35, double* %34, align 8, !llvm.mem.parallel_loop_access !1
  %36 = srem i32 %27, %width
  %37 = add nuw nsw i64 %c, %5
  %38 = getelementptr double, double* %7, i64 %37
  %39 = sitofp i32 %36 to double
  store double %39, double* %38, align 8, !llvm.mem.parallel_loop_access !1
  %40 = sdiv i32 %27, %width
  %41 = add nuw nsw i64 %c, %16
  %42 = getelementptr double, double* %7, i64 %41
  %43 = sitofp i32 %40 to double
  store double %43, double* %42, align 8, !llvm.mem.parallel_loop_access !1
  %44 = add nuw nsw i64 %c, %dst_y_step
  %45 = getelementptr double, double* %7, i64 %44
  %46 = fpext float %31 to double
  store double %46, double* %45, align 8, !llvm.mem.parallel_loop_access !1
  %47 = srem i32 %30, %width
  %48 = add nuw nsw i64 %37, %dst_y_step
  %49 = getelementptr double, double* %7, i64 %48
  %50 = sitofp i32 %47 to double
  store double %50, double* %49, align 8, !llvm.mem.parallel_loop_access !1
  %51 = sdiv i32 %30, %width
  %52 = add nuw nsw i64 %41, %dst_y_step
  %53 = getelementptr double, double* %7, i64 %52
  %54 = sitofp i32 %51 to double
  store double %54, double* %53, align 8, !llvm.mem.parallel_loop_access !1
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %5
  br i1 %c_postcondition, label %c_exit, label %c_body

c_exit:                                           ; preds = %exit
  %dst = bitcast %u0CXYT* %2 to %f64CXY*
  ret %f64CXY* %dst
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
