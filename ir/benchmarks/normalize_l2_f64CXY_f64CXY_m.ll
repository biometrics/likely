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
  %9 = getelementptr inbounds %f64CXY, %f64CXY* %4, i64 0, i32 2
  %channels = load i32, i32* %9, align 4, !range !0
  %dst_c = zext i32 %channels to i64
  %10 = getelementptr inbounds %f64CXY, %f64CXY* %4, i64 0, i32 3
  %columns = load i32, i32* %10, align 4, !range !0
  %dst_x = zext i32 %columns to i64
  %11 = getelementptr inbounds %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 0
  %12 = ptrtoint double* %11 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  tail call void @llvm.assume(i1 %14)
  %15 = getelementptr inbounds %f64CXY, %f64CXY* %6, i64 0, i32 2
  %channels1 = load i32, i32* %15, align 4, !range !0
  %src_c = zext i32 %channels1 to i64
  %16 = getelementptr inbounds %f64CXY, %f64CXY* %6, i64 0, i32 3
  %columns2 = load i32, i32* %16, align 4, !range !0
  %src_x = zext i32 %columns2 to i64
  %17 = getelementptr inbounds %f64CXY, %f64CXY* %6, i64 0, i32 6, i64 0
  %18 = ptrtoint double* %17 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  tail call void @llvm.assume(i1 %20)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %21 = mul i64 %y, %src_x
  %22 = mul i64 %y, %dst_x
  br label %x_body

x_body:                                           ; preds = %c_exit, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %c_exit ]
  %tmp = add i64 %x, %21
  %tmp4 = mul i64 %tmp, %src_c
  %tmp5 = add i64 %x, %22
  %tmp6 = mul i64 %tmp5, %dst_c
  br label %c_body

c_body:                                           ; preds = %c_body, %x_body
  %c = phi i64 [ 0, %x_body ], [ %c_increment, %c_body ]
  %23 = add i64 %tmp4, %c
  %24 = getelementptr %f64CXY, %f64CXY* %6, i64 0, i32 6, i64 %23
  %25 = load double, double* %24, align 8, !llvm.mem.parallel_loop_access !1
  %26 = fmul double %8, %25
  %27 = add i64 %tmp6, %c
  %28 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %27
  store double %26, double* %28, align 8, !llvm.mem.parallel_loop_access !1
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %dst_c
  br i1 %c_postcondition, label %c_exit, label %c_body, !llvm.loop !1

c_exit:                                           ; preds = %c_body
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_x
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %c_exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
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
  br label %then

then:                                             ; preds = %entry, %then
  %storemerge3 = phi i32 [ 0, %entry ], [ %12, %then ]
  %6 = phi double [ 0.000000e+00, %entry ], [ %11, %then ]
  %7 = sext i32 %storemerge3 to i64
  %8 = getelementptr %f64CXY, %f64CXY* %0, i64 0, i32 6, i64 %7
  %9 = load double, double* %8, align 8
  %10 = fmul double %9, %9
  %11 = fadd double %6, %10
  %12 = add nuw nsw i32 %storemerge3, 1
  %13 = icmp eq i32 %12, %5
  br i1 %13, label %end, label %then

end:                                              ; preds = %then
  %14 = tail call double @llvm.sqrt.f64(double %11)
  %15 = fdiv double 1.000000e+00, %14
  %16 = tail call %u0CXYT* @likely_new(i32 28992, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %17 = bitcast %u0CXYT* %16 to %f64CXY*
  %18 = zext i32 %rows to i64
  %19 = alloca { %f64CXY*, %f64CXY*, double }, align 8
  %20 = bitcast { %f64CXY*, %f64CXY*, double }* %19 to %u0CXYT**
  store %u0CXYT* %16, %u0CXYT** %20, align 8
  %21 = getelementptr inbounds { %f64CXY*, %f64CXY*, double }, { %f64CXY*, %f64CXY*, double }* %19, i64 0, i32 1
  store %f64CXY* %0, %f64CXY** %21, align 8
  %22 = getelementptr inbounds { %f64CXY*, %f64CXY*, double }, { %f64CXY*, %f64CXY*, double }* %19, i64 0, i32 2
  store double %15, double* %22, align 8
  %23 = bitcast { %f64CXY*, %f64CXY*, double }* %19 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64CXY*, %f64CXY*, double }*, i64, i64)* @normalize_l2_tmp_thunk0 to i8*), i8* %23, i64 %18)
  ret %f64CXY* %17
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
