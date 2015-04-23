; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64CXY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind readonly
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
  tail call void @llvm.assume(i1 %16)
  %17 = getelementptr inbounds %f64CXY, %f64CXY* %6, i64 0, i32 2
  %channels1 = load i32, i32* %17, align 4, !range !0
  %src_c = zext i32 %channels1 to i64
  %18 = getelementptr inbounds %f64CXY, %f64CXY* %6, i64 0, i32 3
  %columns2 = load i32, i32* %18, align 4, !range !0
  %src_x = zext i32 %columns2 to i64
  %src_y_step = mul nuw nsw i64 %src_x, %src_c
  %19 = getelementptr inbounds %f64CXY, %f64CXY* %6, i64 0, i32 6, i64 0
  %20 = ptrtoint double* %19 to i64
  %21 = and i64 %20, 31
  %22 = icmp eq i64 %21, 0
  tail call void @llvm.assume(i1 %22)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %23 = mul nuw nsw i64 %src_y_step, %y
  %24 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %x_body, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %x_body ]
  %25 = add nuw nsw i64 %x, %23
  %26 = getelementptr %f64CXY, %f64CXY* %6, i64 0, i32 6, i64 %25
  %27 = load double, double* %26, align 8, !llvm.mem.parallel_loop_access !1
  %28 = fmul double %8, %27
  %29 = fadd double %10, %28
  %30 = add nuw nsw i64 %x, %24
  %31 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %30
  store double %29, double* %31, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !1

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
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
  %6 = tail call %u0CXYT* @likely_new(i32 28992, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %7 = bitcast %u0CXYT* %6 to %f64CXY*
  %8 = zext i32 %rows to i64
  %9 = alloca { %f64CXY*, %f64CXY*, double, double }, align 8
  %10 = bitcast { %f64CXY*, %f64CXY*, double, double }* %9 to %u0CXYT**
  store %u0CXYT* %6, %u0CXYT** %10, align 8
  %11 = getelementptr inbounds { %f64CXY*, %f64CXY*, double, double }, { %f64CXY*, %f64CXY*, double, double }* %9, i64 0, i32 1
  store %f64CXY* %0, %f64CXY** %11, align 8
  %12 = getelementptr inbounds { %f64CXY*, %f64CXY*, double, double }, { %f64CXY*, %f64CXY*, double, double }* %9, i64 0, i32 2
  store double %1, double* %12, align 8
  %13 = getelementptr inbounds { %f64CXY*, %f64CXY*, double, double }, { %f64CXY*, %f64CXY*, double, double }* %9, i64 0, i32 3
  store double %2, double* %13, align 8
  %14 = bitcast { %f64CXY*, %f64CXY*, double, double }* %9 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64CXY*, %f64CXY*, double, double }*, i64, i64)* @multiply_add_tmp_thunk0 to i8*), i8* %14, i64 %8)
  ret %f64CXY* %7
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
