; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define noalias %f32Matrix* @multiply_transposed(%f32Matrix* noalias nocapture readonly, %f32Matrix* noalias nocapture readonly) #1 {
entry:
  %2 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %5 = zext i32 %rows to i64
  %mat_y_step = zext i32 %columns to i64
  %6 = getelementptr inbounds %u0Matrix, %u0Matrix* %4, i64 1
  %scevgep = getelementptr %u0Matrix, %u0Matrix* %4, i64 1, i32 0
  %scevgep3 = getelementptr %f32Matrix, %f32Matrix* %0, i64 1, i32 0
  %7 = shl nuw nsw i64 %mat_y_step, 2
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %8 = mul i64 %y, %mat_y_step
  %scevgep1 = getelementptr i32, i32* %scevgep, i64 %8
  %scevgep12 = bitcast i32* %scevgep1 to i8*
  %scevgep4 = getelementptr i32, i32* %scevgep3, i64 %8
  %scevgep45 = bitcast i32* %scevgep4 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep12, i8* %scevgep45, i64 %7, i32 4, i1 false)
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %5
  br i1 %y_postcondition, label %y_body15.preheader, label %y_body

y_body15.preheader:                               ; preds = %y_body
  %9 = bitcast %u0Matrix* %6 to float*
  br label %y_body15

y_body15:                                         ; preds = %x_exit19, %y_body15.preheader
  %y17 = phi i64 [ 0, %y_body15.preheader ], [ %y_increment23, %x_exit19 ]
  %10 = mul nuw nsw i64 %y17, %mat_y_step
  br label %x_body18

x_body18:                                         ; preds = %y_body15, %x_body18
  %x20 = phi i64 [ %x_increment21, %x_body18 ], [ 0, %y_body15 ]
  %11 = add nuw nsw i64 %x20, %10
  %12 = getelementptr float, float* %9, i64 %11
  %13 = load float, float* %12, align 4, !llvm.mem.parallel_loop_access !1
  %14 = getelementptr %f32Matrix, %f32Matrix* %1, i64 0, i32 6, i64 %x20
  %15 = load float, float* %14, align 4, !llvm.mem.parallel_loop_access !1
  %16 = fsub fast float %13, %15
  store float %16, float* %12, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment21 = add nuw nsw i64 %x20, 1
  %x_postcondition22 = icmp eq i64 %x_increment21, %mat_y_step
  br i1 %x_postcondition22, label %x_exit19, label %x_body18

x_exit19:                                         ; preds = %x_body18
  %y_increment23 = add nuw nsw i64 %y17, 1
  %y_postcondition24 = icmp eq i64 %y_increment23, %5
  br i1 %y_postcondition24, label %y_exit16, label %y_body15

y_exit16:                                         ; preds = %x_exit19
  %17 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %18 = getelementptr inbounds %u0Matrix, %u0Matrix* %17, i64 1
  %19 = bitcast %u0Matrix* %18 to float*
  br label %y_body33

y_body33:                                         ; preds = %x_exit37, %y_exit16
  %y35 = phi i64 [ 0, %y_exit16 ], [ %y_increment43, %x_exit37 ]
  %20 = mul nuw nsw i64 %y35, %mat_y_step
  br label %x_body36

x_body36:                                         ; preds = %y_body33, %exit
  %x38 = phi i64 [ %x_increment41, %exit ], [ 0, %y_body33 ]
  %21 = icmp ugt i64 %y35, %x38
  br i1 %21, label %exit, label %true_entry39

true_entry39:                                     ; preds = %x_body36, %true_entry39
  %22 = phi i32 [ %36, %true_entry39 ], [ 0, %x_body36 ]
  %23 = phi double [ %35, %true_entry39 ], [ 0.000000e+00, %x_body36 ]
  %24 = zext i32 %22 to i64
  %25 = mul nuw nsw i64 %24, %mat_y_step
  %26 = add nuw nsw i64 %25, %x38
  %27 = getelementptr float, float* %9, i64 %26
  %28 = load float, float* %27, align 4, !llvm.mem.parallel_loop_access !2
  %29 = fpext float %28 to double
  %30 = add nuw nsw i64 %25, %y35
  %31 = getelementptr float, float* %9, i64 %30
  %32 = load float, float* %31, align 4, !llvm.mem.parallel_loop_access !2
  %33 = fpext float %32 to double
  %34 = fmul fast double %33, %29
  %35 = fadd fast double %34, %23
  %36 = add nuw nsw i32 %22, 1
  %37 = icmp eq i32 %36, %rows
  br i1 %37, label %exit40, label %true_entry39

exit40:                                           ; preds = %true_entry39
  %38 = add nuw nsw i64 %x38, %20
  %39 = getelementptr float, float* %19, i64 %38
  %40 = fptrunc double %35 to float
  store float %40, float* %39, align 4, !llvm.mem.parallel_loop_access !2
  %41 = mul nuw nsw i64 %x38, %mat_y_step
  %42 = add nuw nsw i64 %41, %y35
  %43 = getelementptr float, float* %19, i64 %42
  store float %40, float* %43, align 4, !llvm.mem.parallel_loop_access !2
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
  %dst = bitcast %u0Matrix* %17 to %f32Matrix*
  %44 = bitcast %u0Matrix* %4 to i8*
  call void @likely_release_mat(i8* %44) #1
  ret %f32Matrix* %dst
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1) #0

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
