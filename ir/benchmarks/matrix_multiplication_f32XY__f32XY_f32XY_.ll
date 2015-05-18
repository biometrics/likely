; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

define %f32XY* @matrix_multiplication(%f32XY*, %f32XY*) {
entry:
  %2 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 4
  %rows = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %3, align 4, !range !0
  %4 = icmp eq i32 %rows, %columns
  call void @llvm.assume(i1 %4)
  %5 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 3
  %columns1 = load i32, i32* %5, align 4, !range !0
  %6 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 4
  %rows2 = load i32, i32* %6, align 4, !range !0
  %7 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns1, i32 %rows2, i32 1, i8* null)
  %8 = zext i32 %rows2 to i64
  %C_y_step = zext i32 %columns1 to i64
  %9 = getelementptr inbounds %u0CXYT, %u0CXYT* %7, i64 1
  %10 = bitcast %u0CXYT* %9 to float*
  %11 = ptrtoint %u0CXYT* %9 to i64
  %12 = and i64 %11, 31
  %13 = icmp eq i64 %12, 0
  call void @llvm.assume(i1 %13)
  %A_y_step = zext i32 %columns to i64
  %14 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 6, i64 0
  %15 = ptrtoint float* %14 to i64
  %16 = and i64 %15, 31
  %17 = icmp eq i64 %16, 0
  call void @llvm.assume(i1 %17)
  %18 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 6, i64 0
  %19 = ptrtoint float* %18 to i64
  %20 = and i64 %19, 31
  %21 = icmp eq i64 %20, 0
  call void @llvm.assume(i1 %21)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %22 = mul nuw nsw i64 %y, %A_y_step
  %23 = mul nuw nsw i64 %y, %C_y_step
  br label %x_body

x_body:                                           ; preds = %exit, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %exit ]
  br label %true_enry

true_enry:                                        ; preds = %x_body, %true_enry
  %24 = phi i32 [ 0, %x_body ], [ %38, %true_enry ]
  %25 = phi double [ 0.000000e+00, %x_body ], [ %37, %true_enry ]
  %26 = sext i32 %24 to i64
  %27 = add nuw nsw i64 %26, %22
  %28 = getelementptr %f32XY, %f32XY* %0, i64 0, i32 6, i64 %27
  %29 = load float, float* %28, align 4, !llvm.mem.parallel_loop_access !1
  %30 = fpext float %29 to double
  %31 = mul nuw nsw i64 %26, %C_y_step
  %32 = add nuw nsw i64 %31, %x
  %33 = getelementptr %f32XY, %f32XY* %1, i64 0, i32 6, i64 %32
  %34 = load float, float* %33, align 4, !llvm.mem.parallel_loop_access !1
  %35 = fpext float %34 to double
  %36 = fmul fast double %35, %30
  %37 = fadd fast double %36, %25
  %38 = add nuw nsw i32 %24, 1
  %39 = icmp eq i32 %38, %columns
  br i1 %39, label %exit, label %true_enry

exit:                                             ; preds = %true_enry
  %40 = fptrunc double %37 to float
  %41 = add nuw nsw i64 %x, %23
  %42 = getelementptr float, float* %10, i64 %41
  store float %40, float* %42, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %C_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !1

x_exit:                                           ; preds = %exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %8
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %43 = bitcast %u0CXYT* %7 to %f32XY*
  ret %f32XY* %43
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
