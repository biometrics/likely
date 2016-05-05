; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f32XY* @match_template(%f32XY*, %f32XY*) {
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
  %templ_y_step = zext i32 %width to i64
  %21 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 6, i64 0
  %22 = ptrtoint float* %21 to i64
  %23 = and i64 %22, 31
  %24 = icmp eq i64 %23, 0
  call void @llvm.assume(i1 %24)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %25 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %exit
  %x = phi i64 [ %x_increment, %exit ], [ 0, %y_body ]
  br label %loop9.preheader

loop9.preheader:                                  ; preds = %x_body, %exit11
  %26 = phi i32 [ %50, %exit11 ], [ 0, %x_body ]
  %27 = phi double [ %47, %exit11 ], [ 0.000000e+00, %x_body ]
  %28 = sext i32 %26 to i64
  %29 = add nuw nsw i64 %28, %y
  %30 = mul nuw nsw i64 %29, %src_y_step
  %31 = add i64 %30, %x
  %32 = mul nuw nsw i64 %28, %templ_y_step
  br label %true_entry10

exit:                                             ; preds = %exit11
  %33 = add nuw nsw i64 %x, %25
  %34 = getelementptr float, float* %13, i64 %33
  %35 = fptrunc double %47 to float
  store float %35, float* %34, align 4, !llvm.mem.parallel_loop_access !1
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
  %36 = phi double [ %47, %true_entry10 ], [ %27, %loop9.preheader ]
  %37 = phi i32 [ %48, %true_entry10 ], [ 0, %loop9.preheader ]
  %38 = sext i32 %37 to i64
  %39 = add i64 %31, %38
  %40 = getelementptr %f32XY, %f32XY* %0, i64 0, i32 6, i64 %39
  %41 = load float, float* %40, align 4, !llvm.mem.parallel_loop_access !1
  %42 = add nuw nsw i64 %38, %32
  %43 = getelementptr %f32XY, %f32XY* %1, i64 0, i32 6, i64 %42
  %44 = load float, float* %43, align 4, !llvm.mem.parallel_loop_access !1
  %45 = fmul fast float %44, %41
  %46 = fpext float %45 to double
  %47 = fadd fast double %46, %36
  %48 = add nuw nsw i32 %37, 1
  %49 = icmp eq i32 %48, %width
  br i1 %49, label %exit11, label %true_entry10

exit11:                                           ; preds = %true_entry10
  %50 = add nuw nsw i32 %26, 1
  %51 = icmp eq i32 %50, %height
  br i1 %51, label %exit, label %loop9.preheader
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
