; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64CXY = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%u8SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

; Function Attrs: nounwind
define %f64CXY* @min_max_loc(%u8SCXY*) #1 {
entry:
  %1 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = tail call %u0CXYT* @likely_new(i32 28992, i32 %channels, i32 3, i32 2, i32 1, i8* null)
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
  tail call void @llvm.assume(i1 %10)
  %src_x = zext i32 %columns to i64
  %11 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 6, i64 0
  %12 = ptrtoint i8* %11 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  tail call void @llvm.assume(i1 %14)
  %15 = shl nuw nsw i64 %5, 1
  br label %c_body

c_body:                                           ; preds = %end, %entry
  %c = phi i64 [ 0, %entry ], [ %c_increment, %end ]
  br label %then

then:                                             ; preds = %c_body, %end3
  %storemerge26 = phi i32 [ 0, %c_body ], [ %61, %end3 ]
  %16 = phi double [ 0x7FEFFFFFFFFFFFFF, %c_body ], [ %54, %end3 ]
  %17 = phi double [ 0xFFEFFFFFFFFFFFFF, %c_body ], [ %58, %end3 ]
  %18 = phi i32 [ 0, %c_body ], [ %53, %end3 ]
  %19 = phi i32 [ 0, %c_body ], [ %52, %end3 ]
  %20 = phi i32 [ 0, %c_body ], [ %57, %end3 ]
  %21 = phi i32 [ 0, %c_body ], [ %56, %end3 ]
  %22 = sext i32 %storemerge26 to i64
  %23 = mul nsw i64 %22, %src_x
  br label %then2

end:                                              ; preds = %end3
  %24 = getelementptr double, double* %7, i64 %c
  store double %54, double* %24, align 8, !llvm.mem.parallel_loop_access !1
  %25 = sitofp i32 %53 to double
  %26 = add nuw nsw i64 %c, %5
  %27 = getelementptr double, double* %7, i64 %26
  store double %25, double* %27, align 8, !llvm.mem.parallel_loop_access !1
  %28 = sitofp i32 %52 to double
  %29 = add nuw nsw i64 %c, %15
  %30 = getelementptr double, double* %7, i64 %29
  store double %28, double* %30, align 8, !llvm.mem.parallel_loop_access !1
  %31 = add nuw nsw i64 %c, %dst_y_step
  %32 = getelementptr double, double* %7, i64 %31
  store double %58, double* %32, align 8, !llvm.mem.parallel_loop_access !1
  %33 = sitofp i32 %57 to double
  %34 = add nuw nsw i64 %26, %dst_y_step
  %35 = getelementptr double, double* %7, i64 %34
  store double %33, double* %35, align 8, !llvm.mem.parallel_loop_access !1
  %36 = sitofp i32 %56 to double
  %37 = add nuw nsw i64 %29, %dst_y_step
  %38 = getelementptr double, double* %7, i64 %37
  store double %36, double* %38, align 8, !llvm.mem.parallel_loop_access !1
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %5
  br i1 %c_postcondition, label %c_exit, label %c_body, !llvm.loop !1

c_exit:                                           ; preds = %end
  %39 = bitcast %u0CXYT* %2 to %f64CXY*
  ret %f64CXY* %39

then2:                                            ; preds = %then, %then2
  %storemerge125 = phi i32 [ 0, %then ], [ %59, %then2 ]
  %40 = phi double [ %16, %then ], [ %54, %then2 ]
  %41 = phi double [ %17, %then ], [ %58, %then2 ]
  %42 = phi i32 [ %18, %then ], [ %53, %then2 ]
  %43 = phi i32 [ %19, %then ], [ %52, %then2 ]
  %44 = phi i32 [ %20, %then ], [ %57, %then2 ]
  %45 = phi i32 [ %21, %then ], [ %56, %then2 ]
  %46 = sext i32 %storemerge125 to i64
  %tmp = add i64 %46, %23
  %tmp2 = mul i64 %tmp, %5
  %47 = add i64 %tmp2, %c
  %48 = getelementptr %u8SCXY, %u8SCXY* %0, i64 0, i32 6, i64 %47
  %49 = load i8, i8* %48, align 1, !llvm.mem.parallel_loop_access !1
  %50 = uitofp i8 %49 to double
  %51 = fcmp olt double %50, %40
  %52 = select i1 %51, i32 %storemerge26, i32 %43
  %53 = select i1 %51, i32 %storemerge125, i32 %42
  %54 = select i1 %51, double %50, double %40
  %55 = fcmp ogt double %50, %41
  %56 = select i1 %55, i32 %storemerge26, i32 %45
  %57 = select i1 %55, i32 %storemerge125, i32 %44
  %58 = select i1 %55, double %50, double %41
  %59 = add nuw nsw i32 %storemerge125, 1
  %60 = icmp eq i32 %59, %columns
  br i1 %60, label %end3, label %then2

end3:                                             ; preds = %then2
  %61 = add nuw nsw i32 %storemerge26, 1
  %62 = icmp eq i32 %61, %rows
  br i1 %62, label %end, label %then
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
