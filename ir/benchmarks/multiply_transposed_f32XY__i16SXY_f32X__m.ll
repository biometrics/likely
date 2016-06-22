; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%u16Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: norecurse nounwind
define private void @multiply_transposed_tmp_thunk0({ %f32Matrix*, %u16Matrix* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32Matrix*, %u16Matrix* }, { %f32Matrix*, %u16Matrix* }* %0, i64 0, i32 0
  %4 = load %f32Matrix*, %f32Matrix** %3, align 8
  %5 = getelementptr inbounds { %f32Matrix*, %u16Matrix* }, { %f32Matrix*, %u16Matrix* }* %0, i64 0, i32 1
  %6 = load %u16Matrix*, %u16Matrix** %5, align 8
  %7 = getelementptr inbounds %u16Matrix, %u16Matrix* %6, i64 0, i32 3
  %columns1 = load i32, i32* %7, align 4, !range !0
  %mat_y_step = zext i32 %columns1 to i64
  %8 = mul nuw nsw i64 %mat_y_step, %2
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %y_body ]
  %9 = getelementptr %u16Matrix, %u16Matrix* %6, i64 0, i32 6, i64 %y
  %10 = load i16, i16* %9, align 2, !llvm.mem.parallel_loop_access !1
  %11 = getelementptr %f32Matrix, %f32Matrix* %4, i64 0, i32 6, i64 %y
  %12 = sitofp i16 %10 to float
  store float %12, float* %11, align 4, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %8
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  ret void
}

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: norecurse nounwind
define private void @multiply_transposed_tmp_thunk1({ %f32Matrix*, %f32Matrix* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32Matrix*, %f32Matrix* }, { %f32Matrix*, %f32Matrix* }* %0, i64 0, i32 0
  %4 = load %f32Matrix*, %f32Matrix** %3, align 8
  %5 = getelementptr inbounds { %f32Matrix*, %f32Matrix* }, { %f32Matrix*, %f32Matrix* }* %0, i64 0, i32 1
  %6 = load %f32Matrix*, %f32Matrix** %5, align 8
  %7 = getelementptr inbounds %f32Matrix, %f32Matrix* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !0
  %mat_y_step = zext i32 %columns to i64
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %8 = mul nuw nsw i64 %y, %mat_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %9 = add nuw nsw i64 %x, %8
  %10 = getelementptr %f32Matrix, %f32Matrix* %4, i64 0, i32 6, i64 %9
  %11 = load float, float* %10, align 4, !llvm.mem.parallel_loop_access !2
  %12 = getelementptr %f32Matrix, %f32Matrix* %6, i64 0, i32 6, i64 %x
  %13 = load float, float* %12, align 4, !llvm.mem.parallel_loop_access !2
  %14 = fsub fast float %11, %13
  store float %14, float* %10, align 4, !llvm.mem.parallel_loop_access !2
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %mat_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

; Function Attrs: norecurse nounwind
define private void @multiply_transposed_tmp_thunk2({ %f32Matrix*, %f32Matrix*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, i32 }, { %f32Matrix*, %f32Matrix*, i32 }* %0, i64 0, i32 0
  %4 = load %f32Matrix*, %f32Matrix** %3, align 8
  %5 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, i32 }, { %f32Matrix*, %f32Matrix*, i32 }* %0, i64 0, i32 1
  %6 = load %f32Matrix*, %f32Matrix** %5, align 8
  %7 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, i32 }, { %f32Matrix*, %f32Matrix*, i32 }* %0, i64 0, i32 2
  %8 = load i32, i32* %7, align 4
  %9 = getelementptr inbounds %f32Matrix, %f32Matrix* %6, i64 0, i32 3
  %columns1 = load i32, i32* %9, align 4, !range !0
  %dst_y_step = zext i32 %columns1 to i64
  %10 = icmp eq i32 %8, 0
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %11 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %exit
  %x = phi i64 [ %x_increment, %exit ], [ 0, %y_body ]
  %12 = icmp ugt i64 %y, %x
  br i1 %12, label %exit, label %loop.preheader

loop.preheader:                                   ; preds = %x_body
  br i1 %10, label %exit4, label %true_entry3

true_entry3:                                      ; preds = %loop.preheader, %true_entry3
  %13 = phi i32 [ %27, %true_entry3 ], [ 0, %loop.preheader ]
  %14 = phi double [ %26, %true_entry3 ], [ 0.000000e+00, %loop.preheader ]
  %15 = zext i32 %13 to i64
  %16 = mul nuw nsw i64 %15, %dst_y_step
  %17 = add nuw nsw i64 %16, %x
  %18 = getelementptr %f32Matrix, %f32Matrix* %6, i64 0, i32 6, i64 %17
  %19 = load float, float* %18, align 4, !llvm.mem.parallel_loop_access !3
  %20 = fpext float %19 to double
  %21 = add nuw nsw i64 %16, %y
  %22 = getelementptr %f32Matrix, %f32Matrix* %6, i64 0, i32 6, i64 %21
  %23 = load float, float* %22, align 4, !llvm.mem.parallel_loop_access !3
  %24 = fpext float %23 to double
  %25 = fmul fast double %24, %20
  %26 = fadd fast double %25, %14
  %27 = add nuw nsw i32 %13, 1
  %28 = icmp eq i32 %27, %8
  br i1 %28, label %exit4, label %true_entry3

exit4:                                            ; preds = %true_entry3, %loop.preheader
  %.lcssa = phi double [ 0.000000e+00, %loop.preheader ], [ %26, %true_entry3 ]
  %29 = add nuw nsw i64 %x, %11
  %30 = getelementptr %f32Matrix, %f32Matrix* %4, i64 0, i32 6, i64 %29
  %31 = fptrunc double %.lcssa to float
  store float %31, float* %30, align 4, !llvm.mem.parallel_loop_access !3
  %32 = mul nuw nsw i64 %x, %dst_y_step
  %33 = add nuw nsw i64 %32, %y
  %34 = getelementptr %f32Matrix, %f32Matrix* %4, i64 0, i32 6, i64 %33
  store float %31, float* %34, align 4, !llvm.mem.parallel_loop_access !3
  br label %exit

exit:                                             ; preds = %exit4, %x_body
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

; Function Attrs: nounwind
define noalias %f32Matrix* @multiply_transposed(%u16Matrix* noalias nocapture, %f32Matrix* noalias nocapture) #2 {
entry:
  %2 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %5 = zext i32 %rows to i64
  %6 = alloca { %f32Matrix*, %u16Matrix* }, align 8
  %7 = bitcast { %f32Matrix*, %u16Matrix* }* %6 to %u0Matrix**
  store %u0Matrix* %4, %u0Matrix** %7, align 8
  %8 = getelementptr inbounds { %f32Matrix*, %u16Matrix* }, { %f32Matrix*, %u16Matrix* }* %6, i64 0, i32 1
  store %u16Matrix* %0, %u16Matrix** %8, align 8
  %9 = bitcast { %f32Matrix*, %u16Matrix* }* %6 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, %u16Matrix* }*, i64, i64)* @multiply_transposed_tmp_thunk0 to i8*), i8* %9, i64 %5) #2
  %10 = alloca { %f32Matrix*, %f32Matrix* }, align 8
  %11 = bitcast { %f32Matrix*, %f32Matrix* }* %10 to %u0Matrix**
  store %u0Matrix* %4, %u0Matrix** %11, align 8
  %12 = getelementptr inbounds { %f32Matrix*, %f32Matrix* }, { %f32Matrix*, %f32Matrix* }* %10, i64 0, i32 1
  store %f32Matrix* %1, %f32Matrix** %12, align 8
  %13 = bitcast { %f32Matrix*, %f32Matrix* }* %10 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, %f32Matrix* }*, i64, i64)* @multiply_transposed_tmp_thunk1 to i8*), i8* %13, i64 %5) #2
  %14 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %dst = bitcast %u0Matrix* %14 to %f32Matrix*
  %15 = zext i32 %columns to i64
  %16 = alloca { %f32Matrix*, %f32Matrix*, i32 }, align 8
  %17 = bitcast { %f32Matrix*, %f32Matrix*, i32 }* %16 to %u0Matrix**
  store %u0Matrix* %14, %u0Matrix** %17, align 8
  %18 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, i32 }, { %f32Matrix*, %f32Matrix*, i32 }* %16, i64 0, i32 1
  %19 = bitcast %f32Matrix** %18 to %u0Matrix**
  store %u0Matrix* %4, %u0Matrix** %19, align 8
  %20 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, i32 }, { %f32Matrix*, %f32Matrix*, i32 }* %16, i64 0, i32 2
  store i32 %rows, i32* %20, align 8
  %21 = bitcast { %f32Matrix*, %f32Matrix*, i32 }* %16 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, %f32Matrix*, i32 }*, i64, i64)* @multiply_transposed_tmp_thunk2 to i8*), i8* %21, i64 %15) #2
  %22 = bitcast %u0Matrix* %4 to i8*
  call void @likely_release_mat(i8* %22) #2
  ret %f32Matrix* %dst
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { argmemonly nounwind }
attributes #1 = { norecurse nounwind }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
