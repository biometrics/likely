; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

define %f32XY* @gemm(%f32XY*, %f32XY*, double, %f32XY*, double) {
entry:
  %5 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %6, align 4, !range !0
  %7 = icmp eq i32 %rows, %columns
  call void @llvm.assume(i1 %7)
  %8 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 4
  %rows1 = load i32, i32* %8, align 4, !range !0
  %9 = getelementptr inbounds %f32XY, %f32XY* %3, i64 0, i32 4
  %rows2 = load i32, i32* %9, align 4, !range !0
  %10 = icmp eq i32 %rows1, %rows2
  call void @llvm.assume(i1 %10)
  %11 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 3
  %columns3 = load i32, i32* %11, align 4, !range !0
  %12 = getelementptr inbounds %f32XY, %f32XY* %3, i64 0, i32 3
  %columns4 = load i32, i32* %12, align 4, !range !0
  %13 = icmp eq i32 %columns3, %columns4
  call void @llvm.assume(i1 %13)
  %14 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns4, i32 %rows2, i32 1, i8* null)
  %15 = zext i32 %rows2 to i64
  %dst_y_step = zext i32 %columns4 to i64
  %16 = getelementptr inbounds %u0CXYT, %u0CXYT* %14, i64 1
  %17 = bitcast %u0CXYT* %16 to float*
  %18 = ptrtoint %u0CXYT* %16 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  %src1_y_step = zext i32 %columns to i64
  %21 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 6, i64 0
  %22 = ptrtoint float* %21 to i64
  %23 = and i64 %22, 31
  %24 = icmp eq i64 %23, 0
  call void @llvm.assume(i1 %24)
  %25 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 6, i64 0
  %26 = ptrtoint float* %25 to i64
  %27 = and i64 %26, 31
  %28 = icmp eq i64 %27, 0
  call void @llvm.assume(i1 %28)
  %29 = getelementptr inbounds %f32XY, %f32XY* %3, i64 0, i32 6, i64 0
  %30 = ptrtoint float* %29 to i64
  %31 = and i64 %30, 31
  %32 = icmp eq i64 %31, 0
  call void @llvm.assume(i1 %32)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %33 = mul nuw nsw i64 %y, %dst_y_step
  %34 = mul nuw nsw i64 %y, %src1_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %exit
  %x = phi i64 [ %x_increment, %exit ], [ 0, %y_body ]
  %35 = add nuw nsw i64 %x, %33
  br label %true_entry

true_entry:                                       ; preds = %x_body, %true_entry
  %36 = phi i32 [ %48, %true_entry ], [ 0, %x_body ]
  %37 = phi float [ %47, %true_entry ], [ 0.000000e+00, %x_body ]
  %38 = sext i32 %36 to i64
  %39 = add nuw nsw i64 %38, %34
  %40 = getelementptr %f32XY, %f32XY* %0, i64 0, i32 6, i64 %39
  %41 = load float, float* %40, align 4, !llvm.mem.parallel_loop_access !1
  %42 = mul nuw nsw i64 %38, %dst_y_step
  %43 = add nuw nsw i64 %42, %x
  %44 = getelementptr %f32XY, %f32XY* %1, i64 0, i32 6, i64 %43
  %45 = load float, float* %44, align 4, !llvm.mem.parallel_loop_access !1
  %46 = fmul fast float %45, %41
  %47 = fadd fast float %46, %37
  %48 = add nuw nsw i32 %36, 1
  %49 = icmp eq i32 %48, %columns
  br i1 %49, label %exit, label %true_entry

exit:                                             ; preds = %true_entry
  %50 = getelementptr float, float* %17, i64 %35
  %51 = fpext float %47 to double
  %52 = fmul fast double %51, %2
  %53 = fptrunc double %52 to float
  %54 = getelementptr %f32XY, %f32XY* %3, i64 0, i32 6, i64 %35
  %55 = load float, float* %54, align 4, !llvm.mem.parallel_loop_access !1
  %56 = fpext float %55 to double
  %57 = fmul fast double %56, %4
  %58 = fptrunc double %57 to float
  %59 = fadd fast float %58, %53
  store float %59, float* %50, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %15
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %60 = bitcast %u0CXYT* %14 to %f32XY*
  ret %f32XY* %60
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
