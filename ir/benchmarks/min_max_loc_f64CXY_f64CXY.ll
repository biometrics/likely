; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64CXY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f64CXY* @min_max_loc(%f64CXY*) {
entry:
  %1 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 28992, i32 %channels, i32 3, i32 2, i32 1, i8* null)
  %3 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 3
  %width = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 4
  %height = load i32, i32* %4, align 4, !range !0
  %5 = zext i32 %channels to i64
  %dst_y_step = mul nuw nsw i64 %5, 3
  %6 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %7 = bitcast %u0CXYT* %6 to double*
  %8 = ptrtoint %u0CXYT* %6 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  %11 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 6, i64 0
  %12 = ptrtoint double* %11 to i64
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
  %18 = phi double [ %28, %true_entry ], [ 0x7FEFFFFFFFFFFFFF, %c_body ]
  %19 = phi i32 [ %27, %true_entry ], [ 0, %c_body ]
  %20 = phi double [ %31, %true_entry ], [ 0xFFEFFFFFFFFFFFFF, %c_body ]
  %21 = phi i32 [ %30, %true_entry ], [ 0, %c_body ]
  %22 = sext i32 %17 to i64
  %23 = mul nuw nsw i64 %22, %5
  %24 = add nuw nsw i64 %23, %c
  %25 = getelementptr %f64CXY, %f64CXY* %0, i64 0, i32 6, i64 %24
  %current-value = load double, double* %25, align 8, !llvm.mem.parallel_loop_access !1
  %26 = fcmp olt double %current-value, %18
  %27 = select i1 %26, i32 %17, i32 %19
  %28 = select i1 %26, double %current-value, double %18
  %29 = fcmp ogt double %current-value, %20
  %30 = select i1 %29, i32 %17, i32 %21
  %31 = select i1 %29, double %current-value, double %20
  %32 = add nuw nsw i32 %17, 1
  %33 = icmp eq i32 %32, %15
  br i1 %33, label %exit, label %true_entry

exit:                                             ; preds = %true_entry
  %34 = getelementptr double, double* %7, i64 %c
  store double %28, double* %34, align 8, !llvm.mem.parallel_loop_access !1
  %35 = srem i32 %27, %width
  %36 = add nuw nsw i64 %c, %5
  %37 = getelementptr double, double* %7, i64 %36
  %38 = sitofp i32 %35 to double
  store double %38, double* %37, align 8, !llvm.mem.parallel_loop_access !1
  %39 = sdiv i32 %27, %width
  %40 = add nuw nsw i64 %c, %16
  %41 = getelementptr double, double* %7, i64 %40
  %42 = sitofp i32 %39 to double
  store double %42, double* %41, align 8, !llvm.mem.parallel_loop_access !1
  %43 = add nuw nsw i64 %c, %dst_y_step
  %44 = getelementptr double, double* %7, i64 %43
  store double %31, double* %44, align 8, !llvm.mem.parallel_loop_access !1
  %45 = srem i32 %30, %width
  %46 = add nuw nsw i64 %36, %dst_y_step
  %47 = getelementptr double, double* %7, i64 %46
  %48 = sitofp i32 %45 to double
  store double %48, double* %47, align 8, !llvm.mem.parallel_loop_access !1
  %49 = sdiv i32 %30, %width
  %50 = add nuw nsw i64 %40, %dst_y_step
  %51 = getelementptr double, double* %7, i64 %50
  %52 = sitofp i32 %49 to double
  store double %52, double* %51, align 8, !llvm.mem.parallel_loop_access !1
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
