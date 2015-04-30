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
  %7 = getelementptr { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 2
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
  %15 = getelementptr %i32CXY, %i32CXY* %6, i64 0, i32 2
  %16 = bitcast i32* %15 to i64*
  %channels1.combined = load i64, i64* %16, align 4
  %src_c = and i64 %channels1.combined, 4294967295
  %combine.extract.shift19 = lshr i64 %channels1.combined, 32
  %17 = getelementptr inbounds %i32CXY, %i32CXY* %6, i64 0, i32 6, i64 0
  %18 = ptrtoint i32* %17 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  %21 = icmp eq i32 %combine.extract.trunc17, 0
  %22 = shl nuw nsw i64 %dst_c, 1
  %23 = icmp eq i32 %combine.extract.trunc, 0
  br label %c_body

c_body:                                           ; preds = %end, %entry
  %c = phi i64 [ %1, %entry ], [ %c_increment, %end ]
  %24 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %c
  br i1 %21, label %end, label %then

then:                                             ; preds = %c_body, %end6
  %25 = phi i32 [ %67, %end6 ], [ 0, %c_body ]
  %26 = phi i32 [ %68, %end6 ], [ 0, %c_body ]
  %27 = phi i32 [ %69, %end6 ], [ 0, %c_body ]
  %28 = phi i32 [ %70, %end6 ], [ 0, %c_body ]
  %29 = phi double [ %71, %end6 ], [ 0xFFEFFFFFFFFFFFFF, %c_body ]
  %30 = phi double [ %72, %end6 ], [ 0x7FEFFFFFFFFFFFFF, %c_body ]
  %31 = phi i32 [ %73, %end6 ], [ 0, %c_body ]
  br i1 %23, label %end6, label %then5.lr.ph

then5.lr.ph:                                      ; preds = %then
  %32 = sext i32 %31 to i64
  %33 = mul nsw i64 %32, %combine.extract.shift19
  br label %then5

label.end_crit_edge:                              ; preds = %end6
  br label %end

end:                                              ; preds = %label.end_crit_edge, %c_body
  %34 = phi i32 [ %67, %label.end_crit_edge ], [ 0, %c_body ]
  %35 = phi i32 [ %68, %label.end_crit_edge ], [ 0, %c_body ]
  %36 = phi double [ %71, %label.end_crit_edge ], [ 0xFFEFFFFFFFFFFFFF, %c_body ]
  %37 = phi i32 [ %69, %label.end_crit_edge ], [ 0, %c_body ]
  %38 = phi i32 [ %70, %label.end_crit_edge ], [ 0, %c_body ]
  %39 = phi double [ %72, %label.end_crit_edge ], [ 0x7FEFFFFFFFFFFFFF, %c_body ]
  store double %39, double* %24, align 8, !llvm.mem.parallel_loop_access !0
  %40 = sitofp i32 %38 to double
  %41 = add nuw nsw i64 %c, %dst_c
  %42 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %41
  store double %40, double* %42, align 8, !llvm.mem.parallel_loop_access !0
  %43 = sitofp i32 %37 to double
  %44 = add nuw nsw i64 %c, %22
  %45 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %44
  store double %43, double* %45, align 8, !llvm.mem.parallel_loop_access !0
  %46 = add nuw nsw i64 %c, %dst_y_step
  %47 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %46
  store double %36, double* %47, align 8, !llvm.mem.parallel_loop_access !0
  %48 = sitofp i32 %35 to double
  %49 = add nuw nsw i64 %41, %dst_y_step
  %50 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %49
  store double %48, double* %50, align 8, !llvm.mem.parallel_loop_access !0
  %51 = sitofp i32 %34 to double
  %52 = add nuw nsw i64 %44, %dst_y_step
  %53 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %52
  store double %51, double* %53, align 8, !llvm.mem.parallel_loop_access !0
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %2
  br i1 %c_postcondition, label %c_exit, label %c_body, !llvm.loop !0

c_exit:                                           ; preds = %end
  ret void

then5:                                            ; preds = %then5.lr.ph, %end10
  %54 = phi i32 [ %25, %then5.lr.ph ], [ %79, %end10 ]
  %55 = phi i32 [ %26, %then5.lr.ph ], [ %80, %end10 ]
  %56 = phi i32 [ %27, %then5.lr.ph ], [ %75, %end10 ]
  %57 = phi i32 [ %28, %then5.lr.ph ], [ %76, %end10 ]
  %58 = phi double [ %29, %then5.lr.ph ], [ %81, %end10 ]
  %59 = phi double [ %30, %then5.lr.ph ], [ %77, %end10 ]
  %60 = phi i32 [ 0, %then5.lr.ph ], [ %82, %end10 ]
  %61 = sext i32 %60 to i64
  %tmp = add i64 %61, %33
  %tmp12 = mul i64 %tmp, %src_c
  %62 = add i64 %tmp12, %c
  %63 = getelementptr %i32CXY, %i32CXY* %6, i64 0, i32 6, i64 %62
  %64 = load i32, i32* %63, align 4, !llvm.mem.parallel_loop_access !0
  %65 = sitofp i32 %64 to double
  %66 = fcmp olt double %65, %59
  br i1 %66, label %then7, label %end8

label4.end6_crit_edge:                            ; preds = %end10
  br label %end6

end6:                                             ; preds = %label4.end6_crit_edge, %then
  %67 = phi i32 [ %79, %label4.end6_crit_edge ], [ %25, %then ]
  %68 = phi i32 [ %80, %label4.end6_crit_edge ], [ %26, %then ]
  %69 = phi i32 [ %75, %label4.end6_crit_edge ], [ %27, %then ]
  %70 = phi i32 [ %76, %label4.end6_crit_edge ], [ %28, %then ]
  %71 = phi double [ %81, %label4.end6_crit_edge ], [ %29, %then ]
  %72 = phi double [ %77, %label4.end6_crit_edge ], [ %30, %then ]
  %73 = add nuw nsw i32 %31, 1
  %74 = icmp eq i32 %73, %combine.extract.trunc17
  br i1 %74, label %label.end_crit_edge, label %then

then7:                                            ; preds = %then5
  br label %end8

end8:                                             ; preds = %then7, %then5
  %75 = phi i32 [ %31, %then7 ], [ %56, %then5 ]
  %76 = phi i32 [ %60, %then7 ], [ %57, %then5 ]
  %77 = phi double [ %65, %then7 ], [ %59, %then5 ]
  %78 = fcmp ogt double %65, %58
  br i1 %78, label %then9, label %end10

then9:                                            ; preds = %end8
  br label %end10

end10:                                            ; preds = %then9, %end8
  %79 = phi i32 [ %31, %then9 ], [ %54, %end8 ]
  %80 = phi i32 [ %60, %then9 ], [ %55, %end8 ]
  %81 = phi double [ %65, %then9 ], [ %58, %end8 ]
  %82 = add nuw nsw i32 %60, 1
  %83 = icmp eq i32 %82, %combine.extract.trunc
  br i1 %83, label %label4.end6_crit_edge, label %then5
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %f64CXY* @min_max_loc(%i32CXY*) {
entry:
  %1 = getelementptr %i32CXY, %i32CXY* %0, i64 0, i32 2
  %2 = bitcast i32* %1 to i64*
  %channels.combined = load i64, i64* %2, align 4
  %combine.extract.trunc = trunc i64 %channels.combined to i32
  %3 = call %u0CXYT* @likely_new(i32 28992, i32 %combine.extract.trunc, i32 3, i32 2, i32 1, i8* null)
  %4 = bitcast %u0CXYT* %3 to %f64CXY*
  %combine.extract.shift = lshr i64 %channels.combined, 32
  %combine.extract.trunc1 = trunc i64 %combine.extract.shift to i32
  %5 = getelementptr inbounds %i32CXY, %i32CXY* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !1
  %6 = and i64 %channels.combined, 4294967295
  %7 = alloca { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }, align 8
  %8 = bitcast { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }* %7 to %u0CXYT**
  store %u0CXYT* %3, %u0CXYT** %8, align 8
  %9 = getelementptr inbounds { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 1
  store %i32CXY* %0, %i32CXY** %9, align 8
  %10 = getelementptr inbounds { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %i32CXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 2
  store i32 %combine.extract.trunc1, i32* %10, align 8
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
  ret %f64CXY* %4
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = distinct !{!0}
!1 = !{i32 1, i32 -1}
