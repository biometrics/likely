; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64CXY = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%i32CXY = type { i32, i32, i32, i32, i32, i32, [0 x i32] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @min_max_loc_tmp_thunk0({ %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 0
  %4 = load %f64CXY*, %f64CXY** %3, align 8
  %5 = getelementptr inbounds { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 1
  %6 = load %i32CXY*, %i32CXY** %5, align 8
  %7 = getelementptr inbounds { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 2
  %8 = load i32, i32* %7, align 4
  %9 = getelementptr inbounds { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 3
  %10 = load i32, i32* %9, align 4
  %11 = getelementptr inbounds %f64CXY, %f64CXY* %4, i64 0, i32 2
  %channels = load i32, i32* %11, align 4, !range !0
  %dst_c = zext i32 %channels to i64
  %12 = getelementptr inbounds %f64CXY, %f64CXY* %4, i64 0, i32 3
  %columns = load i32, i32* %12, align 4, !range !0
  %dst_x = zext i32 %columns to i64
  %dst_y_step = mul nuw nsw i64 %dst_x, %dst_c
  %13 = getelementptr inbounds %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 0
  %14 = ptrtoint double* %13 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  tail call void @llvm.assume(i1 %16)
  %17 = getelementptr inbounds %i32CXY, %i32CXY* %6, i64 0, i32 2
  %channels1 = load i32, i32* %17, align 4, !range !0
  %src_c = zext i32 %channels1 to i64
  %18 = getelementptr inbounds %i32CXY, %i32CXY* %6, i64 0, i32 3
  %columns2 = load i32, i32* %18, align 4, !range !0
  %src_x = zext i32 %columns2 to i64
  %19 = getelementptr inbounds %i32CXY, %i32CXY* %6, i64 0, i32 6, i64 0
  %20 = ptrtoint i32* %19 to i64
  %21 = and i64 %20, 31
  %22 = icmp eq i64 %21, 0
  tail call void @llvm.assume(i1 %22)
  %23 = shl nuw nsw i64 %dst_c, 1
  br label %c_body

c_body:                                           ; preds = %end, %entry
  %c = phi i64 [ %1, %entry ], [ %c_increment, %end ]
  %24 = icmp eq i32 %10, 0
  br i1 %24, label %end, label %then

then:                                             ; preds = %c_body, %end6
  %storemerge41 = phi i32 [ %70, %end6 ], [ 0, %c_body ]
  %25 = phi double [ %.lcssa28, %end6 ], [ 0x7FEFFFFFFFFFFFFF, %c_body ]
  %26 = phi double [ %.lcssa27, %end6 ], [ 0xFFEFFFFFFFFFFFFF, %c_body ]
  %27 = phi i32 [ %.lcssa26, %end6 ], [ 0, %c_body ]
  %28 = phi i32 [ %.lcssa25, %end6 ], [ 0, %c_body ]
  %29 = phi i32 [ %.lcssa24, %end6 ], [ 0, %c_body ]
  %30 = phi i32 [ %.lcssa, %end6 ], [ 0, %c_body ]
  %31 = sext i32 %storemerge41 to i64
  %32 = mul nsw i64 %31, %src_x
  %33 = icmp eq i32 %8, 0
  br i1 %33, label %end6, label %then5

end:                                              ; preds = %end6, %c_body
  %.lcssa34 = phi double [ 0x7FEFFFFFFFFFFFFF, %c_body ], [ %.lcssa28, %end6 ]
  %.lcssa33 = phi double [ 0xFFEFFFFFFFFFFFFF, %c_body ], [ %.lcssa27, %end6 ]
  %.lcssa32 = phi i32 [ 0, %c_body ], [ %.lcssa26, %end6 ]
  %.lcssa31 = phi i32 [ 0, %c_body ], [ %.lcssa25, %end6 ]
  %.lcssa30 = phi i32 [ 0, %c_body ], [ %.lcssa24, %end6 ]
  %.lcssa29 = phi i32 [ 0, %c_body ], [ %.lcssa, %end6 ]
  %34 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %c
  store double %.lcssa34, double* %34, align 8, !llvm.mem.parallel_loop_access !1
  %35 = sitofp i32 %.lcssa32 to double
  %36 = add nuw nsw i64 %c, %dst_c
  %37 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %36
  store double %35, double* %37, align 8, !llvm.mem.parallel_loop_access !1
  %38 = sitofp i32 %.lcssa31 to double
  %39 = add nuw nsw i64 %c, %23
  %40 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %39
  store double %38, double* %40, align 8, !llvm.mem.parallel_loop_access !1
  %41 = add nuw nsw i64 %c, %dst_y_step
  %42 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %41
  store double %.lcssa33, double* %42, align 8, !llvm.mem.parallel_loop_access !1
  %43 = sitofp i32 %.lcssa30 to double
  %44 = add nuw nsw i64 %36, %dst_y_step
  %45 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %44
  store double %43, double* %45, align 8, !llvm.mem.parallel_loop_access !1
  %46 = sitofp i32 %.lcssa29 to double
  %47 = add nuw nsw i64 %39, %dst_y_step
  %48 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %47
  store double %46, double* %48, align 8, !llvm.mem.parallel_loop_access !1
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %2
  br i1 %c_postcondition, label %c_exit, label %c_body, !llvm.loop !1

c_exit:                                           ; preds = %end
  ret void

then5:                                            ; preds = %then, %then5
  %storemerge1135 = phi i32 [ %68, %then5 ], [ 0, %then ]
  %49 = phi double [ %63, %then5 ], [ %25, %then ]
  %50 = phi double [ %67, %then5 ], [ %26, %then ]
  %51 = phi i32 [ %62, %then5 ], [ %27, %then ]
  %52 = phi i32 [ %61, %then5 ], [ %28, %then ]
  %53 = phi i32 [ %66, %then5 ], [ %29, %then ]
  %54 = phi i32 [ %65, %then5 ], [ %30, %then ]
  %55 = sext i32 %storemerge1135 to i64
  %tmp = add i64 %55, %32
  %tmp12 = mul i64 %tmp, %src_c
  %56 = add i64 %tmp12, %c
  %57 = getelementptr %i32CXY, %i32CXY* %6, i64 0, i32 6, i64 %56
  %58 = load i32, i32* %57, align 4, !llvm.mem.parallel_loop_access !1
  %59 = sitofp i32 %58 to double
  %60 = fcmp olt double %59, %49
  %61 = select i1 %60, i32 %storemerge41, i32 %52
  %62 = select i1 %60, i32 %storemerge1135, i32 %51
  %63 = select i1 %60, double %59, double %49
  %64 = fcmp ogt double %59, %50
  %65 = select i1 %64, i32 %storemerge41, i32 %54
  %66 = select i1 %64, i32 %storemerge1135, i32 %53
  %67 = select i1 %64, double %59, double %50
  %68 = add nuw nsw i32 %storemerge1135, 1
  %69 = icmp eq i32 %68, %8
  br i1 %69, label %end6, label %then5

end6:                                             ; preds = %then5, %then
  %.lcssa28 = phi double [ %25, %then ], [ %63, %then5 ]
  %.lcssa27 = phi double [ %26, %then ], [ %67, %then5 ]
  %.lcssa26 = phi i32 [ %27, %then ], [ %62, %then5 ]
  %.lcssa25 = phi i32 [ %28, %then ], [ %61, %then5 ]
  %.lcssa24 = phi i32 [ %29, %then ], [ %66, %then5 ]
  %.lcssa = phi i32 [ %30, %then ], [ %65, %then5 ]
  %70 = add nuw nsw i32 %storemerge41, 1
  %71 = icmp eq i32 %70, %10
  br i1 %71, label %end, label %then
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %f64CXY* @min_max_loc(%i32CXY*) {
entry:
  %1 = getelementptr inbounds %i32CXY, %i32CXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = tail call %u0CXYT* @likely_new(i32 28992, i32 %channels, i32 3, i32 2, i32 1, i8* null)
  %3 = bitcast %u0CXYT* %2 to %f64CXY*
  %4 = getelementptr inbounds %i32CXY, %i32CXY* %0, i64 0, i32 3
  %columns = load i32, i32* %4, align 4, !range !0
  %5 = getelementptr inbounds %i32CXY, %i32CXY* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = zext i32 %channels to i64
  %7 = alloca { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }, align 8
  %8 = bitcast { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }* %7 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %8, align 8
  %9 = getelementptr inbounds { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 1
  store %i32CXY* %0, %i32CXY** %9, align 8
  %10 = getelementptr inbounds { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 2
  store i32 %columns, i32* %10, align 8
  %11 = getelementptr inbounds { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 3
  store i32 %rows, i32* %11, align 4
  %12 = getelementptr inbounds { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 4
  store i64 %6, i64* %12, align 8
  %13 = getelementptr inbounds { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 5
  store i64 1, i64* %13, align 8
  %14 = getelementptr inbounds { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 6
  store i64 1, i64* %14, align 8
  %15 = getelementptr inbounds { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 7
  store i64 1, i64* %15, align 8
  %16 = bitcast { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }* %7 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }*, i64, i64)* @min_max_loc_tmp_thunk0 to i8*), i8* %16, i64 %6)
  ret %f64CXY* %3
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
