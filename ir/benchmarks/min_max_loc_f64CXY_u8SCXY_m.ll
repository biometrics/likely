; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64CXY = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%u8SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @min_max_loc_tmp_thunk0({ %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 0
  %4 = load %f64CXY*, %f64CXY** %3, align 8
  %5 = getelementptr inbounds { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 1
  %6 = load %u8SCXY*, %u8SCXY** %5, align 8
  %7 = getelementptr inbounds { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 2
  %8 = load i32, i32* %7, align 4
  %9 = getelementptr inbounds { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %0, i64 0, i32 3
  %10 = load i32, i32* %9, align 4
  %11 = getelementptr inbounds %u8SCXY, %u8SCXY* %6, i64 0, i32 2
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
  %17 = getelementptr inbounds %u8SCXY, %u8SCXY* %6, i64 0, i32 6, i64 0
  %18 = ptrtoint i8* %17 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  %21 = mul nuw nsw i32 %10, %8
  %22 = icmp eq i32 %21, 0
  %23 = shl nuw nsw i64 %dst_c, 1
  br label %c_body

c_body:                                           ; preds = %end, %entry
  %c = phi i64 [ %1, %entry ], [ %c_increment, %end ]
  br i1 %22, label %end, label %then

then:                                             ; preds = %c_body, %then
  %24 = phi i32 [ %38, %then ], [ 0, %c_body ]
  %25 = phi i8 [ %.8, %then ], [ -1, %c_body ]
  %26 = phi i32 [ %., %then ], [ 0, %c_body ]
  %27 = phi i8 [ %37, %then ], [ 0, %c_body ]
  %28 = phi i32 [ %36, %then ], [ 0, %c_body ]
  %29 = sext i32 %24 to i64
  %30 = mul nuw nsw i64 %29, %dst_c
  %31 = add nuw nsw i64 %30, %c
  %32 = getelementptr %u8SCXY, %u8SCXY* %6, i64 0, i32 6, i64 %31
  %33 = load i8, i8* %32, align 1, !llvm.mem.parallel_loop_access !1
  %34 = icmp ult i8 %33, %25
  %. = select i1 %34, i32 %24, i32 %26
  %.8 = select i1 %34, i8 %33, i8 %25
  %35 = icmp ugt i8 %33, %27
  %36 = select i1 %35, i32 %24, i32 %28
  %37 = select i1 %35, i8 %33, i8 %27
  %38 = add nuw nsw i32 %24, 1
  %39 = icmp eq i32 %38, %21
  br i1 %39, label %end, label %then

end:                                              ; preds = %then, %c_body
  %.lcssa11 = phi i8 [ -1, %c_body ], [ %.8, %then ]
  %.lcssa10 = phi i32 [ 0, %c_body ], [ %., %then ]
  %.lcssa9 = phi i8 [ 0, %c_body ], [ %37, %then ]
  %.lcssa = phi i32 [ 0, %c_body ], [ %36, %then ]
  %40 = uitofp i8 %.lcssa11 to double
  %41 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %c
  store double %40, double* %41, align 8, !llvm.mem.parallel_loop_access !1
  %42 = srem i32 %.lcssa10, %8
  %43 = sitofp i32 %42 to double
  %44 = add nuw nsw i64 %c, %dst_c
  %45 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %44
  store double %43, double* %45, align 8, !llvm.mem.parallel_loop_access !1
  %46 = sdiv i32 %.lcssa10, %8
  %47 = sitofp i32 %46 to double
  %48 = add nuw nsw i64 %c, %23
  %49 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %48
  store double %47, double* %49, align 8, !llvm.mem.parallel_loop_access !1
  %50 = uitofp i8 %.lcssa9 to double
  %51 = add nuw nsw i64 %c, %dst_y_step
  %52 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %51
  store double %50, double* %52, align 8, !llvm.mem.parallel_loop_access !1
  %53 = srem i32 %.lcssa, %8
  %54 = sitofp i32 %53 to double
  %55 = add nuw nsw i64 %44, %dst_y_step
  %56 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %55
  store double %54, double* %56, align 8, !llvm.mem.parallel_loop_access !1
  %57 = sdiv i32 %.lcssa, %8
  %58 = sitofp i32 %57 to double
  %59 = add nuw nsw i64 %48, %dst_y_step
  %60 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %59
  store double %58, double* %60, align 8, !llvm.mem.parallel_loop_access !1
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %2
  br i1 %c_postcondition, label %c_exit, label %c_body, !llvm.loop !1

c_exit:                                           ; preds = %end
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %f64CXY* @min_max_loc(%u8SCXY*) {
entry:
  %1 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 28992, i32 %channels, i32 3, i32 2, i32 1, i8* null)
  %3 = bitcast %u0CXYT* %2 to %f64CXY*
  %4 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 3
  %columns = load i32, i32* %4, align 4, !range !0
  %5 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = zext i32 %channels to i64
  %7 = alloca { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, align 8
  %8 = bitcast { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %7 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %8, align 8
  %9 = getelementptr inbounds { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 1
  store %u8SCXY* %0, %u8SCXY** %9, align 8
  %10 = getelementptr inbounds { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 2
  store i32 %columns, i32* %10, align 8
  %11 = getelementptr inbounds { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 3
  store i32 %rows, i32* %11, align 4
  %12 = getelementptr inbounds { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 4
  store i64 %6, i64* %12, align 8
  %13 = getelementptr inbounds { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 5
  store i64 1, i64* %13, align 8
  %14 = getelementptr inbounds { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 6
  store i64 1, i64* %14, align 8
  %15 = getelementptr inbounds { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %7, i64 0, i32 7
  store i64 1, i64* %15, align 8
  %16 = bitcast { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %7 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }*, i64, i64)* @min_max_loc_tmp_thunk0 to i8*), i8* %16, i64 %6)
  ret %f64CXY* %3
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
