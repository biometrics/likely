; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32CXY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readnone
declare double @llvm.sqrt.f64(double) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
define private void @normalize_l2_tmp_thunk0({ %f32CXY*, %f32CXY*, float }* noalias nocapture readonly, i64, i64) #2 {
entry:
  %3 = getelementptr inbounds { %f32CXY*, %f32CXY*, float }, { %f32CXY*, %f32CXY*, float }* %0, i64 0, i32 0
  %4 = load %f32CXY*, %f32CXY** %3, align 8
  %5 = getelementptr inbounds { %f32CXY*, %f32CXY*, float }, { %f32CXY*, %f32CXY*, float }* %0, i64 0, i32 1
  %6 = load %f32CXY*, %f32CXY** %5, align 8
  %7 = getelementptr inbounds { %f32CXY*, %f32CXY*, float }, { %f32CXY*, %f32CXY*, float }* %0, i64 0, i32 2
  %8 = load float, float* %7, align 4
  %9 = getelementptr inbounds %f32CXY, %f32CXY* %6, i64 0, i32 2
  %channels1 = load i32, i32* %9, align 4, !range !0
  %dst_c = zext i32 %channels1 to i64
  %10 = getelementptr inbounds %f32CXY, %f32CXY* %6, i64 0, i32 3
  %columns2 = load i32, i32* %10, align 4, !range !0
  %dst_x = zext i32 %columns2 to i64
  %11 = getelementptr inbounds %f32CXY, %f32CXY* %4, i64 0, i32 6, i64 0
  %12 = ptrtoint float* %11 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  call void @llvm.assume(i1 %14)
  %15 = getelementptr inbounds %f32CXY, %f32CXY* %6, i64 0, i32 6, i64 0
  %16 = ptrtoint float* %15 to i64
  %17 = and i64 %16, 31
  %18 = icmp eq i64 %17, 0
  call void @llvm.assume(i1 %18)
  %19 = mul i64 %dst_c, %2
  %20 = mul i64 %19, %dst_x
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %y_body ]
  %21 = getelementptr %f32CXY, %f32CXY* %6, i64 0, i32 6, i64 %y
  %22 = load float, float* %21, align 4, !llvm.mem.parallel_loop_access !1
  %23 = fmul fast float %22, %8
  %24 = getelementptr %f32CXY, %f32CXY* %4, i64 0, i32 6, i64 %y
  store float %23, float* %24, align 4, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %20
  br i1 %y_postcondition, label %y_exit, label %y_body, !llvm.loop !1

y_exit:                                           ; preds = %y_body
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #2

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %f32CXY* @normalize_l2(%f32CXY*) {
entry:
  %1 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = mul nuw nsw i32 %columns, %channels
  %4 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !0
  %5 = mul nuw nsw i32 %3, %rows
  br label %then

then:                                             ; preds = %entry, %then
  %6 = phi i32 [ 0, %entry ], [ %14, %then ]
  %7 = phi double [ 0.000000e+00, %entry ], [ %13, %then ]
  %8 = sext i32 %6 to i64
  %9 = getelementptr %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 %8
  %10 = load float, float* %9, align 4
  %11 = fpext float %10 to double
  %12 = fmul fast double %11, %11
  %13 = fadd fast double %12, %7
  %14 = add nuw nsw i32 %6, 1
  %15 = icmp eq i32 %14, %5
  br i1 %15, label %end, label %then

end:                                              ; preds = %then
  %16 = call double @llvm.sqrt.f64(double %13)
  %17 = fdiv fast double 1.000000e+00, %16
  %18 = fptrunc double %17 to float
  %19 = call %u0CXYT* @likely_new(i32 28960, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %20 = bitcast %u0CXYT* %19 to %f32CXY*
  %21 = zext i32 %rows to i64
  %22 = alloca { %f32CXY*, %f32CXY*, float }, align 8
  %23 = bitcast { %f32CXY*, %f32CXY*, float }* %22 to %u0CXYT**
  store %u0CXYT* %19, %u0CXYT** %23, align 8
  %24 = getelementptr inbounds { %f32CXY*, %f32CXY*, float }, { %f32CXY*, %f32CXY*, float }* %22, i64 0, i32 1
  store %f32CXY* %0, %f32CXY** %24, align 8
  %25 = getelementptr inbounds { %f32CXY*, %f32CXY*, float }, { %f32CXY*, %f32CXY*, float }* %22, i64 0, i32 2
  store float %18, float* %25, align 8
  %26 = bitcast { %f32CXY*, %f32CXY*, float }* %22 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32CXY*, %f32CXY*, float }*, i64, i64)* @normalize_l2_tmp_thunk0 to i8*), i8* %26, i64 %21)
  ret %f32CXY* %20
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
