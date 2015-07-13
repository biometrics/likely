; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64CXY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind argmemonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @multiply_add_tmp_thunk0({ %f64CXY*, %f64CXY*, double, double }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64CXY*, %f64CXY*, double, double }, { %f64CXY*, %f64CXY*, double, double }* %0, i64 0, i32 0
  %4 = load %f64CXY*, %f64CXY** %3, align 8
  %5 = getelementptr inbounds { %f64CXY*, %f64CXY*, double, double }, { %f64CXY*, %f64CXY*, double, double }* %0, i64 0, i32 1
  %6 = load %f64CXY*, %f64CXY** %5, align 8
  %7 = getelementptr inbounds { %f64CXY*, %f64CXY*, double, double }, { %f64CXY*, %f64CXY*, double, double }* %0, i64 0, i32 2
  %8 = load double, double* %7, align 8
  %9 = getelementptr inbounds { %f64CXY*, %f64CXY*, double, double }, { %f64CXY*, %f64CXY*, double, double }* %0, i64 0, i32 3
  %10 = load double, double* %9, align 8
  %11 = getelementptr inbounds %f64CXY, %f64CXY* %6, i64 0, i32 2
  %channels1 = load i32, i32* %11, align 4, !range !0
  %dst_c = zext i32 %channels1 to i64
  %12 = getelementptr inbounds %f64CXY, %f64CXY* %6, i64 0, i32 3
  %columns2 = load i32, i32* %12, align 4, !range !0
  %dst_x = zext i32 %columns2 to i64
  %13 = getelementptr inbounds %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 0
  %14 = ptrtoint double* %13 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  call void @llvm.assume(i1 %16)
  %17 = getelementptr inbounds %f64CXY, %f64CXY* %6, i64 0, i32 6, i64 0
  %18 = ptrtoint double* %17 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  %21 = mul i64 %dst_c, %2
  %22 = mul i64 %21, %dst_x
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %y_body ]
  %23 = getelementptr %f64CXY, %f64CXY* %6, i64 0, i32 6, i64 %y
  %24 = load double, double* %23, align 8, !llvm.mem.parallel_loop_access !1
  %25 = fmul fast double %24, %8
  %val = fadd fast double %25, %10
  %26 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %y
  store double %val, double* %26, align 8, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %22
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %f64CXY* @multiply_add(%f64CXY*, double, double) {
entry:
  %3 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 2
  %channels = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 3
  %columns = load i32, i32* %4, align 4, !range !0
  %5 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = call %u0CXYT* @likely_new(i32 28992, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %dst = bitcast %u0CXYT* %6 to %f64CXY*
  %7 = zext i32 %rows to i64
  %8 = alloca { %f64CXY*, %f64CXY*, double, double }, align 8
  %9 = bitcast { %f64CXY*, %f64CXY*, double, double }* %8 to %u0CXYT**
  store %u0CXYT* %6, %u0CXYT** %9, align 8
  %10 = getelementptr inbounds { %f64CXY*, %f64CXY*, double, double }, { %f64CXY*, %f64CXY*, double, double }* %8, i64 0, i32 1
  store %f64CXY* %0, %f64CXY** %10, align 8
  %11 = getelementptr inbounds { %f64CXY*, %f64CXY*, double, double }, { %f64CXY*, %f64CXY*, double, double }* %8, i64 0, i32 2
  store double %1, double* %11, align 8
  %12 = getelementptr inbounds { %f64CXY*, %f64CXY*, double, double }, { %f64CXY*, %f64CXY*, double, double }* %8, i64 0, i32 3
  store double %2, double* %12, align 8
  %13 = bitcast { %f64CXY*, %f64CXY*, double, double }* %8 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64CXY*, %f64CXY*, double, double }*, i64, i64)* @multiply_add_tmp_thunk0 to i8*), i8* %13, i64 %7)
  ret %f64CXY* %dst
}

attributes #0 = { nounwind argmemonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
