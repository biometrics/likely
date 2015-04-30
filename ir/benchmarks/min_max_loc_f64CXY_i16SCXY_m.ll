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
  br i1 %23, label %end, label %then

then:                                             ; preds = %c_body, %end6
  %27 = phi i32 [ %69, %end6 ], [ 0, %c_body ]
  %28 = phi i32 [ %70, %end6 ], [ 0, %c_body ]
  %29 = phi i32 [ %71, %end6 ], [ 0, %c_body ]
  %30 = phi i32 [ %72, %end6 ], [ 0, %c_body ]
  %31 = phi double [ %73, %end6 ], [ 0xFFEFFFFFFFFFFFFF, %c_body ]
  %32 = phi double [ %74, %end6 ], [ 0x7FEFFFFFFFFFFFFF, %c_body ]
  %33 = phi i32 [ %75, %end6 ], [ 0, %c_body ]
  br i1 %25, label %end6, label %then5.lr.ph

then5.lr.ph:                                      ; preds = %then
  %34 = sext i32 %33 to i64
  %35 = mul nsw i64 %34, %src_x
  br label %then5

label.end_crit_edge:                              ; preds = %end6
  br label %end

end:                                              ; preds = %label.end_crit_edge, %c_body
  %36 = phi i32 [ %69, %label.end_crit_edge ], [ 0, %c_body ]
  %37 = phi i32 [ %70, %label.end_crit_edge ], [ 0, %c_body ]
  %38 = phi double [ %73, %label.end_crit_edge ], [ 0xFFEFFFFFFFFFFFFF, %c_body ]
  %39 = phi i32 [ %71, %label.end_crit_edge ], [ 0, %c_body ]
  %40 = phi i32 [ %72, %label.end_crit_edge ], [ 0, %c_body ]
  %41 = phi double [ %74, %label.end_crit_edge ], [ 0x7FEFFFFFFFFFFFFF, %c_body ]
  store double %41, double* %26, align 8, !llvm.mem.parallel_loop_access !1
  %42 = sitofp i32 %40 to double
  %43 = add nuw nsw i64 %c, %dst_c
  %44 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %43
  store double %42, double* %44, align 8, !llvm.mem.parallel_loop_access !1
  %45 = sitofp i32 %39 to double
  %46 = add nuw nsw i64 %c, %24
  %47 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %46
  store double %45, double* %47, align 8, !llvm.mem.parallel_loop_access !1
  %48 = add nuw nsw i64 %c, %dst_y_step
  %49 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %48
  store double %38, double* %49, align 8, !llvm.mem.parallel_loop_access !1
  %50 = sitofp i32 %37 to double
  %51 = add nuw nsw i64 %43, %dst_y_step
  %52 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %51
  store double %50, double* %52, align 8, !llvm.mem.parallel_loop_access !1
  %53 = sitofp i32 %36 to double
  %54 = add nuw nsw i64 %46, %dst_y_step
  %55 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %54
  store double %53, double* %55, align 8, !llvm.mem.parallel_loop_access !1
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %2
  br i1 %c_postcondition, label %c_exit, label %c_body, !llvm.loop !1

c_exit:                                           ; preds = %end
  ret void

then5:                                            ; preds = %then5.lr.ph, %end10
  %56 = phi i32 [ %27, %then5.lr.ph ], [ %81, %end10 ]
  %57 = phi i32 [ %28, %then5.lr.ph ], [ %82, %end10 ]
  %58 = phi i32 [ %29, %then5.lr.ph ], [ %77, %end10 ]
  %59 = phi i32 [ %30, %then5.lr.ph ], [ %78, %end10 ]
  %60 = phi double [ %31, %then5.lr.ph ], [ %83, %end10 ]
  %61 = phi double [ %32, %then5.lr.ph ], [ %79, %end10 ]
  %62 = phi i32 [ 0, %then5.lr.ph ], [ %84, %end10 ]
  %63 = sext i32 %62 to i64
  %tmp = add i64 %63, %35
  %tmp12 = mul i64 %tmp, %src_c
  %64 = add i64 %tmp12, %c
  %65 = getelementptr %i16SCXY, %i16SCXY* %6, i64 0, i32 6, i64 %64
  %66 = load i16, i16* %65, align 2, !llvm.mem.parallel_loop_access !1
  %67 = sitofp i16 %66 to double
  %68 = fcmp olt double %67, %61
  br i1 %68, label %then7, label %end8

label4.end6_crit_edge:                            ; preds = %end10
  br label %end6

end6:                                             ; preds = %label4.end6_crit_edge, %then
  %69 = phi i32 [ %81, %label4.end6_crit_edge ], [ %27, %then ]
  %70 = phi i32 [ %82, %label4.end6_crit_edge ], [ %28, %then ]
  %71 = phi i32 [ %77, %label4.end6_crit_edge ], [ %29, %then ]
  %72 = phi i32 [ %78, %label4.end6_crit_edge ], [ %30, %then ]
  %73 = phi double [ %83, %label4.end6_crit_edge ], [ %31, %then ]
  %74 = phi double [ %79, %label4.end6_crit_edge ], [ %32, %then ]
  %75 = add nuw nsw i32 %33, 1
  %76 = icmp eq i32 %75, %10
  br i1 %76, label %label.end_crit_edge, label %then

then7:                                            ; preds = %then5
  br label %end8

end8:                                             ; preds = %then7, %then5
  %77 = phi i32 [ %33, %then7 ], [ %58, %then5 ]
  %78 = phi i32 [ %62, %then7 ], [ %59, %then5 ]
  %79 = phi double [ %67, %then7 ], [ %61, %then5 ]
  %80 = fcmp ogt double %67, %60
  br i1 %80, label %then9, label %end10

then9:                                            ; preds = %end8
  br label %end10

end10:                                            ; preds = %then9, %end8
  %81 = phi i32 [ %33, %then9 ], [ %56, %end8 ]
  %82 = phi i32 [ %62, %then9 ], [ %57, %end8 ]
  %83 = phi double [ %67, %then9 ], [ %60, %end8 ]
  %84 = add nuw nsw i32 %62, 1
  %85 = icmp eq i32 %84, %8
  br i1 %85, label %label4.end6_crit_edge, label %then5
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
