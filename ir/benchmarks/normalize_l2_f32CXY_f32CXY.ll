; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32CXY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readnone
declare double @llvm.sqrt.f64(double) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
declare void @llvm.assume(i1) #2

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
  %20 = zext i32 %rows to i64
  %dst_c = zext i32 %channels to i64
  %dst_x = zext i32 %columns to i64
  %21 = getelementptr inbounds %u0CXYT, %u0CXYT* %19, i64 1
  %22 = bitcast %u0CXYT* %21 to float*
  %23 = ptrtoint %u0CXYT* %21 to i64
  %24 = and i64 %23, 31
  %25 = icmp eq i64 %24, 0
  call void @llvm.assume(i1 %25)
  %26 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 0
  %27 = ptrtoint float* %26 to i64
  %28 = and i64 %27, 31
  %29 = icmp eq i64 %28, 0
  call void @llvm.assume(i1 %29)
  %30 = mul nuw nsw i64 %dst_x, %dst_c
  %31 = mul nuw nsw i64 %30, %20
  br label %y_body

y_body:                                           ; preds = %y_body, %end
  %y = phi i64 [ 0, %end ], [ %y_increment, %y_body ]
  %32 = getelementptr %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 %y
  %33 = load float, float* %32, align 4, !llvm.mem.parallel_loop_access !1
  %34 = fmul fast float %33, %18
  %35 = getelementptr float, float* %22, i64 %y
  store float %34, float* %35, align 4, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %31
  br i1 %y_postcondition, label %y_exit, label %y_body, !llvm.loop !1

y_exit:                                           ; preds = %y_body
  %36 = bitcast %u0CXYT* %19 to %f32CXY*
  ret %f32CXY* %36
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
