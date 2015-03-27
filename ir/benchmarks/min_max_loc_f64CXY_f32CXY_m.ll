; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64CXY = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%f32CXY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @min_max_loc_tmp_thunk0({ %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 0
  %4 = load %f64CXY*, %f64CXY** %3, align 8
  %5 = getelementptr inbounds { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 1
  %6 = load %f32CXY*, %f32CXY** %5, align 8
  %7 = getelementptr inbounds { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 2
  %8 = load i32, i32* %7, align 4
  %9 = getelementptr inbounds { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 3
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
  %17 = getelementptr inbounds %f32CXY, %f32CXY* %6, i64 0, i32 2
  %channels1 = load i32, i32* %17, align 4, !range !0
  %src_c = zext i32 %channels1 to i64
  %18 = getelementptr inbounds %f32CXY, %f32CXY* %6, i64 0, i32 3
  %columns2 = load i32, i32* %18, align 4, !range !0
  %src_x = zext i32 %columns2 to i64
  %19 = getelementptr inbounds %f32CXY, %f32CXY* %6, i64 0, i32 6, i64 0
  %20 = ptrtoint float* %19 to i64
  %21 = and i64 %20, 31
  %22 = icmp eq i64 %21, 0
  tail call void @llvm.assume(i1 %22)
  %23 = shl nuw nsw i64 %dst_c, 1
  %24 = add nuw nsw i64 %dst_y_step, %dst_c
  %25 = add nuw nsw i64 %dst_y_step, %23
  br label %c_body

c_body:                                           ; preds = %end, %entry
  %c = phi i64 [ %1, %entry ], [ %c_increment, %end ]
  %26 = icmp eq i32 %10, 0
  br i1 %26, label %end, label %then

then:                                             ; preds = %c_body, %end6
  %storemerge41 = phi i32 [ %72, %end6 ], [ 0, %c_body ]
  %27 = phi double [ %.lcssa28, %end6 ], [ 0x7FEFFFFFFFFFFFFF, %c_body ]
  %28 = phi double [ %.lcssa27, %end6 ], [ 0xFFEFFFFFFFFFFFFF, %c_body ]
  %29 = phi i32 [ %.lcssa26, %end6 ], [ 0, %c_body ]
  %30 = phi i32 [ %.lcssa25, %end6 ], [ 0, %c_body ]
  %31 = phi i32 [ %.lcssa24, %end6 ], [ 0, %c_body ]
  %32 = phi i32 [ %.lcssa, %end6 ], [ 0, %c_body ]
  %33 = sext i32 %storemerge41 to i64
  %34 = mul nsw i64 %33, %src_x
  %35 = icmp eq i32 %8, 0
  br i1 %35, label %end6, label %then5

end:                                              ; preds = %end6, %c_body
  %.lcssa34 = phi double [ 0x7FEFFFFFFFFFFFFF, %c_body ], [ %.lcssa28, %end6 ]
  %.lcssa33 = phi double [ 0xFFEFFFFFFFFFFFFF, %c_body ], [ %.lcssa27, %end6 ]
  %.lcssa32 = phi i32 [ 0, %c_body ], [ %.lcssa26, %end6 ]
  %.lcssa31 = phi i32 [ 0, %c_body ], [ %.lcssa25, %end6 ]
  %.lcssa30 = phi i32 [ 0, %c_body ], [ %.lcssa24, %end6 ]
  %.lcssa29 = phi i32 [ 0, %c_body ], [ %.lcssa, %end6 ]
  %36 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %c
  store double %.lcssa34, double* %36, align 8, !llvm.mem.parallel_loop_access !1
  %37 = sitofp i32 %.lcssa32 to double
  %38 = add nuw nsw i64 %c, %dst_c
  %39 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %38
  store double %37, double* %39, align 8, !llvm.mem.parallel_loop_access !1
  %40 = sitofp i32 %.lcssa31 to double
  %41 = add nuw nsw i64 %c, %23
  %42 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %41
  store double %40, double* %42, align 8, !llvm.mem.parallel_loop_access !1
  %43 = add nuw nsw i64 %c, %dst_y_step
  %44 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %43
  store double %.lcssa33, double* %44, align 8, !llvm.mem.parallel_loop_access !1
  %45 = sitofp i32 %.lcssa30 to double
  %46 = add nuw nsw i64 %24, %c
  %47 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %46
  store double %45, double* %47, align 8, !llvm.mem.parallel_loop_access !1
  %48 = sitofp i32 %.lcssa29 to double
  %49 = add nuw nsw i64 %25, %c
  %50 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %49
  store double %48, double* %50, align 8, !llvm.mem.parallel_loop_access !1
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %2
  br i1 %c_postcondition, label %c_exit, label %c_body, !llvm.loop !1

c_exit:                                           ; preds = %end
  ret void

then5:                                            ; preds = %then, %then5
  %storemerge1135 = phi i32 [ %70, %then5 ], [ 0, %then ]
  %51 = phi double [ %65, %then5 ], [ %27, %then ]
  %52 = phi double [ %69, %then5 ], [ %28, %then ]
  %53 = phi i32 [ %64, %then5 ], [ %29, %then ]
  %54 = phi i32 [ %63, %then5 ], [ %30, %then ]
  %55 = phi i32 [ %68, %then5 ], [ %31, %then ]
  %56 = phi i32 [ %67, %then5 ], [ %32, %then ]
  %57 = sext i32 %storemerge1135 to i64
  %tmp = add i64 %57, %34
  %tmp12 = mul i64 %tmp, %src_c
  %58 = add i64 %tmp12, %c
  %59 = getelementptr %f32CXY, %f32CXY* %6, i64 0, i32 6, i64 %58
  %60 = load float, float* %59, align 4, !llvm.mem.parallel_loop_access !1
  %61 = fpext float %60 to double
  %62 = fcmp olt double %61, %51
  %63 = select i1 %62, i32 %storemerge41, i32 %54
  %64 = select i1 %62, i32 %storemerge1135, i32 %53
  %65 = select i1 %62, double %61, double %51
  %66 = fcmp ogt double %61, %52
  %67 = select i1 %66, i32 %storemerge41, i32 %56
  %68 = select i1 %66, i32 %storemerge1135, i32 %55
  %69 = select i1 %66, double %61, double %52
  %70 = add nuw nsw i32 %storemerge1135, 1
  %71 = icmp eq i32 %70, %8
  br i1 %71, label %end6, label %then5

end6:                                             ; preds = %then5, %then
  %.lcssa28 = phi double [ %27, %then ], [ %65, %then5 ]
  %.lcssa27 = phi double [ %28, %then ], [ %69, %then5 ]
  %.lcssa26 = phi i32 [ %29, %then ], [ %64, %then5 ]
  %.lcssa25 = phi i32 [ %30, %then ], [ %63, %then5 ]
  %.lcssa24 = phi i32 [ %31, %then ], [ %68, %then5 ]
  %.lcssa = phi i32 [ %32, %then ], [ %67, %then5 ]
  %72 = add nuw nsw i32 %storemerge41, 1
  %73 = icmp eq i32 %72, %10
  br i1 %73, label %end, label %then
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %f64CXY* @min_max_loc(%f32CXY*) {
entry:
  %1 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = tail call %u0CXYT* @likely_new(i32 28992, i32 %channels, i32 3, i32 2, i32 1, i8* null)
  %3 = bitcast %u0CXYT* %2 to %f64CXY*
  %4 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 3
  %columns = load i32, i32* %4, align 4, !range !0
  %5 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = zext i32 %channels to i64
  %7 = alloca { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }, align 8
  %8 = bitcast { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }* %7 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %8, align 8
  %9 = getelementptr inbounds { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 1
  store %f32CXY* %0, %f32CXY** %9, align 8
  %10 = getelementptr inbounds { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 2
  store i32 %columns, i32* %10, align 8
  %11 = getelementptr inbounds { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 3
  store i32 %rows, i32* %11, align 4
  %12 = getelementptr inbounds { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 4
  store i64 %6, i64* %12, align 8
  %13 = getelementptr inbounds { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 5
  store i64 1, i64* %13, align 8
  %14 = getelementptr inbounds { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 6
  store i64 1, i64* %14, align 8
  %15 = getelementptr inbounds { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 7
  store i64 1, i64* %15, align 8
  %16 = bitcast { %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }* %7 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64CXY*, %f32CXY*, i32, i32, i64, i64, i64, i64 }*, i64, i64)* @min_max_loc_tmp_thunk0 to i8*), i8* %16, i64 %6)
  ret %f64CXY* %3
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
