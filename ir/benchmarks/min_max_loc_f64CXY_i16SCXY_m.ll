; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64CXY = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%i16SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @min_max_loc_tmp_thunk0({ %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 0
  %4 = load %f64CXY*, %f64CXY** %3, align 8
  %5 = getelementptr inbounds { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 1
  %6 = load %i16SCXY*, %i16SCXY** %5, align 8
  %7 = getelementptr inbounds { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 2
  %8 = load i32, i32* %7, align 4
  %9 = getelementptr inbounds { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 3
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
  call void @llvm.assume(i1 %16)
  %17 = getelementptr inbounds %i16SCXY, %i16SCXY* %6, i64 0, i32 2
  %channels1 = load i32, i32* %17, align 4, !range !0
  %src_c = zext i32 %channels1 to i64
  %18 = getelementptr inbounds %i16SCXY, %i16SCXY* %6, i64 0, i32 3
  %columns2 = load i32, i32* %18, align 4, !range !0
  %src_x = zext i32 %columns2 to i64
  %19 = getelementptr inbounds %i16SCXY, %i16SCXY* %6, i64 0, i32 6, i64 0
  %20 = ptrtoint i16* %19 to i64
  %21 = and i64 %20, 31
  %22 = icmp eq i64 %21, 0
  call void @llvm.assume(i1 %22)
  %23 = icmp eq i32 %10, 0
  %24 = shl nuw nsw i64 %dst_c, 1
  %25 = icmp eq i32 %8, 0
  br label %c_body

c_body:                                           ; preds = %end, %entry
  %c = phi i64 [ %1, %entry ], [ %c_increment, %end ]
  %26 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %c
  br i1 %23, label %end, label %label4.preheader

label4.preheader:                                 ; preds = %c_body, %end6
  %27 = phi i32 [ %72, %end6 ], [ 0, %c_body ]
  %28 = phi double [ %.lcssa16, %end6 ], [ 0x7FEFFFFFFFFFFFFF, %c_body ]
  %29 = phi i32 [ %.lcssa15, %end6 ], [ 0, %c_body ]
  %30 = phi i32 [ %.lcssa14, %end6 ], [ 0, %c_body ]
  %31 = phi double [ %.lcssa13, %end6 ], [ 0xFFEFFFFFFFFFFFFF, %c_body ]
  %32 = phi i32 [ %.lcssa12, %end6 ], [ 0, %c_body ]
  %33 = phi i32 [ %.lcssa, %end6 ], [ 0, %c_body ]
  br i1 %25, label %end6, label %then5.lr.ph

then5.lr.ph:                                      ; preds = %label4.preheader
  %34 = sext i32 %27 to i64
  %35 = mul nsw i64 %34, %src_x
  br label %then5

end:                                              ; preds = %end6, %c_body
  %.lcssa22 = phi double [ 0x7FEFFFFFFFFFFFFF, %c_body ], [ %.lcssa16, %end6 ]
  %.lcssa21 = phi i32 [ 0, %c_body ], [ %.lcssa15, %end6 ]
  %.lcssa20 = phi i32 [ 0, %c_body ], [ %.lcssa14, %end6 ]
  %.lcssa19 = phi double [ 0xFFEFFFFFFFFFFFFF, %c_body ], [ %.lcssa13, %end6 ]
  %.lcssa18 = phi i32 [ 0, %c_body ], [ %.lcssa12, %end6 ]
  %.lcssa17 = phi i32 [ 0, %c_body ], [ %.lcssa, %end6 ]
  store double %.lcssa22, double* %26, align 8, !llvm.mem.parallel_loop_access !1
  %36 = sitofp i32 %.lcssa21 to double
  %37 = add nuw nsw i64 %c, %dst_c
  %38 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %37
  store double %36, double* %38, align 8, !llvm.mem.parallel_loop_access !1
  %39 = sitofp i32 %.lcssa20 to double
  %40 = add nuw nsw i64 %c, %24
  %41 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %40
  store double %39, double* %41, align 8, !llvm.mem.parallel_loop_access !1
  %42 = add nuw nsw i64 %c, %dst_y_step
  %43 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %42
  store double %.lcssa19, double* %43, align 8, !llvm.mem.parallel_loop_access !1
  %44 = sitofp i32 %.lcssa18 to double
  %45 = add nuw nsw i64 %37, %dst_y_step
  %46 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %45
  store double %44, double* %46, align 8, !llvm.mem.parallel_loop_access !1
  %47 = sitofp i32 %.lcssa17 to double
  %48 = add nuw nsw i64 %40, %dst_y_step
  %49 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %48
  store double %47, double* %49, align 8, !llvm.mem.parallel_loop_access !1
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %2
  br i1 %c_postcondition, label %c_exit, label %c_body, !llvm.loop !1

c_exit:                                           ; preds = %end
  ret void

then5:                                            ; preds = %then5.lr.ph, %then5
  %50 = phi double [ %28, %then5.lr.ph ], [ %65, %then5 ]
  %51 = phi i32 [ %29, %then5.lr.ph ], [ %64, %then5 ]
  %52 = phi i32 [ %30, %then5.lr.ph ], [ %63, %then5 ]
  %53 = phi double [ %31, %then5.lr.ph ], [ %69, %then5 ]
  %54 = phi i32 [ %32, %then5.lr.ph ], [ %68, %then5 ]
  %55 = phi i32 [ %33, %then5.lr.ph ], [ %67, %then5 ]
  %56 = phi i32 [ 0, %then5.lr.ph ], [ %70, %then5 ]
  %57 = sext i32 %56 to i64
  %tmp = add i64 %57, %35
  %tmp11 = mul i64 %tmp, %src_c
  %58 = add i64 %tmp11, %c
  %59 = getelementptr %i16SCXY, %i16SCXY* %6, i64 0, i32 6, i64 %58
  %60 = load i16, i16* %59, align 2, !llvm.mem.parallel_loop_access !1
  %61 = sitofp i16 %60 to double
  %62 = fcmp olt double %61, %50
  %63 = select i1 %62, i32 %27, i32 %52
  %64 = select i1 %62, i32 %56, i32 %51
  %65 = select i1 %62, double %61, double %50
  %66 = fcmp ogt double %61, %53
  %67 = select i1 %66, i32 %27, i32 %55
  %68 = select i1 %66, i32 %56, i32 %54
  %69 = select i1 %66, double %61, double %53
  %70 = add nuw nsw i32 %56, 1
  %71 = icmp eq i32 %70, %8
  br i1 %71, label %end6, label %then5

end6:                                             ; preds = %then5, %label4.preheader
  %.lcssa16 = phi double [ %28, %label4.preheader ], [ %65, %then5 ]
  %.lcssa15 = phi i32 [ %29, %label4.preheader ], [ %64, %then5 ]
  %.lcssa14 = phi i32 [ %30, %label4.preheader ], [ %63, %then5 ]
  %.lcssa13 = phi double [ %31, %label4.preheader ], [ %69, %then5 ]
  %.lcssa12 = phi i32 [ %32, %label4.preheader ], [ %68, %then5 ]
  %.lcssa = phi i32 [ %33, %label4.preheader ], [ %67, %then5 ]
  %72 = add nuw nsw i32 %27, 1
  %73 = icmp eq i32 %72, %10
  br i1 %73, label %end, label %label4.preheader
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %f64CXY* @min_max_loc(%i16SCXY*) {
entry:
  %1 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 28992, i32 %channels, i32 3, i32 2, i32 1, i8* null)
  %3 = bitcast %u0CXYT* %2 to %f64CXY*
  %4 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 3
  %columns = load i32, i32* %4, align 4, !range !0
  %5 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = zext i32 %channels to i64
  %7 = alloca { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }, align 8
  %8 = bitcast { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }* %7 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %8, align 8
  %9 = getelementptr inbounds { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 1
  store %i16SCXY* %0, %i16SCXY** %9, align 8
  %10 = getelementptr inbounds { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 2
  store i32 %columns, i32* %10, align 8
  %11 = getelementptr inbounds { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 3
  store i32 %rows, i32* %11, align 4
  %12 = getelementptr inbounds { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 4
  store i64 %6, i64* %12, align 8
  %13 = getelementptr inbounds { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 5
  store i64 1, i64* %13, align 8
  %14 = getelementptr inbounds { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 6
  store i64 1, i64* %14, align 8
  %15 = getelementptr inbounds { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 7
  store i64 1, i64* %15, align 8
  %16 = bitcast { %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }* %7 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64CXY*, %i16SCXY*, i32, i32, i64, i64, i64, i64 }*, i64, i64)* @min_max_loc_tmp_thunk0 to i8*), i8* %16, i64 %6)
  ret %f64CXY* %3
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
