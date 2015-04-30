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
  %9 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 2
  %10 = bitcast i32* %9 to i64*
  %channels.combined = load i64, i64* %10, align 4
  %dst_c = and i64 %channels.combined, 4294967295
  %combine.extract.shift9 = lshr i64 %channels.combined, 32
  %11 = getelementptr inbounds %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 0
  %12 = ptrtoint double* %11 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  call void @llvm.assume(i1 %14)
  %15 = getelementptr %f64CXY, %f64CXY* %6, i64 0, i32 2
  %16 = bitcast i32* %15 to i64*
  %channels1.combined = load i64, i64* %16, align 4
  %src_c = and i64 %channels1.combined, 4294967295
  %combine.extract.shift = lshr i64 %channels1.combined, 32
  %17 = getelementptr inbounds %f64CXY, %f64CXY* %6, i64 0, i32 6, i64 0
  %18 = ptrtoint double* %17 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %21 = mul i64 %y, %combine.extract.shift
  %22 = mul i64 %y, %combine.extract.shift9
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
  %25 = load double, double* %24, align 8, !llvm.mem.parallel_loop_access !0
  %26 = fmul double %8, %25
  %27 = add i64 %tmp6, %c
  %28 = getelementptr %f64CXY, %f64CXY* %4, i64 0, i32 6, i64 %27
  store double %26, double* %28, align 8, !llvm.mem.parallel_loop_access !0
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %dst_c
  br i1 %c_postcondition, label %c_exit, label %c_body, !llvm.loop !0

c_exit:                                           ; preds = %c_body
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %combine.extract.shift9
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
  %1 = getelementptr %f64CXY, %f64CXY* %0, i64 0, i32 2
  %2 = bitcast i32* %1 to i64*
  %channels.combined = load i64, i64* %2, align 4
  %combine.extract.trunc = trunc i64 %channels.combined to i32
  %combine.extract.shift = lshr i64 %channels.combined, 32
  %combine.extract.trunc4 = trunc i64 %combine.extract.shift to i32
  %3 = mul i32 %combine.extract.trunc4, %combine.extract.trunc
  %4 = getelementptr inbounds %f64CXY, %f64CXY* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !1
  %5 = mul i32 %3, %rows
  br label %then

then:                                             ; preds = %entry, %then
  %6 = phi i32 [ 0, %entry ], [ %13, %then ]
  %7 = phi double [ 0.000000e+00, %entry ], [ %12, %then ]
  %8 = sext i32 %6 to i64
  %9 = getelementptr %f64CXY, %f64CXY* %0, i64 0, i32 6, i64 %8
  %10 = load double, double* %9, align 8
  %11 = fmul double %10, %10
  %12 = fadd double %7, %11
  %13 = add nuw nsw i32 %6, 1
  %14 = icmp eq i32 %13, %5
  br i1 %14, label %end, label %then

end:                                              ; preds = %then
  %15 = call double @llvm.sqrt.f64(double %12)
  %16 = fdiv double 1.000000e+00, %15
  %17 = call %u0CXYT* @likely_new(i32 28992, i32 %combine.extract.trunc, i32 %combine.extract.trunc4, i32 %rows, i32 1, i8* null)
  %18 = bitcast %u0CXYT* %17 to %f64CXY*
  %19 = zext i32 %rows to i64
  %20 = alloca { %f64CXY*, %f64CXY*, double }, align 8
  %21 = bitcast { %f64CXY*, %f64CXY*, double }* %20 to %u0CXYT**
  store %u0CXYT* %17, %u0CXYT** %21, align 8
  %22 = getelementptr inbounds { %f64CXY*, %f64CXY*, double }, { %f64CXY*, %f64CXY*, double }* %20, i64 0, i32 1
  store %f64CXY* %0, %f64CXY** %22, align 8
  %23 = getelementptr inbounds { %f64CXY*, %f64CXY*, double }, { %f64CXY*, %f64CXY*, double }* %20, i64 0, i32 2
  store double %16, double* %23, align 8
  %24 = bitcast { %f64CXY*, %f64CXY*, double }* %20 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64CXY*, %f64CXY*, double }*, i64, i64)* @normalize_l2_tmp_thunk0 to i8*), i8* %24, i64 %19)
  ret %f64CXY* %18
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!0 = distinct !{!0}
!1 = !{i32 1, i32 -1}
