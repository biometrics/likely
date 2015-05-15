; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%f32X = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f32XY* @multiply_transposed(%f32XY*, %f32X*) {
entry:
  %2 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 4
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
  %11 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 6, i64 0
  %12 = ptrtoint float* %11 to i64
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
  %21 = getelementptr %f32XY, %f32XY* %0, i64 0, i32 6, i64 %20
  %22 = load float, float* %21, align 4, !llvm.mem.parallel_loop_access !1
  %23 = getelementptr %f32X, %f32X* %1, i64 0, i32 6, i64 %x
  %24 = load float, float* %23, align 4, !llvm.mem.parallel_loop_access !1
  %25 = fsub fast float %22, %24
  %26 = getelementptr float, float* %7, i64 %20
  store float %25, float* %26, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %centered_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !1

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %5
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %27 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %28 = getelementptr inbounds %u0CXYT, %u0CXYT* %27, i64 1
  %29 = bitcast %u0CXYT* %28 to float*
  %30 = ptrtoint %u0CXYT* %28 to i64
  %31 = and i64 %30, 31
  %32 = icmp eq i64 %31, 0
  call void @llvm.assume(i1 %32)
  br label %y_body18

y_body18:                                         ; preds = %x_exit22, %y_exit
  %y20 = phi i64 [ 0, %y_exit ], [ %y_increment28, %x_exit22 ]
  %33 = mul nuw nsw i64 %y20, %centered_y_step
  br label %x_body21

x_body21:                                         ; preds = %end, %y_body18
  %x23 = phi i64 [ 0, %y_body18 ], [ %x_increment26, %end ]
  %34 = icmp ugt i64 %y20, %x23
  br i1 %34, label %end, label %then24

end:                                              ; preds = %x_body21, %end25
  %x_increment26 = add nuw nsw i64 %x23, 1
  %x_postcondition27 = icmp eq i64 %x_increment26, %centered_y_step
  br i1 %x_postcondition27, label %x_exit22, label %x_body21, !llvm.loop !2

x_exit22:                                         ; preds = %end
  %y_increment28 = add nuw nsw i64 %y20, 1
  %y_postcondition29 = icmp eq i64 %y_increment28, %centered_y_step
  br i1 %y_postcondition29, label %y_exit19, label %y_body18

y_exit19:                                         ; preds = %x_exit22
  %35 = bitcast %u0CXYT* %27 to %f32XY*
  %36 = bitcast %u0CXYT* %4 to i8*
  call void @likely_release_mat(i8* %36)
  ret %f32XY* %35

then24:                                           ; preds = %x_body21, %then24
  %37 = phi i32 [ %50, %then24 ], [ 0, %x_body21 ]
  %38 = phi double [ %49, %then24 ], [ 0.000000e+00, %x_body21 ]
  %39 = sext i32 %37 to i64
  %40 = mul nuw nsw i64 %39, %centered_y_step
  %41 = add nuw nsw i64 %40, %x23
  %42 = getelementptr float, float* %7, i64 %41
  %43 = load float, float* %42, align 4, !llvm.mem.parallel_loop_access !2
  %44 = add nuw nsw i64 %40, %y20
  %45 = getelementptr float, float* %7, i64 %44
  %46 = load float, float* %45, align 4, !llvm.mem.parallel_loop_access !2
  %47 = fmul fast float %46, %43
  %48 = fpext float %47 to double
  %49 = fadd fast double %48, %38
  %50 = add nuw nsw i32 %37, 1
  %51 = icmp eq i32 %50, %rows
  br i1 %51, label %end25, label %then24

end25:                                            ; preds = %then24
  %52 = fptrunc double %49 to float
  %53 = add nuw nsw i64 %x23, %33
  %54 = getelementptr float, float* %29, i64 %53
  store float %52, float* %54, align 4, !llvm.mem.parallel_loop_access !2
  %55 = mul nuw nsw i64 %x23, %centered_y_step
  %56 = add nuw nsw i64 %55, %y20
  %57 = getelementptr float, float* %29, i64 %56
  store float %52, float* %57, align 4, !llvm.mem.parallel_loop_access !2
  br label %end
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
