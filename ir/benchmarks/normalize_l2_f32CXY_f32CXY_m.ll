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
  %9 = getelementptr inbounds %f32CXY, %f32CXY* %4, i64 0, i32 2
  %channels = load i32, i32* %9, align 4, !range !0
  %dst_c = zext i32 %channels to i64
  %10 = getelementptr inbounds %f32CXY, %f32CXY* %4, i64 0, i32 3
  %columns = load i32, i32* %10, align 4, !range !0
  %dst_x = zext i32 %columns to i64
  %dst_y_step = mul nuw nsw i64 %dst_x, %dst_c
  %11 = getelementptr inbounds %f32CXY, %f32CXY* %4, i64 0, i32 6, i64 0
  %12 = ptrtoint float* %11 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  tail call void @llvm.assume(i1 %14)
  %15 = getelementptr inbounds %f32CXY, %f32CXY* %6, i64 0, i32 2
  %channels1 = load i32, i32* %15, align 4, !range !0
  %src_c = zext i32 %channels1 to i64
  %16 = getelementptr inbounds %f32CXY, %f32CXY* %6, i64 0, i32 3
  %columns2 = load i32, i32* %16, align 4, !range !0
  %src_x = zext i32 %columns2 to i64
  %src_y_step = mul nuw nsw i64 %src_x, %src_c
  %17 = getelementptr inbounds %f32CXY, %f32CXY* %6, i64 0, i32 6, i64 0
  %18 = ptrtoint float* %17 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  tail call void @llvm.assume(i1 %20)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %21 = mul nuw nsw i64 %src_y_step, %y
  %22 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %x_body, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %x_body ]
  %23 = add nuw nsw i64 %x, %21
  %24 = getelementptr %f32CXY, %f32CXY* %6, i64 0, i32 6, i64 %23
  %25 = load float, float* %24, align 4, !llvm.mem.parallel_loop_access !1
  %26 = fmul float %8, %25
  %27 = add nuw nsw i64 %x, %22
  %28 = getelementptr %f32CXY, %f32CXY* %4, i64 0, i32 6, i64 %27
  store float %26, float* %28, align 4, !llvm.mem.parallel_loop_access !1
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
  %storemerge3 = phi i32 [ 0, %entry ], [ %13, %then ]
  %6 = phi double [ 0.000000e+00, %entry ], [ %12, %then ]
  %7 = sext i32 %storemerge3 to i64
  %8 = getelementptr %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 %7
  %9 = load float, float* %8, align 4
  %10 = fpext float %9 to double
  %11 = fmul double %10, %10
  %12 = fadd double %6, %11
  %13 = add nuw nsw i32 %storemerge3, 1
  %14 = icmp eq i32 %13, %5
  br i1 %14, label %end, label %then

end:                                              ; preds = %then
  %15 = tail call double @llvm.sqrt.f64(double %12)
  %16 = fdiv double 1.000000e+00, %15
  %17 = fptrunc double %16 to float
  %18 = tail call %u0CXYT* @likely_new(i32 28960, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %19 = bitcast %u0CXYT* %18 to %f32CXY*
  %20 = zext i32 %rows to i64
  %21 = alloca { %f32CXY*, %f32CXY*, float }, align 8
  %22 = bitcast { %f32CXY*, %f32CXY*, float }* %21 to %u0CXYT**
  store %u0CXYT* %18, %u0CXYT** %22, align 8
  %23 = getelementptr inbounds { %f32CXY*, %f32CXY*, float }, { %f32CXY*, %f32CXY*, float }* %21, i64 0, i32 1
  store %f32CXY* %0, %f32CXY** %23, align 8
  %24 = getelementptr inbounds { %f32CXY*, %f32CXY*, float }, { %f32CXY*, %f32CXY*, float }* %21, i64 0, i32 2
  store float %17, float* %24, align 8
  %25 = bitcast { %f32CXY*, %f32CXY*, float }* %21 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32CXY*, %f32CXY*, float }*, i64, i64)* @normalize_l2_tmp_thunk0 to i8*), i8* %25, i64 %20)
  ret %f32CXY* %19
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
