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
  br label %then

then:                                             ; preds = %c_body, %end3
  %16 = phi i32 [ 0, %c_body ], [ %59, %end3 ]
  %17 = phi i32 [ 0, %c_body ], [ %60, %end3 ]
  %18 = phi i32 [ 0, %c_body ], [ %55, %end3 ]
  %19 = phi i32 [ 0, %c_body ], [ %56, %end3 ]
  %20 = phi double [ 0xFFEFFFFFFFFFFFFF, %c_body ], [ %61, %end3 ]
  %21 = phi double [ 0x7FEFFFFFFFFFFFFF, %c_body ], [ %57, %end3 ]
  %22 = phi i32 [ 0, %c_body ], [ %53, %end3 ]
  %23 = sext i32 %22 to i64
  %24 = mul nsw i64 %23, %src_x
  br label %then2

end:                                              ; preds = %end3
  %25 = getelementptr double, double* %7, i64 %c
  store double %57, double* %25, align 8, !llvm.mem.parallel_loop_access !1
  %26 = sitofp i32 %56 to double
  %27 = add nuw nsw i64 %c, %5
  %28 = getelementptr double, double* %7, i64 %27
  store double %26, double* %28, align 8, !llvm.mem.parallel_loop_access !1
  %29 = sitofp i32 %55 to double
  %30 = add nuw nsw i64 %c, %15
  %31 = getelementptr double, double* %7, i64 %30
  store double %29, double* %31, align 8, !llvm.mem.parallel_loop_access !1
  %32 = add nuw nsw i64 %c, %dst_y_step
  %33 = getelementptr double, double* %7, i64 %32
  store double %61, double* %33, align 8, !llvm.mem.parallel_loop_access !1
  %34 = sitofp i32 %60 to double
  %35 = add nuw nsw i64 %27, %dst_y_step
  %36 = getelementptr double, double* %7, i64 %35
  store double %34, double* %36, align 8, !llvm.mem.parallel_loop_access !1
  %37 = sitofp i32 %59 to double
  %38 = add nuw nsw i64 %30, %dst_y_step
  %39 = getelementptr double, double* %7, i64 %38
  store double %37, double* %39, align 8, !llvm.mem.parallel_loop_access !1
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %5
  br i1 %c_postcondition, label %c_exit, label %c_body, !llvm.loop !1

c_exit:                                           ; preds = %end
  %40 = bitcast %u0CXYT* %2 to %f64CXY*
  ret %f64CXY* %40

then2:                                            ; preds = %then, %end7
  %41 = phi i32 [ %16, %then ], [ %59, %end7 ]
  %42 = phi i32 [ %17, %then ], [ %60, %end7 ]
  %43 = phi i32 [ %18, %then ], [ %55, %end7 ]
  %44 = phi i32 [ %19, %then ], [ %56, %end7 ]
  %45 = phi double [ %20, %then ], [ %61, %end7 ]
  %46 = phi double [ %21, %then ], [ %57, %end7 ]
  %47 = phi i32 [ 0, %then ], [ %62, %end7 ]
  %48 = sext i32 %47 to i64
  %tmp = add i64 %48, %24
  %tmp2 = mul i64 %tmp, %5
  %49 = add i64 %tmp2, %c
  %50 = getelementptr %f64CXY, %f64CXY* %0, i64 0, i32 6, i64 %49
  %51 = load double, double* %50, align 8, !llvm.mem.parallel_loop_access !1
  %52 = fcmp olt double %51, %46
  br i1 %52, label %then4, label %end5

end3:                                             ; preds = %end7
  %53 = add nuw nsw i32 %22, 1
  %54 = icmp eq i32 %53, %rows
  br i1 %54, label %end, label %then

then4:                                            ; preds = %then2
  br label %end5

end5:                                             ; preds = %then4, %then2
  %55 = phi i32 [ %22, %then4 ], [ %43, %then2 ]
  %56 = phi i32 [ %47, %then4 ], [ %44, %then2 ]
  %57 = phi double [ %51, %then4 ], [ %46, %then2 ]
  %58 = fcmp ogt double %51, %45
  br i1 %58, label %then6, label %end7

then6:                                            ; preds = %end5
  br label %end7

end7:                                             ; preds = %then6, %end5
  %59 = phi i32 [ %22, %then6 ], [ %41, %end5 ]
  %60 = phi i32 [ %47, %then6 ], [ %42, %end5 ]
  %61 = phi double [ %51, %then6 ], [ %45, %end5 ]
  %62 = add nuw nsw i32 %47, 1
  %63 = icmp eq i32 %62, %columns
  br i1 %63, label %end3, label %then2
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
