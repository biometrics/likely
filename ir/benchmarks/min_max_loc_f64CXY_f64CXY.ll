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
  %columns = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !0
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
  %15 = mul nuw nsw i32 %rows, %columns
  %16 = shl nuw nsw i64 %5, 1
  br label %c_body

c_body:                                           ; preds = %exit, %entry
  %c = phi i64 [ 0, %entry ], [ %c_increment, %exit ]
  br label %true_entry

true_entry:                                       ; preds = %c_body, %true_entry
  %17 = phi i32 [ %33, %true_entry ], [ 0, %c_body ]
  %18 = phi double [ %29, %true_entry ], [ 0x7FEFFFFFFFFFFFFF, %c_body ]
  %19 = phi i32 [ %28, %true_entry ], [ 0, %c_body ]
  %20 = phi double [ %32, %true_entry ], [ 0xFFEFFFFFFFFFFFFF, %c_body ]
  %21 = phi i32 [ %31, %true_entry ], [ 0, %c_body ]
  %22 = sext i32 %17 to i64
  %23 = mul nuw nsw i64 %22, %5
  %24 = add nuw nsw i64 %23, %c
  %25 = getelementptr %f64CXY, %f64CXY* %0, i64 0, i32 6, i64 %24
  %26 = load double, double* %25, align 8, !llvm.mem.parallel_loop_access !1
  %27 = fcmp olt double %26, %18
  %28 = select i1 %27, i32 %17, i32 %19
  %29 = select i1 %27, double %26, double %18
  %30 = fcmp ogt double %26, %20
  %31 = select i1 %30, i32 %17, i32 %21
  %32 = select i1 %30, double %26, double %20
  %33 = add nuw nsw i32 %17, 1
  %34 = icmp eq i32 %33, %15
  br i1 %34, label %exit, label %true_entry

exit:                                             ; preds = %true_entry
  %35 = getelementptr double, double* %7, i64 %c
  store double %29, double* %35, align 8, !llvm.mem.parallel_loop_access !1
  %36 = srem i32 %28, %columns
  %37 = add nuw nsw i64 %c, %5
  %38 = getelementptr double, double* %7, i64 %37
  %39 = sitofp i32 %36 to double
  store double %39, double* %38, align 8, !llvm.mem.parallel_loop_access !1
  %40 = sdiv i32 %28, %columns
  %41 = add nuw nsw i64 %c, %16
  %42 = getelementptr double, double* %7, i64 %41
  %43 = sitofp i32 %40 to double
  store double %43, double* %42, align 8, !llvm.mem.parallel_loop_access !1
  %44 = add nuw nsw i64 %c, %dst_y_step
  %45 = getelementptr double, double* %7, i64 %44
  store double %32, double* %45, align 8, !llvm.mem.parallel_loop_access !1
  %46 = srem i32 %31, %columns
  %47 = add nuw nsw i64 %37, %dst_y_step
  %48 = getelementptr double, double* %7, i64 %47
  %49 = sitofp i32 %46 to double
  store double %49, double* %48, align 8, !llvm.mem.parallel_loop_access !1
  %50 = sdiv i32 %31, %columns
  %51 = add nuw nsw i64 %41, %dst_y_step
  %52 = getelementptr double, double* %7, i64 %51
  %53 = sitofp i32 %50 to double
  store double %53, double* %52, align 8, !llvm.mem.parallel_loop_access !1
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %5
  br i1 %c_postcondition, label %c_exit, label %c_body

c_exit:                                           ; preds = %exit
  %54 = bitcast %u0CXYT* %2 to %f64CXY*
  ret %f64CXY* %54
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
