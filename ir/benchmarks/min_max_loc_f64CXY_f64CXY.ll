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
  %src_x = zext i32 %columns to i64
  %11 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 6, i64 0
  %12 = ptrtoint double* %11 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  call void @llvm.assume(i1 %14)
  %15 = shl nuw nsw i64 %5, 1
  br label %c_body

c_body:                                           ; preds = %end, %entry
  %c = phi i64 [ 0, %entry ], [ %c_increment, %end ]
  br label %label8.preheader

label8.preheader:                                 ; preds = %c_body, %end10
  %16 = phi i32 [ 0, %c_body ], [ %62, %end10 ]
  %17 = phi double [ 0x7FEFFFFFFFFFFFFF, %c_body ], [ %55, %end10 ]
  %18 = phi i32 [ 0, %c_body ], [ %54, %end10 ]
  %19 = phi i32 [ 0, %c_body ], [ %53, %end10 ]
  %20 = phi double [ 0xFFEFFFFFFFFFFFFF, %c_body ], [ %59, %end10 ]
  %21 = phi i32 [ 0, %c_body ], [ %58, %end10 ]
  %22 = phi i32 [ 0, %c_body ], [ %57, %end10 ]
  %23 = sext i32 %16 to i64
  %24 = mul nsw i64 %23, %src_x
  br label %then9

end:                                              ; preds = %end10
  %25 = getelementptr double, double* %7, i64 %c
  store double %55, double* %25, align 8, !llvm.mem.parallel_loop_access !1
  %26 = sitofp i32 %54 to double
  %27 = add nuw nsw i64 %c, %5
  %28 = getelementptr double, double* %7, i64 %27
  store double %26, double* %28, align 8, !llvm.mem.parallel_loop_access !1
  %29 = sitofp i32 %53 to double
  %30 = add nuw nsw i64 %c, %15
  %31 = getelementptr double, double* %7, i64 %30
  store double %29, double* %31, align 8, !llvm.mem.parallel_loop_access !1
  %32 = add nuw nsw i64 %c, %dst_y_step
  %33 = getelementptr double, double* %7, i64 %32
  store double %59, double* %33, align 8, !llvm.mem.parallel_loop_access !1
  %34 = sitofp i32 %58 to double
  %35 = add nuw nsw i64 %27, %dst_y_step
  %36 = getelementptr double, double* %7, i64 %35
  store double %34, double* %36, align 8, !llvm.mem.parallel_loop_access !1
  %37 = sitofp i32 %57 to double
  %38 = add nuw nsw i64 %30, %dst_y_step
  %39 = getelementptr double, double* %7, i64 %38
  store double %37, double* %39, align 8, !llvm.mem.parallel_loop_access !1
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %5
  br i1 %c_postcondition, label %c_exit, label %c_body, !llvm.loop !1

c_exit:                                           ; preds = %end
  %40 = bitcast %u0CXYT* %2 to %f64CXY*
  ret %f64CXY* %40

then9:                                            ; preds = %label8.preheader, %then9
  %41 = phi double [ %17, %label8.preheader ], [ %55, %then9 ]
  %42 = phi i32 [ %18, %label8.preheader ], [ %54, %then9 ]
  %43 = phi i32 [ %19, %label8.preheader ], [ %53, %then9 ]
  %44 = phi double [ %20, %label8.preheader ], [ %59, %then9 ]
  %45 = phi i32 [ %21, %label8.preheader ], [ %58, %then9 ]
  %46 = phi i32 [ %22, %label8.preheader ], [ %57, %then9 ]
  %47 = phi i32 [ 0, %label8.preheader ], [ %60, %then9 ]
  %48 = sext i32 %47 to i64
  %tmp = add i64 %48, %24
  %tmp1 = mul i64 %tmp, %5
  %49 = add i64 %tmp1, %c
  %50 = getelementptr %f64CXY, %f64CXY* %0, i64 0, i32 6, i64 %49
  %51 = load double, double* %50, align 8, !llvm.mem.parallel_loop_access !1
  %52 = fcmp olt double %51, %41
  %53 = select i1 %52, i32 %16, i32 %43
  %54 = select i1 %52, i32 %47, i32 %42
  %55 = select i1 %52, double %51, double %41
  %56 = fcmp ogt double %51, %44
  %57 = select i1 %56, i32 %16, i32 %46
  %58 = select i1 %56, i32 %47, i32 %45
  %59 = select i1 %56, double %51, double %44
  %60 = add nuw nsw i32 %47, 1
  %61 = icmp eq i32 %60, %columns
  br i1 %61, label %end10, label %then9

end10:                                            ; preds = %then9
  %62 = add nuw nsw i32 %16, 1
  %63 = icmp eq i32 %62, %rows
  br i1 %63, label %end, label %label8.preheader
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
