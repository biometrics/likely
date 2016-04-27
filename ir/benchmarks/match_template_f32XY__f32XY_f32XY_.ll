; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind argmemonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f32XY* @match_template(%f32XY*, %f32XY*) {
entry:
  %2 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 3
  %columns1 = load i32, i32* %3, align 4, !range !0
  %4 = sub i32 %columns, %columns1
  %5 = add nuw nsw i32 %4, 1
  %6 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 4
  %rows = load i32, i32* %6, align 4, !range !0
  %7 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 4
  %rows2 = load i32, i32* %7, align 4, !range !0
  %8 = sub i32 %rows, %rows2
  %9 = add nuw nsw i32 %8, 1
  %10 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %5, i32 %9, i32 1, i8* null)
  %11 = zext i32 %9 to i64
  %dst_y_step = zext i32 %5 to i64
  %12 = getelementptr inbounds %u0CXYT, %u0CXYT* %10, i64 1
  %13 = bitcast %u0CXYT* %12 to float*
  %14 = ptrtoint %u0CXYT* %12 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  call void @llvm.assume(i1 %16)
  %src_y_step = zext i32 %columns to i64
  %17 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 6, i64 0
  %18 = ptrtoint float* %17 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  %templ_y_step = zext i32 %columns1 to i64
  %templ_y = zext i32 %rows2 to i64
  %21 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 6, i64 0
  %22 = ptrtoint float* %21 to i64
  %23 = and i64 %22, 31
  %24 = icmp eq i64 %23, 0
  call void @llvm.assume(i1 %24)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %outer-y = phi i64 [ 0, %entry ], [ %y_increment32, %x_exit ]
  %25 = mul nuw nsw i64 %outer-y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %y_exit27
  %outer-x = phi i64 [ %x_increment30, %y_exit27 ], [ 0, %y_body ]
  br label %y_body26

y_body26:                                         ; preds = %x_body, %x_exit29
  %26 = phi double [ %40, %x_exit29 ], [ 0.000000e+00, %x_body ]
  %y = phi i64 [ %y_increment, %x_exit29 ], [ 0, %x_body ]
  %27 = add nuw nsw i64 %y, %outer-y
  %28 = mul nuw nsw i64 %27, %src_y_step
  %29 = add i64 %28, %outer-x
  %30 = mul nuw nsw i64 %y, %templ_y_step
  br label %x_body28

x_body28:                                         ; preds = %y_body26, %x_body28
  %31 = phi double [ %40, %x_body28 ], [ %26, %y_body26 ]
  %x = phi i64 [ %x_increment, %x_body28 ], [ 0, %y_body26 ]
  %32 = add i64 %29, %x
  %33 = getelementptr %f32XY, %f32XY* %0, i64 0, i32 6, i64 %32
  %34 = load float, float* %33, align 4
  %35 = add nuw nsw i64 %x, %30
  %36 = getelementptr %f32XY, %f32XY* %1, i64 0, i32 6, i64 %35
  %37 = load float, float* %36, align 4
  %38 = fmul fast float %37, %34
  %39 = fpext float %38 to double
  %40 = fadd fast double %39, %31
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %templ_y_step
  br i1 %x_postcondition, label %x_exit29, label %x_body28

x_exit29:                                         ; preds = %x_body28
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %templ_y
  br i1 %y_postcondition, label %y_exit27, label %y_body26

y_exit27:                                         ; preds = %x_exit29
  %41 = add nuw nsw i64 %outer-x, %25
  %42 = getelementptr float, float* %13, i64 %41
  %43 = fptrunc double %40 to float
  store float %43, float* %42, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment30 = add nuw nsw i64 %outer-x, 1
  %x_postcondition31 = icmp eq i64 %x_increment30, %dst_y_step
  br i1 %x_postcondition31, label %x_exit, label %x_body

x_exit:                                           ; preds = %y_exit27
  %y_increment32 = add nuw nsw i64 %outer-y, 1
  %y_postcondition33 = icmp eq i64 %y_increment32, %11
  br i1 %y_postcondition33, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %dst = bitcast %u0CXYT* %10 to %f32XY*
  ret %f32XY* %dst
}

attributes #0 = { nounwind argmemonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
