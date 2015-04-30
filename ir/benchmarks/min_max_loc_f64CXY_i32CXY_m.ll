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
  call void @llvm.assume(i1 %16)
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
  %27 = phi i32 [ %78, %end6 ], [ 0, %c_body ]
  %28 = phi i32 [ %79, %end6 ], [ 0, %c_body ]
  %29 = phi i32 [ %80, %end6 ], [ 0, %c_body ]
  %30 = phi i32 [ %81, %end6 ], [ 0, %c_body ]
  %31 = phi double [ %82, %end6 ], [ 0xFFEFFFFFFFFFFFFF, %c_body ]
  %32 = phi double [ %83, %end6 ], [ 0x7FEFFFFFFFFFFFFF, %c_body ]
  %33 = phi i32 [ %84, %end6 ], [ 0, %c_body ]
  br i1 %25, label %end6, label %then5.lr.ph

then5.lr.ph:                                      ; preds = %then
  %34 = sext i32 %33 to i64
  %35 = mul nsw i64 %34, %src_x
  br label %then5

end:                                              ; preds = %end6, %c_body
  %36 = phi i32 [ 0, %c_body ], [ %78, %end6 ]
  %37 = phi i32 [ 0, %c_body ], [ %79, %end6 ]
  %38 = phi double [ 0xFFEFFFFFFFFFFFFF, %c_body ], [ %82, %end6 ]
  %39 = phi i32 [ 0, %c_body ], [ %80, %end6 ]
  %40 = phi i32 [ 0, %c_body ], [ %81, %end6 ]
  %41 = phi double [ 0x7FEFFFFFFFFFFFFF, %c_body ], [ %83, %end6 ]
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

then5:                                            ; preds = %then5.lr.ph, %then5
  %56 = phi i32 [ %27, %then5.lr.ph ], [ %73, %then5 ]
  %57 = phi i32 [ %28, %then5.lr.ph ], [ %74, %then5 ]
  %58 = phi i32 [ %29, %then5.lr.ph ], [ %69, %then5 ]
  %59 = phi i32 [ %30, %then5.lr.ph ], [ %70, %then5 ]
  %60 = phi double [ %31, %then5.lr.ph ], [ %75, %then5 ]
  %61 = phi double [ %32, %then5.lr.ph ], [ %71, %then5 ]
  %62 = phi i32 [ 0, %then5.lr.ph ], [ %76, %then5 ]
  %63 = sext i32 %62 to i64
  %tmp = add i64 %63, %35
  %tmp12 = mul i64 %tmp, %src_c
  %64 = add i64 %tmp12, %c
  %65 = getelementptr %i32CXY, %i32CXY* %6, i64 0, i32 6, i64 %64
  %66 = load i32, i32* %65, align 4, !llvm.mem.parallel_loop_access !1
  %67 = sitofp i32 %66 to double
  %68 = fcmp olt double %67, %61
  %69 = select i1 %68, i32 %33, i32 %58
  %70 = select i1 %68, i32 %62, i32 %59
  %71 = select i1 %68, double %67, double %61
  %72 = fcmp ogt double %67, %60
  %73 = select i1 %72, i32 %33, i32 %56
  %74 = select i1 %72, i32 %62, i32 %57
  %75 = select i1 %72, double %67, double %60
  %76 = add nuw nsw i32 %62, 1
  %77 = icmp eq i32 %76, %8
  br i1 %77, label %end6, label %then5

end6:                                             ; preds = %then5, %then
  %78 = phi i32 [ %27, %then ], [ %73, %then5 ]
  %79 = phi i32 [ %28, %then ], [ %74, %then5 ]
  %80 = phi i32 [ %29, %then ], [ %69, %then5 ]
  %81 = phi i32 [ %30, %then ], [ %70, %then5 ]
  %82 = phi double [ %31, %then ], [ %75, %then5 ]
  %83 = phi double [ %32, %then ], [ %71, %then5 ]
  %84 = add nuw nsw i32 %33, 1
  %85 = icmp eq i32 %84, %10
  br i1 %85, label %end, label %then
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %f64CXY* @min_max_loc(%i32CXY*) {
entry:
  %1 = getelementptr inbounds %i32CXY, %i32CXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 28992, i32 %channels, i32 3, i32 2, i32 1, i8* null)
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
