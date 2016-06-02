; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

define noalias %f32XY* @match_template(%f32XY* nocapture readonly, %f32XY* nocapture readonly) {
entry:
  %2 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 3
  %width = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 4
  %height = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %4, align 4, !range !0
  %5 = sub i32 %columns, %width
  %6 = add nuw nsw i32 %5, 1
  %7 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 4
  %rows = load i32, i32* %7, align 4, !range !0
  %8 = sub i32 %rows, %height
  %9 = add nuw nsw i32 %8, 1
  %10 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %6, i32 %9, i32 1, i8* null)
  %11 = zext i32 %9 to i64
  %dst_y_step = zext i32 %6 to i64
  %12 = getelementptr inbounds %u0CXYT, %u0CXYT* %10, i64 1
  %13 = bitcast %u0CXYT* %12 to float*
  %src_y_step = zext i32 %columns to i64
  %templ_y_step = zext i32 %width to i64
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %14 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %exit
  %x = phi i64 [ %x_increment, %exit ], [ 0, %y_body ]
  br label %loop9.preheader

loop9.preheader:                                  ; preds = %x_body, %exit11
  %15 = phi i32 [ %39, %exit11 ], [ 0, %x_body ]
  %16 = phi double [ %36, %exit11 ], [ 0.000000e+00, %x_body ]
  %17 = sext i32 %15 to i64
  %18 = add nuw nsw i64 %17, %y
  %19 = mul nuw nsw i64 %18, %src_y_step
  %20 = add i64 %19, %x
  %21 = mul nuw nsw i64 %17, %templ_y_step
  br label %true_entry10

exit:                                             ; preds = %exit11
  %22 = add nuw nsw i64 %x, %14
  %23 = getelementptr float, float* %13, i64 %22
  %24 = fptrunc double %36 to float
  store float %24, float* %23, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %11
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %dst = bitcast %u0CXYT* %10 to %f32XY*
  ret %f32XY* %dst

true_entry10:                                     ; preds = %loop9.preheader, %true_entry10
  %25 = phi double [ %36, %true_entry10 ], [ %16, %loop9.preheader ]
  %26 = phi i32 [ %37, %true_entry10 ], [ 0, %loop9.preheader ]
  %27 = sext i32 %26 to i64
  %28 = add i64 %20, %27
  %29 = getelementptr %f32XY, %f32XY* %0, i64 0, i32 6, i64 %28
  %30 = load float, float* %29, align 4, !llvm.mem.parallel_loop_access !1
  %31 = add nuw nsw i64 %27, %21
  %32 = getelementptr %f32XY, %f32XY* %1, i64 0, i32 6, i64 %31
  %33 = load float, float* %32, align 4, !llvm.mem.parallel_loop_access !1
  %34 = fmul fast float %33, %30
  %35 = fpext float %34 to double
  %36 = fadd fast double %35, %25
  %37 = add nuw nsw i32 %26, 1
  %38 = icmp eq i32 %37, %width
  br i1 %38, label %exit11, label %true_entry10

exit11:                                           ; preds = %true_entry10
  %39 = add nuw nsw i32 %15, 1
  %40 = icmp eq i32 %39, %height
  br i1 %40, label %exit, label %loop9.preheader
}

attributes #0 = { argmemonly nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
