; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

define noalias %f32Matrix* @multiply_transposed(%f32Matrix* nocapture readonly, %f32Matrix* nocapture readonly) {
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

x_body36:                                         ; preds = %y_body33, %Flow
  %x38 = phi i64 [ %x_increment41, %Flow ], [ 0, %y_body33 ]
  %21 = icmp ugt i64 %y35, %x38
  br i1 %21, label %Flow, label %true_entry39

x_exit37:                                         ; preds = %Flow
  %y_increment43 = add nuw nsw i64 %y35, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %mat_y_step
  br i1 %y_postcondition44, label %y_exit34, label %y_body33

y_exit34:                                         ; preds = %x_exit37
  %dst = bitcast %u0Matrix* %17 to %f32Matrix*
  %22 = bitcast %u0Matrix* %4 to i8*
  call void @likely_release_mat(i8* %22)
  ret %f32Matrix* %dst

true_entry39:                                     ; preds = %x_body36, %true_entry39
  %23 = phi i32 [ %37, %true_entry39 ], [ 0, %x_body36 ]
  %24 = phi double [ %36, %true_entry39 ], [ 0.000000e+00, %x_body36 ]
  %25 = sext i32 %23 to i64
  %26 = mul nuw nsw i64 %25, %mat_y_step
  %27 = add nuw nsw i64 %26, %x38
  %28 = getelementptr float, float* %9, i64 %27
  %29 = load float, float* %28, align 4, !llvm.mem.parallel_loop_access !2
  %30 = fpext float %29 to double
  %31 = add nuw nsw i64 %26, %y35
  %32 = getelementptr float, float* %9, i64 %31
  %33 = load float, float* %32, align 4, !llvm.mem.parallel_loop_access !2
  %34 = fpext float %33 to double
  %35 = fmul fast double %34, %30
  %36 = fadd fast double %35, %24
  %37 = add nuw nsw i32 %23, 1
  %38 = icmp eq i32 %37, %rows
  br i1 %38, label %exit40, label %true_entry39

Flow:                                             ; preds = %x_body36, %exit40
  %x_increment41 = add nuw nsw i64 %x38, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %mat_y_step
  br i1 %x_postcondition42, label %x_exit37, label %x_body36

exit40:                                           ; preds = %true_entry39
  %39 = add nuw nsw i64 %x38, %20
  %40 = getelementptr float, float* %19, i64 %39
  %41 = fptrunc double %36 to float
  store float %41, float* %40, align 4, !llvm.mem.parallel_loop_access !2
  %42 = mul nuw nsw i64 %x38, %mat_y_step
  %43 = add nuw nsw i64 %42, %y35
  %44 = getelementptr float, float* %19, i64 %43
  store float %41, float* %44, align 4, !llvm.mem.parallel_loop_access !2
  br label %Flow
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1) #0

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { argmemonly nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
