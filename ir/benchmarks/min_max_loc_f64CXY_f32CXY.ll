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
  %columns = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !0
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
  %15 = mul nuw nsw i32 %rows, %columns
  %16 = shl nuw nsw i64 %5, 1
  br label %c_body

c_body:                                           ; preds = %exit, %entry
  %c = phi i64 [ 0, %entry ], [ %c_increment, %exit ]
  br label %true_entry

true_entry:                                       ; preds = %c_body, %true_entry
  %17 = phi i32 [ %33, %true_entry ], [ 0, %c_body ]
  %18 = phi float [ %29, %true_entry ], [ 0x47EFFFFFE0000000, %c_body ]
  %19 = phi i32 [ %28, %true_entry ], [ 0, %c_body ]
  %20 = phi float [ %32, %true_entry ], [ 0xC7EFFFFFE0000000, %c_body ]
  %21 = phi i32 [ %31, %true_entry ], [ 0, %c_body ]
  %22 = sext i32 %17 to i64
  %23 = mul nuw nsw i64 %22, %5
  %24 = add nuw nsw i64 %23, %c
  %25 = getelementptr %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 %24
  %26 = load float, float* %25, align 4, !llvm.mem.parallel_loop_access !1
  %27 = fcmp olt float %26, %18
  %28 = select i1 %27, i32 %17, i32 %19
  %29 = select i1 %27, float %26, float %18
  %30 = fcmp ogt float %26, %20
  %31 = select i1 %30, i32 %17, i32 %21
  %32 = select i1 %30, float %26, float %20
  %33 = add nuw nsw i32 %17, 1
  %34 = icmp eq i32 %33, %15
  br i1 %34, label %exit, label %true_entry

exit:                                             ; preds = %true_entry
  %35 = getelementptr double, double* %7, i64 %c
  %36 = fpext float %29 to double
  store double %36, double* %35, align 8, !llvm.mem.parallel_loop_access !1
  %37 = srem i32 %28, %columns
  %38 = add nuw nsw i64 %c, %5
  %39 = getelementptr double, double* %7, i64 %38
  %40 = sitofp i32 %37 to double
  store double %40, double* %39, align 8, !llvm.mem.parallel_loop_access !1
  %41 = sdiv i32 %28, %columns
  %42 = add nuw nsw i64 %c, %16
  %43 = getelementptr double, double* %7, i64 %42
  %44 = sitofp i32 %41 to double
  store double %44, double* %43, align 8, !llvm.mem.parallel_loop_access !1
  %45 = add nuw nsw i64 %c, %dst_y_step
  %46 = getelementptr double, double* %7, i64 %45
  %47 = fpext float %32 to double
  store double %47, double* %46, align 8, !llvm.mem.parallel_loop_access !1
  %48 = srem i32 %31, %columns
  %49 = add nuw nsw i64 %38, %dst_y_step
  %50 = getelementptr double, double* %7, i64 %49
  %51 = sitofp i32 %48 to double
  store double %51, double* %50, align 8, !llvm.mem.parallel_loop_access !1
  %52 = sdiv i32 %31, %columns
  %53 = add nuw nsw i64 %42, %dst_y_step
  %54 = getelementptr double, double* %7, i64 %53
  %55 = sitofp i32 %52 to double
  store double %55, double* %54, align 8, !llvm.mem.parallel_loop_access !1
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %5
  br i1 %c_postcondition, label %c_exit, label %c_body

c_exit:                                           ; preds = %exit
  %56 = bitcast %u0CXYT* %2 to %f64CXY*
  ret %f64CXY* %56
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
