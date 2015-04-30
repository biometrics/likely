; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64CXY = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%u8SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f64CXY* @min_max_loc(%u8SCXY*) {
entry:
  %1 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 28992, i32 %channels, i32 3, i32 2, i32 1, i8* null)
  %3 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 3
  %columns = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 4
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
  %11 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 6, i64 0
  %12 = ptrtoint i8* %11 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  call void @llvm.assume(i1 %14)
  %15 = shl nuw nsw i64 %5, 1
  br label %c_body

c_body:                                           ; preds = %end, %entry
  %c = phi i64 [ 0, %entry ], [ %c_increment, %end ]
  br label %then

then:                                             ; preds = %c_body, %end3
  %16 = phi i32 [ 0, %c_body ], [ %58, %end3 ]
  %17 = phi i32 [ 0, %c_body ], [ %59, %end3 ]
  %18 = phi i32 [ 0, %c_body ], [ %54, %end3 ]
  %19 = phi i32 [ 0, %c_body ], [ %55, %end3 ]
  %20 = phi double [ 0xFFEFFFFFFFFFFFFF, %c_body ], [ %60, %end3 ]
  %21 = phi double [ 0x7FEFFFFFFFFFFFFF, %c_body ], [ %56, %end3 ]
  %22 = phi i32 [ 0, %c_body ], [ %63, %end3 ]
  %23 = sext i32 %22 to i64
  %24 = mul nsw i64 %23, %src_x
  br label %then2

end:                                              ; preds = %end3
  %25 = getelementptr double, double* %7, i64 %c
  store double %56, double* %25, align 8, !llvm.mem.parallel_loop_access !1
  %26 = sitofp i32 %55 to double
  %27 = add nuw nsw i64 %c, %5
  %28 = getelementptr double, double* %7, i64 %27
  store double %26, double* %28, align 8, !llvm.mem.parallel_loop_access !1
  %29 = sitofp i32 %54 to double
  %30 = add nuw nsw i64 %c, %15
  %31 = getelementptr double, double* %7, i64 %30
  store double %29, double* %31, align 8, !llvm.mem.parallel_loop_access !1
  %32 = add nuw nsw i64 %c, %dst_y_step
  %33 = getelementptr double, double* %7, i64 %32
  store double %60, double* %33, align 8, !llvm.mem.parallel_loop_access !1
  %34 = sitofp i32 %59 to double
  %35 = add nuw nsw i64 %27, %dst_y_step
  %36 = getelementptr double, double* %7, i64 %35
  store double %34, double* %36, align 8, !llvm.mem.parallel_loop_access !1
  %37 = sitofp i32 %58 to double
  %38 = add nuw nsw i64 %30, %dst_y_step
  %39 = getelementptr double, double* %7, i64 %38
  store double %37, double* %39, align 8, !llvm.mem.parallel_loop_access !1
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %5
  br i1 %c_postcondition, label %c_exit, label %c_body, !llvm.loop !1

c_exit:                                           ; preds = %end
  %40 = bitcast %u0CXYT* %2 to %f64CXY*
  ret %f64CXY* %40

then2:                                            ; preds = %then, %then2
  %41 = phi i32 [ %16, %then ], [ %58, %then2 ]
  %42 = phi i32 [ %17, %then ], [ %59, %then2 ]
  %43 = phi i32 [ %18, %then ], [ %54, %then2 ]
  %44 = phi i32 [ %19, %then ], [ %55, %then2 ]
  %45 = phi double [ %20, %then ], [ %60, %then2 ]
  %46 = phi double [ %21, %then ], [ %56, %then2 ]
  %47 = phi i32 [ 0, %then ], [ %61, %then2 ]
  %48 = sext i32 %47 to i64
  %tmp = add i64 %48, %24
  %tmp2 = mul i64 %tmp, %5
  %49 = add i64 %tmp2, %c
  %50 = getelementptr %u8SCXY, %u8SCXY* %0, i64 0, i32 6, i64 %49
  %51 = load i8, i8* %50, align 1, !llvm.mem.parallel_loop_access !1
  %52 = uitofp i8 %51 to double
  %53 = fcmp olt double %52, %46
  %54 = select i1 %53, i32 %22, i32 %43
  %55 = select i1 %53, i32 %47, i32 %44
  %56 = select i1 %53, double %52, double %46
  %57 = fcmp ogt double %52, %45
  %58 = select i1 %57, i32 %22, i32 %41
  %59 = select i1 %57, i32 %47, i32 %42
  %60 = select i1 %57, double %52, double %45
  %61 = add nuw nsw i32 %47, 1
  %62 = icmp eq i32 %61, %columns
  br i1 %62, label %end3, label %then2

end3:                                             ; preds = %then2
  %63 = add nuw nsw i32 %22, 1
  %64 = icmp eq i32 %63, %rows
  br i1 %64, label %end, label %then
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
