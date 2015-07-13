; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64CXY = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%i16SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: nounwind argmemonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f64CXY* @min_max_loc(%i16SCXY*) {
entry:
  %1 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 28992, i32 %channels, i32 3, i32 2, i32 1, i8* null)
  %3 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 3
  %width = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 4
  %height = load i32, i32* %4, align 4, !range !0
  %5 = zext i32 %channels to i64
  %dst_y_step = mul nuw nsw i64 %5, 3
  %6 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %7 = bitcast %u0CXYT* %6 to double*
  %8 = ptrtoint %u0CXYT* %6 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  %11 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 6, i64 0
  %12 = ptrtoint i16* %11 to i64
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
  %17 = phi i32 [ %30, %true_entry ], [ 0, %c_body ]
  %18 = phi i16 [ %current-value., %true_entry ], [ 32767, %c_body ]
  %19 = phi i32 [ %., %true_entry ], [ 0, %c_body ]
  %20 = phi i16 [ %29, %true_entry ], [ -32768, %c_body ]
  %21 = phi i32 [ %28, %true_entry ], [ 0, %c_body ]
  %22 = sext i32 %17 to i64
  %23 = mul nuw nsw i64 %22, %5
  %24 = add nuw nsw i64 %23, %c
  %25 = getelementptr %i16SCXY, %i16SCXY* %0, i64 0, i32 6, i64 %24
  %current-value = load i16, i16* %25, align 2, !llvm.mem.parallel_loop_access !1
  %26 = icmp slt i16 %current-value, %18
  %. = select i1 %26, i32 %17, i32 %19
  %current-value. = select i1 %26, i16 %current-value, i16 %18
  %27 = icmp sgt i16 %current-value, %20
  %28 = select i1 %27, i32 %17, i32 %21
  %29 = select i1 %27, i16 %current-value, i16 %20
  %30 = add nuw nsw i32 %17, 1
  %31 = icmp eq i32 %30, %15
  br i1 %31, label %exit, label %true_entry

exit:                                             ; preds = %true_entry
  %32 = getelementptr double, double* %7, i64 %c
  %33 = sitofp i16 %current-value. to double
  store double %33, double* %32, align 8, !llvm.mem.parallel_loop_access !1
  %34 = srem i32 %., %width
  %35 = add nuw nsw i64 %c, %5
  %36 = getelementptr double, double* %7, i64 %35
  %37 = sitofp i32 %34 to double
  store double %37, double* %36, align 8, !llvm.mem.parallel_loop_access !1
  %38 = sdiv i32 %., %width
  %39 = add nuw nsw i64 %c, %16
  %40 = getelementptr double, double* %7, i64 %39
  %41 = sitofp i32 %38 to double
  store double %41, double* %40, align 8, !llvm.mem.parallel_loop_access !1
  %42 = add nuw nsw i64 %c, %dst_y_step
  %43 = getelementptr double, double* %7, i64 %42
  %44 = sitofp i16 %29 to double
  store double %44, double* %43, align 8, !llvm.mem.parallel_loop_access !1
  %45 = srem i32 %28, %width
  %46 = add nuw nsw i64 %35, %dst_y_step
  %47 = getelementptr double, double* %7, i64 %46
  %48 = sitofp i32 %45 to double
  store double %48, double* %47, align 8, !llvm.mem.parallel_loop_access !1
  %49 = sdiv i32 %28, %width
  %50 = add nuw nsw i64 %39, %dst_y_step
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

attributes #0 = { nounwind argmemonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
