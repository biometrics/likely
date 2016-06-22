; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u16Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i16] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define noalias %f32Matrix* @multiply_transposed(%u16Matrix* noalias nocapture readonly, %f32Matrix* noalias nocapture readonly) #1 {
entry:
  %2 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %5 = zext i32 %rows to i64
  %mat_y_step = zext i32 %columns to i64
  %6 = getelementptr inbounds %u0Matrix, %u0Matrix* %4, i64 1
  %7 = bitcast %u0Matrix* %6 to float*
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %8 = mul nuw nsw i64 %y, %mat_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %9 = add nuw nsw i64 %x, %8
  %10 = getelementptr %u16Matrix, %u16Matrix* %0, i64 0, i32 6, i64 %9
  %11 = load i16, i16* %10, align 2, !llvm.mem.parallel_loop_access !1
  %12 = getelementptr float, float* %7, i64 %9
  %13 = sitofp i16 %11 to float
  store float %13, float* %12, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %mat_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %5
  br i1 %y_postcondition, label %y_body15, label %y_body

y_body15:                                         ; preds = %x_exit, %x_exit19
  %y17 = phi i64 [ %y_increment23, %x_exit19 ], [ 0, %x_exit ]
  %14 = mul nuw nsw i64 %y17, %mat_y_step
  br label %x_body18

x_body18:                                         ; preds = %y_body15, %x_body18
  %x20 = phi i64 [ %x_increment21, %x_body18 ], [ 0, %y_body15 ]
  %15 = add nuw nsw i64 %x20, %14
  %16 = getelementptr float, float* %7, i64 %15
  %17 = load float, float* %16, align 4, !llvm.mem.parallel_loop_access !2
  %18 = getelementptr %f32Matrix, %f32Matrix* %1, i64 0, i32 6, i64 %x20
  %19 = load float, float* %18, align 4, !llvm.mem.parallel_loop_access !2
  %20 = fsub fast float %17, %19
  store float %20, float* %16, align 4, !llvm.mem.parallel_loop_access !2
  %x_increment21 = add nuw nsw i64 %x20, 1
  %x_postcondition22 = icmp eq i64 %x_increment21, %mat_y_step
  br i1 %x_postcondition22, label %x_exit19, label %x_body18

x_exit19:                                         ; preds = %x_body18
  %y_increment23 = add nuw nsw i64 %y17, 1
  %y_postcondition24 = icmp eq i64 %y_increment23, %5
  br i1 %y_postcondition24, label %y_exit16, label %y_body15

y_exit16:                                         ; preds = %x_exit19
  %21 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %22 = getelementptr inbounds %u0Matrix, %u0Matrix* %21, i64 1
  %23 = bitcast %u0Matrix* %22 to float*
  br label %y_body33

y_body33:                                         ; preds = %x_exit37, %y_exit16
  %y35 = phi i64 [ 0, %y_exit16 ], [ %y_increment43, %x_exit37 ]
  %24 = mul nuw nsw i64 %y35, %mat_y_step
  br label %x_body36

x_body36:                                         ; preds = %y_body33, %exit
  %x38 = phi i64 [ %x_increment41, %exit ], [ 0, %y_body33 ]
  %25 = icmp ugt i64 %y35, %x38
  br i1 %25, label %exit, label %true_entry39

true_entry39:                                     ; preds = %x_body36, %true_entry39
  %26 = phi i32 [ %40, %true_entry39 ], [ 0, %x_body36 ]
  %27 = phi double [ %39, %true_entry39 ], [ 0.000000e+00, %x_body36 ]
  %28 = zext i32 %26 to i64
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

exit40:                                           ; preds = %true_entry39
  %42 = add nuw nsw i64 %x38, %24
  %43 = getelementptr float, float* %23, i64 %42
  %44 = fptrunc double %39 to float
  store float %44, float* %43, align 4, !llvm.mem.parallel_loop_access !3
  %45 = mul nuw nsw i64 %x38, %mat_y_step
  %46 = add nuw nsw i64 %45, %y35
  %47 = getelementptr float, float* %23, i64 %46
  store float %44, float* %47, align 4, !llvm.mem.parallel_loop_access !3
  br label %exit

exit:                                             ; preds = %exit40, %x_body36
  %x_increment41 = add nuw nsw i64 %x38, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %mat_y_step
  br i1 %x_postcondition42, label %x_exit37, label %x_body36

x_exit37:                                         ; preds = %exit
  %y_increment43 = add nuw nsw i64 %y35, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %mat_y_step
  br i1 %y_postcondition44, label %y_exit34, label %y_body33

y_exit34:                                         ; preds = %x_exit37
  %dst = bitcast %u0Matrix* %21 to %f32Matrix*
  %48 = bitcast %u0Matrix* %4 to i8*
  call void @likely_release_mat(i8* %48) #1
  ret %f32Matrix* %dst
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
