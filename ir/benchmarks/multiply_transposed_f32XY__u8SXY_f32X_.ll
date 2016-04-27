; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%u8SXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32X = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f32XY* @multiply_transposed(%u8SXY*, %f32X*) {
entry:
  %2 = getelementptr inbounds %u8SXY, %u8SXY* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %u8SXY, %u8SXY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %5 = zext i32 %rows to i64
  %mat_y_step = zext i32 %columns to i64
  %6 = getelementptr inbounds %u0CXYT, %u0CXYT* %4, i64 1
  %7 = bitcast %u0CXYT* %6 to float*
  %8 = ptrtoint %u0CXYT* %6 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  %11 = getelementptr inbounds %u8SXY, %u8SXY* %0, i64 0, i32 6, i64 0
  %12 = ptrtoint i8* %11 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  call void @llvm.assume(i1 %14)
  %15 = mul nuw nsw i64 %5, %mat_y_step
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %16 = getelementptr %u8SXY, %u8SXY* %0, i64 0, i32 6, i64 %y
  %17 = load i8, i8* %16, align 1, !llvm.mem.parallel_loop_access !1
  %18 = getelementptr float, float* %7, i64 %y
  %19 = uitofp i8 %17 to float
  store float %19, float* %18, align 4, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %15
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %20 = getelementptr inbounds %f32X, %f32X* %1, i64 0, i32 6, i64 0
  %21 = ptrtoint float* %20 to i64
  %22 = and i64 %21, 31
  %23 = icmp eq i64 %22, 0
  call void @llvm.assume(i1 %23)
  br label %y_body15

y_body15:                                         ; preds = %x_exit19, %y_exit
  %y17 = phi i64 [ 0, %y_exit ], [ %y_increment23, %x_exit19 ]
  %24 = mul nuw nsw i64 %y17, %mat_y_step
  br label %x_body18

x_body18:                                         ; preds = %y_body15, %x_body18
  %x20 = phi i64 [ %x_increment21, %x_body18 ], [ 0, %y_body15 ]
  %25 = add nuw nsw i64 %x20, %24
  %26 = getelementptr float, float* %7, i64 %25
  %27 = load float, float* %26, align 4, !llvm.mem.parallel_loop_access !2
  %28 = getelementptr %f32X, %f32X* %1, i64 0, i32 6, i64 %x20
  %29 = load float, float* %28, align 4, !llvm.mem.parallel_loop_access !2
  %30 = fsub fast float %27, %29
  store float %30, float* %26, align 4, !llvm.mem.parallel_loop_access !2
  %x_increment21 = add nuw nsw i64 %x20, 1
  %x_postcondition22 = icmp eq i64 %x_increment21, %mat_y_step
  br i1 %x_postcondition22, label %x_exit19, label %x_body18

x_exit19:                                         ; preds = %x_body18
  %y_increment23 = add nuw nsw i64 %y17, 1
  %y_postcondition24 = icmp eq i64 %y_increment23, %5
  br i1 %y_postcondition24, label %y_exit16, label %y_body15

y_exit16:                                         ; preds = %x_exit19
  %31 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %32 = getelementptr inbounds %u0CXYT, %u0CXYT* %31, i64 1
  %33 = bitcast %u0CXYT* %32 to float*
  %34 = ptrtoint %u0CXYT* %32 to i64
  %35 = and i64 %34, 31
  %36 = icmp eq i64 %35, 0
  call void @llvm.assume(i1 %36)
  br label %y_body33

y_body33:                                         ; preds = %x_exit37, %y_exit16
  %y35 = phi i64 [ 0, %y_exit16 ], [ %y_increment43, %x_exit37 ]
  %37 = mul nuw nsw i64 %y35, %mat_y_step
  br label %x_body36

x_body36:                                         ; preds = %y_body33, %Flow
  %x38 = phi i64 [ %x_increment41, %Flow ], [ 0, %y_body33 ]
  %38 = icmp ugt i64 %y35, %x38
  br i1 %38, label %Flow, label %true_entry39

x_exit37:                                         ; preds = %Flow
  %y_increment43 = add nuw nsw i64 %y35, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %mat_y_step
  br i1 %y_postcondition44, label %y_exit34, label %y_body33

y_exit34:                                         ; preds = %x_exit37
  %dst = bitcast %u0CXYT* %31 to %f32XY*
  %39 = bitcast %u0CXYT* %4 to i8*
  call void @likely_release_mat(i8* %39)
  ret %f32XY* %dst

true_entry39:                                     ; preds = %x_body36, %true_entry39
  %40 = phi i32 [ %54, %true_entry39 ], [ 0, %x_body36 ]
  %41 = phi double [ %53, %true_entry39 ], [ 0.000000e+00, %x_body36 ]
  %42 = sext i32 %40 to i64
  %43 = mul nuw nsw i64 %42, %mat_y_step
  %44 = add nuw nsw i64 %43, %x38
  %45 = getelementptr float, float* %7, i64 %44
  %46 = load float, float* %45, align 4, !llvm.mem.parallel_loop_access !3
  %47 = fpext float %46 to double
  %48 = add nuw nsw i64 %43, %y35
  %49 = getelementptr float, float* %7, i64 %48
  %50 = load float, float* %49, align 4, !llvm.mem.parallel_loop_access !3
  %51 = fpext float %50 to double
  %52 = fmul fast double %51, %47
  %53 = fadd fast double %52, %41
  %54 = add nuw nsw i32 %40, 1
  %55 = icmp eq i32 %54, %rows
  br i1 %55, label %exit40, label %true_entry39

Flow:                                             ; preds = %x_body36, %exit40
  %x_increment41 = add nuw nsw i64 %x38, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %mat_y_step
  br i1 %x_postcondition42, label %x_exit37, label %x_body36

exit40:                                           ; preds = %true_entry39
  %56 = add nuw nsw i64 %x38, %37
  %57 = getelementptr float, float* %33, i64 %56
  %58 = fptrunc double %53 to float
  store float %58, float* %57, align 4, !llvm.mem.parallel_loop_access !3
  %59 = mul nuw nsw i64 %x38, %mat_y_step
  %60 = add nuw nsw i64 %59, %y35
  %61 = getelementptr float, float* %33, i64 %60
  store float %58, float* %61, align 4, !llvm.mem.parallel_loop_access !3
  br label %Flow
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
