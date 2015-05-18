; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%i16XY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }
%f32X = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f32XY* @multiply_transposed(%i16XY*, %f32X*) {
entry:
  %2 = getelementptr inbounds %i16XY, %i16XY* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %i16XY, %i16XY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %5 = zext i32 %rows to i64
  %centered_y_step = zext i32 %columns to i64
  %6 = getelementptr inbounds %u0CXYT, %u0CXYT* %4, i64 1
  %7 = bitcast %u0CXYT* %6 to float*
  %8 = ptrtoint %u0CXYT* %6 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  %11 = getelementptr inbounds %i16XY, %i16XY* %0, i64 0, i32 6, i64 0
  %12 = ptrtoint i16* %11 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  call void @llvm.assume(i1 %14)
  %15 = getelementptr inbounds %f32X, %f32X* %1, i64 0, i32 6, i64 0
  %16 = ptrtoint float* %15 to i64
  %17 = and i64 %16, 31
  %18 = icmp eq i64 %17, 0
  call void @llvm.assume(i1 %18)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %19 = mul nuw nsw i64 %y, %centered_y_step
  br label %x_body

x_body:                                           ; preds = %x_body, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %x_body ]
  %20 = add nuw nsw i64 %x, %19
  %21 = getelementptr %i16XY, %i16XY* %0, i64 0, i32 6, i64 %20
  %22 = load i16, i16* %21, align 2, !llvm.mem.parallel_loop_access !1
  %23 = getelementptr %f32X, %f32X* %1, i64 0, i32 6, i64 %x
  %24 = load float, float* %23, align 4, !llvm.mem.parallel_loop_access !1
  %25 = sitofp i16 %22 to float
  %26 = fsub fast float %25, %24
  %27 = getelementptr float, float* %7, i64 %20
  store float %26, float* %27, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %centered_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !1

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %5
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %28 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %29 = getelementptr inbounds %u0CXYT, %u0CXYT* %28, i64 1
  %30 = bitcast %u0CXYT* %29 to float*
  %31 = ptrtoint %u0CXYT* %29 to i64
  %32 = and i64 %31, 31
  %33 = icmp eq i64 %32, 0
  call void @llvm.assume(i1 %33)
  br label %y_body18

y_body18:                                         ; preds = %x_exit22, %y_exit
  %y20 = phi i64 [ 0, %y_exit ], [ %y_increment28, %x_exit22 ]
  %34 = mul nuw nsw i64 %y20, %centered_y_step
  br label %x_body21

x_body21:                                         ; preds = %exit, %y_body18
  %x23 = phi i64 [ 0, %y_body18 ], [ %x_increment26, %exit ]
  %35 = icmp ugt i64 %y20, %x23
  br i1 %35, label %exit, label %true_entry24

exit:                                             ; preds = %x_body21, %exit25
  %x_increment26 = add nuw nsw i64 %x23, 1
  %x_postcondition27 = icmp eq i64 %x_increment26, %centered_y_step
  br i1 %x_postcondition27, label %x_exit22, label %x_body21, !llvm.loop !2

x_exit22:                                         ; preds = %exit
  %y_increment28 = add nuw nsw i64 %y20, 1
  %y_postcondition29 = icmp eq i64 %y_increment28, %centered_y_step
  br i1 %y_postcondition29, label %y_exit19, label %y_body18

y_exit19:                                         ; preds = %x_exit22
  %36 = bitcast %u0CXYT* %28 to %f32XY*
  %37 = bitcast %u0CXYT* %4 to i8*
  call void @likely_release_mat(i8* %37)
  ret %f32XY* %36

true_entry24:                                     ; preds = %x_body21, %true_entry24
  %38 = phi i32 [ %52, %true_entry24 ], [ 0, %x_body21 ]
  %39 = phi double [ %51, %true_entry24 ], [ 0.000000e+00, %x_body21 ]
  %40 = sext i32 %38 to i64
  %41 = mul nuw nsw i64 %40, %centered_y_step
  %42 = add nuw nsw i64 %41, %x23
  %43 = getelementptr float, float* %7, i64 %42
  %44 = load float, float* %43, align 4, !llvm.mem.parallel_loop_access !2
  %45 = fpext float %44 to double
  %46 = add nuw nsw i64 %41, %y20
  %47 = getelementptr float, float* %7, i64 %46
  %48 = load float, float* %47, align 4, !llvm.mem.parallel_loop_access !2
  %49 = fpext float %48 to double
  %50 = fmul fast double %49, %45
  %51 = fadd fast double %50, %39
  %52 = add nuw nsw i32 %38, 1
  %53 = icmp eq i32 %52, %rows
  br i1 %53, label %exit25, label %true_entry24

exit25:                                           ; preds = %true_entry24
  %54 = fptrunc double %51 to float
  %55 = add nuw nsw i64 %x23, %34
  %56 = getelementptr float, float* %30, i64 %55
  store float %54, float* %56, align 4, !llvm.mem.parallel_loop_access !2
  %57 = mul nuw nsw i64 %x23, %centered_y_step
  %58 = add nuw nsw i64 %57, %y20
  %59 = getelementptr float, float* %30, i64 %58
  store float %54, float* %59, align 4, !llvm.mem.parallel_loop_access !2
  br label %exit
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
