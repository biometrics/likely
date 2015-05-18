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
  %11 = getelementptr inbounds %f32CXY, %f32CXY* %6, i64 0, i32 2
  %channels1 = load i32, i32* %11, align 4, !range !0
  %dst_c = zext i32 %channels1 to i64
  %12 = getelementptr inbounds %f64CXY, %f64CXY* %4, i64 0, i32 3
  %columns = load i32, i32* %12, align 4, !range !0
  %dst_x = zext i32 %columns to i64
  %dst_y_step = mul nuw nsw i64 %dst_x, %dst_c
  %13 = getelementptr inbounds %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 0
  %14 = ptrtoint double* %13 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  call void @llvm.assume(i1 %16)
  %17 = getelementptr inbounds %f32CXY, %f32CXY* %6, i64 0, i32 6, i64 0
  %18 = ptrtoint float* %17 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  %21 = mul nuw nsw i32 %10, %8
  %22 = icmp eq i32 %21, 0
  %23 = shl nuw nsw i64 %dst_c, 1
  br label %c_body

c_body:                                           ; preds = %exit, %entry
  %c = phi i64 [ %1, %entry ], [ %c_increment, %exit ]
  br i1 %22, label %exit, label %true_enry

true_enry:                                        ; preds = %c_body, %true_enry
  %24 = phi i32 [ %40, %true_enry ], [ 0, %c_body ]
  %25 = phi float [ %36, %true_enry ], [ 0x47EFFFFFE0000000, %c_body ]
  %26 = phi i32 [ %35, %true_enry ], [ 0, %c_body ]
  %27 = phi float [ %39, %true_enry ], [ 0xC7EFFFFFE0000000, %c_body ]
  %28 = phi i32 [ %38, %true_enry ], [ 0, %c_body ]
  %29 = sext i32 %24 to i64
  %30 = mul nuw nsw i64 %29, %dst_c
  %31 = add nuw nsw i64 %30, %c
  %32 = getelementptr %f32CXY, %f32CXY* %6, i64 0, i32 6, i64 %31
  %33 = load float, float* %32, align 4, !llvm.mem.parallel_loop_access !1
  %34 = fcmp olt float %33, %25
  %35 = select i1 %34, i32 %24, i32 %26
  %36 = select i1 %34, float %33, float %25
  %37 = fcmp ogt float %33, %27
  %38 = select i1 %37, i32 %24, i32 %28
  %39 = select i1 %37, float %33, float %27
  %40 = add nuw nsw i32 %24, 1
  %41 = icmp eq i32 %40, %21
  br i1 %41, label %exit, label %true_enry

exit:                                             ; preds = %true_enry, %c_body
  %.lcssa10 = phi float [ 0x47EFFFFFE0000000, %c_body ], [ %36, %true_enry ]
  %.lcssa9 = phi i32 [ 0, %c_body ], [ %35, %true_enry ]
  %.lcssa8 = phi float [ 0xC7EFFFFFE0000000, %c_body ], [ %39, %true_enry ]
  %.lcssa = phi i32 [ 0, %c_body ], [ %38, %true_enry ]
  %42 = fpext float %.lcssa10 to double
  %43 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %c
  store double %42, double* %43, align 8, !llvm.mem.parallel_loop_access !1
  %44 = srem i32 %.lcssa9, %8
  %45 = sitofp i32 %44 to double
  %46 = add nuw nsw i64 %c, %dst_c
  %47 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %46
  store double %45, double* %47, align 8, !llvm.mem.parallel_loop_access !1
  %48 = sdiv i32 %.lcssa9, %8
  %49 = sitofp i32 %48 to double
  %50 = add nuw nsw i64 %c, %23
  %51 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %50
  store double %49, double* %51, align 8, !llvm.mem.parallel_loop_access !1
  %52 = fpext float %.lcssa8 to double
  %53 = add nuw nsw i64 %c, %dst_y_step
  %54 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %53
  store double %52, double* %54, align 8, !llvm.mem.parallel_loop_access !1
  %55 = srem i32 %.lcssa, %8
  %56 = sitofp i32 %55 to double
  %57 = add nuw nsw i64 %46, %dst_y_step
  %58 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %57
  store double %56, double* %58, align 8, !llvm.mem.parallel_loop_access !1
  %59 = sdiv i32 %.lcssa, %8
  %60 = sitofp i32 %59 to double
  %61 = add nuw nsw i64 %50, %dst_y_step
  %62 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %61
  store double %60, double* %62, align 8, !llvm.mem.parallel_loop_access !1
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %2
  br i1 %c_postcondition, label %c_exit, label %c_body, !llvm.loop !1

c_exit:                                           ; preds = %exit
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %f64CXY* @min_max_loc(%f32CXY*) {
entry:
  %1 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 28992, i32 %channels, i32 3, i32 2, i32 1, i8* null)
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
