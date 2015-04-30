; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64CXY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @min_max_loc_tmp_thunk0({ %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 0
  %4 = load %f64CXY*, %f64CXY** %3, align 8
  %5 = getelementptr inbounds { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 1
  %6 = load %f64CXY*, %f64CXY** %5, align 8
  %7 = getelementptr { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 2
  %8 = bitcast i32* %7 to i64*
  %.combined = load i64, i64* %8, align 4
  %combine.extract.trunc = trunc i64 %.combined to i32
  %combine.extract.shift = lshr i64 %.combined, 32
  %combine.extract.trunc17 = trunc i64 %combine.extract.shift to i32
  %9 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 2
  %10 = bitcast i32* %9 to i64*
  %channels.combined = load i64, i64* %10, align 4
  %dst_c = and i64 %channels.combined, 4294967295
  %combine.extract.shift22 = lshr i64 %channels.combined, 32
  %dst_y_step = mul nuw nsw i64 %combine.extract.shift22, %dst_c
  %11 = getelementptr inbounds %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 0
  %12 = ptrtoint double* %11 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  call void @llvm.assume(i1 %14)
  %15 = getelementptr %f64CXY, %f64CXY* %6, i64 0, i32 2
  %16 = bitcast i32* %15 to i64*
  %channels1.combined = load i64, i64* %16, align 4
  %src_c = and i64 %channels1.combined, 4294967295
  %combine.extract.shift19 = lshr i64 %channels1.combined, 32
  %17 = getelementptr inbounds %f64CXY, %f64CXY* %6, i64 0, i32 6, i64 0
  %18 = ptrtoint double* %17 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  %21 = icmp eq i32 %combine.extract.trunc17, 0
  %22 = shl nuw nsw i64 %dst_c, 1
  %23 = icmp eq i32 %combine.extract.trunc, 0
  br label %c_body

c_body:                                           ; preds = %end, %entry
  %c = phi i64 [ %1, %entry ], [ %c_increment, %end ]
  br i1 %21, label %end, label %then

then:                                             ; preds = %c_body, %end6
  %24 = phi i32 [ %66, %end6 ], [ 0, %c_body ]
  %25 = phi i32 [ %67, %end6 ], [ 0, %c_body ]
  %26 = phi i32 [ %68, %end6 ], [ 0, %c_body ]
  %27 = phi i32 [ %69, %end6 ], [ 0, %c_body ]
  %28 = phi double [ %70, %end6 ], [ 0xFFEFFFFFFFFFFFFF, %c_body ]
  %29 = phi double [ %71, %end6 ], [ 0x7FEFFFFFFFFFFFFF, %c_body ]
  %30 = phi i32 [ %72, %end6 ], [ 0, %c_body ]
  br i1 %23, label %end6, label %then5.lr.ph

then5.lr.ph:                                      ; preds = %then
  %31 = sext i32 %30 to i64
  %32 = mul nsw i64 %31, %combine.extract.shift19
  br label %then5

label.end_crit_edge:                              ; preds = %end6
  br label %end

end:                                              ; preds = %label.end_crit_edge, %c_body
  %33 = phi i32 [ %66, %label.end_crit_edge ], [ 0, %c_body ]
  %34 = phi i32 [ %67, %label.end_crit_edge ], [ 0, %c_body ]
  %35 = phi double [ %70, %label.end_crit_edge ], [ 0xFFEFFFFFFFFFFFFF, %c_body ]
  %36 = phi i32 [ %68, %label.end_crit_edge ], [ 0, %c_body ]
  %37 = phi i32 [ %69, %label.end_crit_edge ], [ 0, %c_body ]
  %38 = phi double [ %71, %label.end_crit_edge ], [ 0x7FEFFFFFFFFFFFFF, %c_body ]
  %39 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %c
  store double %38, double* %39, align 8, !llvm.mem.parallel_loop_access !0
  %40 = sitofp i32 %37 to double
  %41 = add nuw nsw i64 %c, %dst_c
  %42 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %41
  store double %40, double* %42, align 8, !llvm.mem.parallel_loop_access !0
  %43 = sitofp i32 %36 to double
  %44 = add nuw nsw i64 %c, %22
  %45 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %44
  store double %43, double* %45, align 8, !llvm.mem.parallel_loop_access !0
  %46 = add nuw nsw i64 %c, %dst_y_step
  %47 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %46
  store double %35, double* %47, align 8, !llvm.mem.parallel_loop_access !0
  %48 = sitofp i32 %34 to double
  %49 = add nuw nsw i64 %41, %dst_y_step
  %50 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %49
  store double %48, double* %50, align 8, !llvm.mem.parallel_loop_access !0
  %51 = sitofp i32 %33 to double
  %52 = add nuw nsw i64 %44, %dst_y_step
  %53 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %52
  store double %51, double* %53, align 8, !llvm.mem.parallel_loop_access !0
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %2
  br i1 %c_postcondition, label %c_exit, label %c_body, !llvm.loop !0

c_exit:                                           ; preds = %end
  ret void

then5:                                            ; preds = %then5.lr.ph, %end10
  %54 = phi i32 [ %24, %then5.lr.ph ], [ %78, %end10 ]
  %55 = phi i32 [ %25, %then5.lr.ph ], [ %79, %end10 ]
  %56 = phi i32 [ %26, %then5.lr.ph ], [ %74, %end10 ]
  %57 = phi i32 [ %27, %then5.lr.ph ], [ %75, %end10 ]
  %58 = phi double [ %28, %then5.lr.ph ], [ %80, %end10 ]
  %59 = phi double [ %29, %then5.lr.ph ], [ %76, %end10 ]
  %60 = phi i32 [ 0, %then5.lr.ph ], [ %81, %end10 ]
  %61 = sext i32 %60 to i64
  %tmp = add i64 %61, %32
  %tmp12 = mul i64 %tmp, %src_c
  %62 = add i64 %tmp12, %c
  %63 = getelementptr %f64CXY, %f64CXY* %6, i64 0, i32 6, i64 %62
  %64 = load double, double* %63, align 8, !llvm.mem.parallel_loop_access !0
  %65 = fcmp olt double %64, %59
  br i1 %65, label %then7, label %end8

label4.end6_crit_edge:                            ; preds = %end10
  br label %end6

end6:                                             ; preds = %label4.end6_crit_edge, %then
  %66 = phi i32 [ %78, %label4.end6_crit_edge ], [ %24, %then ]
  %67 = phi i32 [ %79, %label4.end6_crit_edge ], [ %25, %then ]
  %68 = phi i32 [ %74, %label4.end6_crit_edge ], [ %26, %then ]
  %69 = phi i32 [ %75, %label4.end6_crit_edge ], [ %27, %then ]
  %70 = phi double [ %80, %label4.end6_crit_edge ], [ %28, %then ]
  %71 = phi double [ %76, %label4.end6_crit_edge ], [ %29, %then ]
  %72 = add nuw nsw i32 %30, 1
  %73 = icmp eq i32 %72, %combine.extract.trunc17
  br i1 %73, label %label.end_crit_edge, label %then

then7:                                            ; preds = %then5
  br label %end8

end8:                                             ; preds = %then7, %then5
  %74 = phi i32 [ %30, %then7 ], [ %56, %then5 ]
  %75 = phi i32 [ %60, %then7 ], [ %57, %then5 ]
  %76 = phi double [ %64, %then7 ], [ %59, %then5 ]
  %77 = fcmp ogt double %64, %58
  br i1 %77, label %then9, label %end10

then9:                                            ; preds = %end8
  br label %end10

end10:                                            ; preds = %then9, %end8
  %78 = phi i32 [ %30, %then9 ], [ %54, %end8 ]
  %79 = phi i32 [ %60, %then9 ], [ %55, %end8 ]
  %80 = phi double [ %64, %then9 ], [ %58, %end8 ]
  %81 = add nuw nsw i32 %60, 1
  %82 = icmp eq i32 %81, %combine.extract.trunc
  br i1 %82, label %label4.end6_crit_edge, label %then5
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %f64CXY* @min_max_loc(%f64CXY*) {
entry:
  %1 = getelementptr %f64CXY, %f64CXY* %0, i64 0, i32 2
  %2 = bitcast i32* %1 to i64*
  %channels.combined = load i64, i64* %2, align 4
  %combine.extract.trunc = trunc i64 %channels.combined to i32
  %3 = call %u0CXYT* @likely_new(i32 28992, i32 %combine.extract.trunc, i32 3, i32 2, i32 1, i8* null)
  %4 = bitcast %u0CXYT* %3 to %f64CXY*
  %combine.extract.shift = lshr i64 %channels.combined, 32
  %combine.extract.trunc1 = trunc i64 %combine.extract.shift to i32
  %5 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !1
  %6 = and i64 %channels.combined, 4294967295
  %7 = alloca { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }, align 8
  %8 = bitcast { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }* %7 to %u0CXYT**
  store %u0CXYT* %3, %u0CXYT** %8, align 8
  %9 = getelementptr inbounds { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 1
  store %f64CXY* %0, %f64CXY** %9, align 8
  %10 = getelementptr inbounds { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 2
  store i32 %combine.extract.trunc1, i32* %10, align 8
  %11 = getelementptr inbounds { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 3
  store i32 %rows, i32* %11, align 4
  %12 = getelementptr inbounds { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 4
  store i64 %6, i64* %12, align 8
  %13 = getelementptr inbounds { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 5
  %14 = bitcast i64* %13 to <2 x i64>*
  store <2 x i64> <i64 1, i64 1>, <2 x i64>* %14, align 8
  %15 = getelementptr inbounds { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 7
  store i64 1, i64* %15, align 8
  %16 = bitcast { %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }* %7 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64CXY*, %f64CXY*, i32, i32, i64, i64, i64, i64 }*, i64, i64)* @min_max_loc_tmp_thunk0 to i8*), i8* %16, i64 %6)
  ret %f64CXY* %4
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = distinct !{!0}
!1 = !{i32 1, i32 -1}
