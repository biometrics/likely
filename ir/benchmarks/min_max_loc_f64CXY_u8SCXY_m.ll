; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64CXY = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%u8SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: norecurse nounwind
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
  %22 = shl nuw nsw i64 %dst_c, 1
  %23 = icmp eq i32 %21, 0
  br label %c_body

c_body:                                           ; preds = %exit, %entry
  %c = phi i64 [ %1, %entry ], [ %c_increment, %exit ]
  br i1 %23, label %exit, label %true_entry

true_entry:                                       ; preds = %c_body, %true_entry
  %24 = phi i32 [ %37, %true_entry ], [ 0, %c_body ]
  %25 = phi i8 [ %current-value., %true_entry ], [ -1, %c_body ]
  %26 = phi i32 [ %., %true_entry ], [ 0, %c_body ]
  %27 = phi i8 [ %36, %true_entry ], [ 0, %c_body ]
  %28 = phi i32 [ %35, %true_entry ], [ 0, %c_body ]
  %29 = sext i32 %24 to i64
  %30 = mul nuw nsw i64 %29, %dst_c
  %31 = add nuw nsw i64 %30, %c
  %32 = getelementptr %u8SCXY, %u8SCXY* %6, i64 0, i32 6, i64 %31
  %current-value = load i8, i8* %32, align 1, !llvm.mem.parallel_loop_access !1
  %33 = icmp ult i8 %current-value, %25
  %. = select i1 %33, i32 %24, i32 %26
  %current-value. = select i1 %33, i8 %current-value, i8 %25
  %34 = icmp ugt i8 %current-value, %27
  %35 = select i1 %34, i32 %24, i32 %28
  %36 = select i1 %34, i8 %current-value, i8 %27
  %37 = add nuw nsw i32 %24, 1
  %38 = icmp eq i32 %37, %21
  br i1 %38, label %exit, label %true_entry

exit:                                             ; preds = %true_entry, %c_body
  %.lcssa10 = phi i8 [ -1, %c_body ], [ %current-value., %true_entry ]
  %.lcssa9 = phi i32 [ 0, %c_body ], [ %., %true_entry ]
  %.lcssa8 = phi i8 [ 0, %c_body ], [ %36, %true_entry ]
  %.lcssa = phi i32 [ 0, %c_body ], [ %35, %true_entry ]
  %39 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %c
  %40 = uitofp i8 %.lcssa10 to double
  store double %40, double* %39, align 8, !llvm.mem.parallel_loop_access !1
  %41 = srem i32 %.lcssa9, %8
  %42 = add nuw nsw i64 %c, %dst_c
  %43 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %42
  %44 = sitofp i32 %41 to double
  store double %44, double* %43, align 8, !llvm.mem.parallel_loop_access !1
  %45 = sdiv i32 %.lcssa9, %8
  %46 = add nuw nsw i64 %c, %22
  %47 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %46
  %48 = sitofp i32 %45 to double
  store double %48, double* %47, align 8, !llvm.mem.parallel_loop_access !1
  %49 = add nuw nsw i64 %c, %dst_y_step
  %50 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %49
  %51 = uitofp i8 %.lcssa8 to double
  store double %51, double* %50, align 8, !llvm.mem.parallel_loop_access !1
  %52 = srem i32 %.lcssa, %8
  %53 = add nuw nsw i64 %42, %dst_y_step
  %54 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %53
  %55 = sitofp i32 %52 to double
  store double %55, double* %54, align 8, !llvm.mem.parallel_loop_access !1
  %56 = sdiv i32 %.lcssa, %8
  %57 = add nuw nsw i64 %46, %dst_y_step
  %58 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %57
  %59 = sitofp i32 %56 to double
  store double %59, double* %58, align 8, !llvm.mem.parallel_loop_access !1
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %2
  br i1 %c_postcondition, label %c_exit, label %c_body

c_exit:                                           ; preds = %exit
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #2

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %f64CXY* @min_max_loc(%u8SCXY*) {
entry:
  %1 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 28992, i32 %channels, i32 3, i32 2, i32 1, i8* null)
  %dst = bitcast %u0CXYT* %2 to %f64CXY*
  %3 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 3
  %width = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 4
  %height = load i32, i32* %4, align 4, !range !0
  %5 = zext i32 %channels to i64
  %6 = alloca { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, align 8
  %7 = bitcast { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %6 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %7, align 8
  %8 = getelementptr inbounds { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %6, i64 0, i32 1
  store %u8SCXY* %0, %u8SCXY** %8, align 8
  %9 = getelementptr inbounds { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %6, i64 0, i32 2
  store i32 %width, i32* %9, align 8
  %10 = getelementptr inbounds { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %6, i64 0, i32 3
  store i32 %height, i32* %10, align 4
  %11 = getelementptr inbounds { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %6, i64 0, i32 4
  store i64 %5, i64* %11, align 8
  %12 = getelementptr inbounds { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %6, i64 0, i32 5
  store i64 1, i64* %12, align 8
  %13 = getelementptr inbounds { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %6, i64 0, i32 6
  store i64 1, i64* %13, align 8
  %14 = getelementptr inbounds { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }, { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %6, i64 0, i32 7
  store i64 1, i64* %14, align 8
  %15 = bitcast { %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }* %6 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64CXY*, %u8SCXY*, i32, i32, i64, i64, i64, i64 }*, i64, i64)* @min_max_loc_tmp_thunk0 to i8*), i8* %15, i64 %5)
  ret %f64CXY* %dst
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { norecurse nounwind }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
