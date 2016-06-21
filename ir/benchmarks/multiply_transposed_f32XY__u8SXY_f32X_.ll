; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define noalias %f32Matrix* @multiply_transposed(%u8Matrix* noalias nocapture readonly, %f32Matrix* noalias nocapture readonly) #1 {
entry:
  %2 = getelementptr inbounds %u8Matrix, %u8Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %u8Matrix, %u8Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %5 = zext i32 %rows to i64
  %mat_y_step = zext i32 %columns to i64
  %6 = getelementptr inbounds %u0Matrix, %u0Matrix* %4, i64 1
  %7 = bitcast %u0Matrix* %6 to float*
  %8 = mul nuw nsw i64 %5, %mat_y_step
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %9 = getelementptr %u8Matrix, %u8Matrix* %0, i64 0, i32 6, i64 %y
  %10 = load i8, i8* %9, align 1, !llvm.mem.parallel_loop_access !1
  %11 = getelementptr float, float* %7, i64 %y
  %12 = uitofp i8 %10 to float
  store float %12, float* %11, align 4, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %8
  br i1 %y_postcondition, label %y_body15, label %y_body

y_body15:                                         ; preds = %y_body, %x_exit19
  %y17 = phi i64 [ %y_increment23, %x_exit19 ], [ 0, %y_body ]
  %13 = mul nuw nsw i64 %y17, %mat_y_step
  br label %x_body18

x_body18:                                         ; preds = %y_body15, %x_body18
  %x20 = phi i64 [ %x_increment21, %x_body18 ], [ 0, %y_body15 ]
  %14 = add nuw nsw i64 %x20, %13
  %15 = getelementptr float, float* %7, i64 %14
  %16 = load float, float* %15, align 4, !llvm.mem.parallel_loop_access !2
  %17 = getelementptr %f32Matrix, %f32Matrix* %1, i64 0, i32 6, i64 %x20
  %18 = load float, float* %17, align 4, !llvm.mem.parallel_loop_access !2
  %19 = fsub fast float %16, %18
  store float %19, float* %15, align 4, !llvm.mem.parallel_loop_access !2
  %x_increment21 = add nuw nsw i64 %x20, 1
  %x_postcondition22 = icmp eq i64 %x_increment21, %mat_y_step
  br i1 %x_postcondition22, label %x_exit19, label %x_body18

x_exit19:                                         ; preds = %x_body18
  %y_increment23 = add nuw nsw i64 %y17, 1
  %y_postcondition24 = icmp eq i64 %y_increment23, %5
  br i1 %y_postcondition24, label %y_exit16, label %y_body15

y_exit16:                                         ; preds = %x_exit19
  %20 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %21 = getelementptr inbounds %u0Matrix, %u0Matrix* %20, i64 1
  %22 = bitcast %u0Matrix* %21 to float*
  br label %y_body33

y_body33:                                         ; preds = %x_exit37, %y_exit16
  %y35 = phi i64 [ 0, %y_exit16 ], [ %y_increment43, %x_exit37 ]
  %23 = mul nuw nsw i64 %y35, %mat_y_step
  br label %x_body36

x_body36:                                         ; preds = %y_body33, %Flow
  %x38 = phi i64 [ %x_increment41, %Flow ], [ 0, %y_body33 ]
  %24 = icmp ugt i64 %y35, %x38
  br i1 %24, label %Flow, label %true_entry39

x_exit37:                                         ; preds = %Flow
  %y_increment43 = add nuw nsw i64 %y35, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %mat_y_step
  br i1 %y_postcondition44, label %y_exit34, label %y_body33

y_exit34:                                         ; preds = %x_exit37
  %dst = bitcast %u0Matrix* %20 to %f32Matrix*
  %25 = bitcast %u0Matrix* %4 to i8*
  call void @likely_release_mat(i8* %25) #1
  ret %f32Matrix* %dst

true_entry39:                                     ; preds = %x_body36, %true_entry39
  %26 = phi i32 [ %40, %true_entry39 ], [ 0, %x_body36 ]
  %27 = phi double [ %39, %true_entry39 ], [ 0.000000e+00, %x_body36 ]
  %28 = sext i32 %26 to i64
  %29 = mul nuw nsw i64 %28, %mat_y_step
  %30 = add nuw nsw i64 %29, %x38
  %31 = getelementptr float, float* %7, i64 %30
  %32 = load float, float* %31, align 4, !llvm.mem.parallel_loop_access !3
  %33 = fpext float %32 to double
  %34 = add nuw nsw i64 %29, %y35
  %35 = getelementptr float, float* %7, i64 %34
  %36 = load float, float* %35, align 4, !llvm.mem.parallel_loop_access !3
  %37 = fpext float %36 to double
  %38 = fmul fast double %37, %33
  %39 = fadd fast double %38, %27
  %40 = add nuw nsw i32 %26, 1
  %41 = icmp eq i32 %40, %rows
  br i1 %41, label %exit40, label %true_entry39

Flow:                                             ; preds = %x_body36, %exit40
  %x_increment41 = add nuw nsw i64 %x38, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %mat_y_step
  br i1 %x_postcondition42, label %x_exit37, label %x_body36

exit40:                                           ; preds = %true_entry39
  %42 = add nuw nsw i64 %x38, %23
  %43 = getelementptr float, float* %22, i64 %42
  %44 = fptrunc double %39 to float
  store float %44, float* %43, align 4, !llvm.mem.parallel_loop_access !3
  %45 = mul nuw nsw i64 %x38, %mat_y_step
  %46 = add nuw nsw i64 %45, %y35
  %47 = getelementptr float, float* %22, i64 %46
  store float %44, float* %47, align 4, !llvm.mem.parallel_loop_access !3
  br label %Flow
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
