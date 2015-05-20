; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64CXY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind readnone
declare double @llvm.sqrt.f64(double) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
define private void @normalize_l2_tmp_thunk0({ %f64CXY*, %f64CXY*, double }* noalias nocapture readonly, i64, i64) #2 {
entry:
  %3 = getelementptr inbounds { %f64CXY*, %f64CXY*, double }, { %f64CXY*, %f64CXY*, double }* %0, i64 0, i32 0
  %4 = load %f64CXY*, %f64CXY** %3, align 8
  %5 = getelementptr inbounds { %f64CXY*, %f64CXY*, double }, { %f64CXY*, %f64CXY*, double }* %0, i64 0, i32 1
  %6 = load %f64CXY*, %f64CXY** %5, align 8
  %7 = getelementptr inbounds { %f64CXY*, %f64CXY*, double }, { %f64CXY*, %f64CXY*, double }* %0, i64 0, i32 2
  %8 = load double, double* %7, align 8
  %9 = getelementptr inbounds %f64CXY, %f64CXY* %6, i64 0, i32 2
  %channels1 = load i32, i32* %9, align 4, !range !0
  %dst_c = zext i32 %channels1 to i64
  %10 = getelementptr inbounds %f64CXY, %f64CXY* %6, i64 0, i32 3
  %columns2 = load i32, i32* %10, align 4, !range !0
  %dst_x = zext i32 %columns2 to i64
  %11 = getelementptr inbounds %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 0
  %12 = ptrtoint double* %11 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  call void @llvm.assume(i1 %14)
  %15 = getelementptr inbounds %f64CXY, %f64CXY* %6, i64 0, i32 6, i64 0
  %16 = ptrtoint double* %15 to i64
  %17 = and i64 %16, 31
  %18 = icmp eq i64 %17, 0
  call void @llvm.assume(i1 %18)
  %19 = mul i64 %dst_c, %2
  %20 = mul i64 %19, %dst_x
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %y_body ]
  %21 = getelementptr %f64CXY, %f64CXY* %6, i64 0, i32 6, i64 %y
  %22 = load double, double* %21, align 8, !llvm.mem.parallel_loop_access !1
  %23 = fmul fast double %22, %8
  %24 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %y
  store double %23, double* %24, align 8, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %20
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #2

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %f64CXY* @normalize_l2(%f64CXY*) {
entry:
  %1 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = mul nuw nsw i32 %columns, %channels
  %4 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !0
  %5 = mul nuw nsw i32 %3, %rows
  br label %true_entry

true_entry:                                       ; preds = %true_entry, %entry
  %6 = phi i32 [ 0, %entry ], [ %13, %true_entry ]
  %7 = phi double [ 0.000000e+00, %entry ], [ %12, %true_entry ]
  %8 = sext i32 %6 to i64
  %9 = getelementptr %f64CXY, %f64CXY* %0, i64 0, i32 6, i64 %8
  %10 = load double, double* %9, align 8
  %11 = fmul fast double %10, %10
  %12 = fadd fast double %11, %7
  %13 = add nuw nsw i32 %6, 1
  %14 = icmp eq i32 %13, %5
  br i1 %14, label %exit, label %true_entry

exit:                                             ; preds = %true_entry
  %15 = call double @llvm.sqrt.f64(double %12)
  %16 = fdiv fast double 1.000000e+00, %15
  %17 = call %u0CXYT* @likely_new(i32 28992, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %dst = bitcast %u0CXYT* %17 to %f64CXY*
  %18 = zext i32 %rows to i64
  %19 = alloca { %f64CXY*, %f64CXY*, double }, align 8
  %20 = bitcast { %f64CXY*, %f64CXY*, double }* %19 to %u0CXYT**
  store %u0CXYT* %17, %u0CXYT** %20, align 8
  %21 = getelementptr inbounds { %f64CXY*, %f64CXY*, double }, { %f64CXY*, %f64CXY*, double }* %19, i64 0, i32 1
  store %f64CXY* %0, %f64CXY** %21, align 8
  %22 = getelementptr inbounds { %f64CXY*, %f64CXY*, double }, { %f64CXY*, %f64CXY*, double }* %19, i64 0, i32 2
  store double %16, double* %22, align 8
  %23 = bitcast { %f64CXY*, %f64CXY*, double }* %19 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64CXY*, %f64CXY*, double }*, i64, i64)* @normalize_l2_tmp_thunk0 to i8*), i8* %23, i64 %18)
  ret %f64CXY* %dst
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
