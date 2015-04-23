; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32CXY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readnone
declare double @llvm.sqrt.f64(double) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
declare void @llvm.assume(i1) #2

; Function Attrs: nounwind
define %f32CXY* @normalize_l2(%f32CXY*) #2 {
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
  %19 = zext i32 %rows to i64
  %dst_c = zext i32 %channels to i64
  %dst_x = zext i32 %columns to i64
  %dst_y_step = mul nuw nsw i64 %dst_x, %dst_c
  %20 = getelementptr inbounds %u0CXYT, %u0CXYT* %18, i64 1
  %21 = bitcast %u0CXYT* %20 to float*
  %22 = ptrtoint %u0CXYT* %20 to i64
  %23 = and i64 %22, 31
  %24 = icmp eq i64 %23, 0
  tail call void @llvm.assume(i1 %24)
  %25 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 0
  %26 = ptrtoint float* %25 to i64
  %27 = and i64 %26, 31
  %28 = icmp eq i64 %27, 0
  tail call void @llvm.assume(i1 %28)
  br label %y_body

y_body:                                           ; preds = %x_exit, %end
  %y = phi i64 [ 0, %end ], [ %y_increment, %x_exit ]
  %29 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %x_body, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %x_body ]
  %30 = add nuw nsw i64 %x, %29
  %31 = getelementptr %f32CXY, %f32CXY* %0, i64 0, i32 6, i64 %30
  %32 = load float, float* %31, align 4, !llvm.mem.parallel_loop_access !1
  %33 = fmul float %17, %32
  %34 = getelementptr float, float* %21, i64 %30
  store float %33, float* %34, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !1

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %19
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %35 = bitcast %u0CXYT* %18 to %f32CXY*
  ret %f32CXY* %35
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
