; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readonly
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
  br label %label12.preheader

label12.preheader:                                ; preds = %x_body, %exit14
  %26 = phi i32 [ %51, %exit14 ], [ 0, %x_body ]
  %27 = phi double [ %48, %exit14 ], [ 0.000000e+00, %x_body ]
  %28 = sext i32 %26 to i64
  %29 = add nuw nsw i64 %28, %y
  %30 = mul nuw nsw i64 %29, %src_y_step
  %31 = add i64 %30, %x
  %32 = mul nuw nsw i64 %28, %templ_y_step
  br label %true_entry13

exit:                                             ; preds = %exit14
  %33 = fptrunc double %48 to float
  %34 = add nuw nsw i64 %x, %25
  %35 = getelementptr float, float* %13, i64 %34
  store float %33, float* %35, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %11
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %36 = bitcast %u0CXYT* %10 to %f32XY*
  ret %f32XY* %36

true_entry13:                                     ; preds = %label12.preheader, %true_entry13
  %37 = phi double [ %48, %true_entry13 ], [ %27, %label12.preheader ]
  %38 = phi i32 [ %49, %true_entry13 ], [ 0, %label12.preheader ]
  %39 = sext i32 %38 to i64
  %40 = add i64 %31, %39
  %41 = getelementptr %f32XY, %f32XY* %0, i64 0, i32 6, i64 %40
  %42 = load float, float* %41, align 4, !llvm.mem.parallel_loop_access !1
  %43 = add nuw nsw i64 %39, %32
  %44 = getelementptr %f32XY, %f32XY* %1, i64 0, i32 6, i64 %43
  %45 = load float, float* %44, align 4, !llvm.mem.parallel_loop_access !1
  %46 = fmul fast float %45, %42
  %47 = fpext float %46 to double
  %48 = fadd fast double %47, %37
  %49 = add nuw nsw i32 %38, 1
  %50 = icmp eq i32 %49, %columns1
  br i1 %50, label %exit14, label %true_entry13

exit14:                                           ; preds = %true_entry13
  %51 = add nuw nsw i32 %26, 1
  %52 = icmp eq i32 %51, %rows2
  br i1 %52, label %exit, label %label12.preheader
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
