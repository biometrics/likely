; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64CXY = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%i16SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f64CXY* @min_max_loc(%i16SCXY*) {
entry:
  %1 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 28992, i32 %channels, i32 3, i32 2, i32 1, i8* null)
  %3 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 3
  %columns = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !0
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
  %15 = mul nuw nsw i32 %rows, %columns
  %16 = shl nuw nsw i64 %5, 1
  br label %c_body

c_body:                                           ; preds = %exit, %entry
  %c = phi i64 [ 0, %entry ], [ %c_increment, %exit ]
  br label %true_enry

true_enry:                                        ; preds = %c_body, %true_enry
  %17 = phi i32 [ 0, %c_body ], [ %31, %true_enry ]
  %18 = phi i16 [ 32767, %c_body ], [ %.1, %true_enry ]
  %19 = phi i32 [ 0, %c_body ], [ %., %true_enry ]
  %20 = phi i16 [ -32768, %c_body ], [ %30, %true_enry ]
  %21 = phi i32 [ 0, %c_body ], [ %29, %true_enry ]
  %22 = sext i32 %17 to i64
  %23 = mul nuw nsw i64 %22, %5
  %24 = add nuw nsw i64 %23, %c
  %25 = getelementptr %i16SCXY, %i16SCXY* %0, i64 0, i32 6, i64 %24
  %26 = load i16, i16* %25, align 2, !llvm.mem.parallel_loop_access !1
  %27 = icmp slt i16 %26, %18
  %. = select i1 %27, i32 %17, i32 %19
  %.1 = select i1 %27, i16 %26, i16 %18
  %28 = icmp sgt i16 %26, %20
  %29 = select i1 %28, i32 %17, i32 %21
  %30 = select i1 %28, i16 %26, i16 %20
  %31 = add nuw nsw i32 %17, 1
  %32 = icmp eq i32 %31, %15
  br i1 %32, label %exit, label %true_enry

exit:                                             ; preds = %true_enry
  %33 = sitofp i16 %.1 to double
  %34 = getelementptr double, double* %7, i64 %c
  store double %33, double* %34, align 8, !llvm.mem.parallel_loop_access !1
  %35 = srem i32 %., %columns
  %36 = sitofp i32 %35 to double
  %37 = add nuw nsw i64 %c, %5
  %38 = getelementptr double, double* %7, i64 %37
  store double %36, double* %38, align 8, !llvm.mem.parallel_loop_access !1
  %39 = sdiv i32 %., %columns
  %40 = sitofp i32 %39 to double
  %41 = add nuw nsw i64 %c, %16
  %42 = getelementptr double, double* %7, i64 %41
  store double %40, double* %42, align 8, !llvm.mem.parallel_loop_access !1
  %43 = sitofp i16 %30 to double
  %44 = add nuw nsw i64 %c, %dst_y_step
  %45 = getelementptr double, double* %7, i64 %44
  store double %43, double* %45, align 8, !llvm.mem.parallel_loop_access !1
  %46 = srem i32 %29, %columns
  %47 = sitofp i32 %46 to double
  %48 = add nuw nsw i64 %37, %dst_y_step
  %49 = getelementptr double, double* %7, i64 %48
  store double %47, double* %49, align 8, !llvm.mem.parallel_loop_access !1
  %50 = sdiv i32 %29, %columns
  %51 = sitofp i32 %50 to double
  %52 = add nuw nsw i64 %41, %dst_y_step
  %53 = getelementptr double, double* %7, i64 %52
  store double %51, double* %53, align 8, !llvm.mem.parallel_loop_access !1
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %5
  br i1 %c_postcondition, label %c_exit, label %c_body, !llvm.loop !1

c_exit:                                           ; preds = %exit
  %54 = bitcast %u0CXYT* %2 to %f64CXY*
  ret %f64CXY* %54
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
