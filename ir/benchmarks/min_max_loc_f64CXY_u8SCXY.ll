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
  %16 = shl nuw nsw i64 %5, 2
  %17 = mul nuw nsw i64 %5, 5
  br label %c_body

c_body:                                           ; preds = %end, %entry
  %c = phi i64 [ 0, %entry ], [ %c_increment, %end ]
  br label %then

then:                                             ; preds = %c_body, %end6
  %storemerge26 = phi i32 [ 0, %c_body ], [ %63, %end6 ]
  %18 = phi double [ 0x7FEFFFFFFFFFFFFF, %c_body ], [ %56, %end6 ]
  %19 = phi double [ 0xFFEFFFFFFFFFFFFF, %c_body ], [ %60, %end6 ]
  %20 = phi i32 [ 0, %c_body ], [ %55, %end6 ]
  %21 = phi i32 [ 0, %c_body ], [ %54, %end6 ]
  %22 = phi i32 [ 0, %c_body ], [ %59, %end6 ]
  %23 = phi i32 [ 0, %c_body ], [ %58, %end6 ]
  %24 = sext i32 %storemerge26 to i64
  %25 = mul nsw i64 %24, %src_x
  br label %then5

end:                                              ; preds = %end6
  %26 = getelementptr double, double* %7, i64 %c
  store double %56, double* %26, align 8, !llvm.mem.parallel_loop_access !1
  %27 = sitofp i32 %55 to double
  %28 = add nuw nsw i64 %c, %5
  %29 = getelementptr double, double* %7, i64 %28
  store double %27, double* %29, align 8, !llvm.mem.parallel_loop_access !1
  %30 = sitofp i32 %54 to double
  %31 = add nuw nsw i64 %c, %15
  %32 = getelementptr double, double* %7, i64 %31
  store double %30, double* %32, align 8, !llvm.mem.parallel_loop_access !1
  %33 = add nuw nsw i64 %c, %dst_y_step
  %34 = getelementptr double, double* %7, i64 %33
  store double %60, double* %34, align 8, !llvm.mem.parallel_loop_access !1
  %35 = sitofp i32 %59 to double
  %36 = add nuw nsw i64 %c, %16
  %37 = getelementptr double, double* %7, i64 %36
  store double %35, double* %37, align 8, !llvm.mem.parallel_loop_access !1
  %38 = sitofp i32 %58 to double
  %39 = add nuw nsw i64 %c, %17
  %40 = getelementptr double, double* %7, i64 %39
  store double %38, double* %40, align 8, !llvm.mem.parallel_loop_access !1
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %5
  br i1 %c_postcondition, label %c_exit, label %c_body, !llvm.loop !1

c_exit:                                           ; preds = %end
  %41 = bitcast %u0CXYT* %2 to %f64CXY*
  ret %f64CXY* %41

then5:                                            ; preds = %then, %then5
  %storemerge125 = phi i32 [ 0, %then ], [ %61, %then5 ]
  %42 = phi double [ %18, %then ], [ %56, %then5 ]
  %43 = phi double [ %19, %then ], [ %60, %then5 ]
  %44 = phi i32 [ %20, %then ], [ %55, %then5 ]
  %45 = phi i32 [ %21, %then ], [ %54, %then5 ]
  %46 = phi i32 [ %22, %then ], [ %59, %then5 ]
  %47 = phi i32 [ %23, %then ], [ %58, %then5 ]
  %48 = sext i32 %storemerge125 to i64
  %tmp = add i64 %48, %25
  %tmp2 = mul i64 %tmp, %5
  %49 = add i64 %tmp2, %c
  %50 = getelementptr %u8SCXY, %u8SCXY* %0, i64 0, i32 6, i64 %49
  %51 = load i8, i8* %50, align 1, !llvm.mem.parallel_loop_access !1
  %52 = uitofp i8 %51 to double
  %53 = fcmp olt double %52, %42
  %54 = select i1 %53, i32 %storemerge26, i32 %45
  %55 = select i1 %53, i32 %storemerge125, i32 %44
  %56 = select i1 %53, double %52, double %42
  %57 = fcmp ogt double %52, %43
  %58 = select i1 %57, i32 %storemerge26, i32 %47
  %59 = select i1 %57, i32 %storemerge125, i32 %46
  %60 = select i1 %57, double %52, double %43
  %61 = add nuw nsw i32 %storemerge125, 1
  %62 = icmp eq i32 %61, %columns
  br i1 %62, label %end6, label %then5

end6:                                             ; preds = %then5
  %63 = add nuw nsw i32 %storemerge26, 1
  %64 = icmp eq i32 %63, %rows
  br i1 %64, label %end, label %then
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
