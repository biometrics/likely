; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%u16Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: norecurse nounwind
define private void @covariance_tmp_thunk0({ %f32Matrix*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32Matrix*, i32 }, { %f32Matrix*, i32 }* %0, i64 0, i32 0
  %4 = load %f32Matrix*, %f32Matrix** %3, align 8
  %5 = getelementptr inbounds { %f32Matrix*, i32 }, { %f32Matrix*, i32 }* %0, i64 0, i32 1
  %6 = load i32, i32* %5, align 4
  %7 = sitofp i32 %6 to float
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %8 = getelementptr %f32Matrix, %f32Matrix* %4, i64 0, i32 6, i64 %x
  store float %7, float* %8, align 4, !llvm.mem.parallel_loop_access !0
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  ret void
}

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: norecurse nounwind
define private void @covariance_tmp_thunk1({ %f32Matrix*, float }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32Matrix*, float }, { %f32Matrix*, float }* %0, i64 0, i32 0
  %4 = load %f32Matrix*, %f32Matrix** %3, align 8
  %5 = getelementptr inbounds { %f32Matrix*, float }, { %f32Matrix*, float }* %0, i64 0, i32 1
  %6 = load float, float* %5, align 4
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %7 = getelementptr %f32Matrix, %f32Matrix* %4, i64 0, i32 6, i64 %x
  %8 = load float, float* %7, align 4, !llvm.mem.parallel_loop_access !1
  %9 = fmul fast float %8, %6
  store float %9, float* %7, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  ret void
}

; Function Attrs: norecurse nounwind
define private void @covariance_tmp_thunk2({ %f32Matrix*, %u16Matrix* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32Matrix*, %u16Matrix* }, { %f32Matrix*, %u16Matrix* }* %0, i64 0, i32 0
  %4 = load %f32Matrix*, %f32Matrix** %3, align 8
  %5 = getelementptr inbounds { %f32Matrix*, %u16Matrix* }, { %f32Matrix*, %u16Matrix* }* %0, i64 0, i32 1
  %6 = load %u16Matrix*, %u16Matrix** %5, align 8
  %7 = getelementptr inbounds %f32Matrix, %f32Matrix* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !2
  %mat_y_step = zext i32 %columns to i64
  %8 = getelementptr inbounds %u16Matrix, %u16Matrix* %6, i64 0, i32 3
  %columns1 = load i32, i32* %8, align 4, !range !2
  %val_y_step = zext i32 %columns1 to i64
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %9 = mul nuw nsw i64 %y, %val_y_step
  %10 = mul nuw nsw i64 %y, %mat_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %11 = add nuw nsw i64 %x, %9
  %12 = getelementptr %u16Matrix, %u16Matrix* %6, i64 0, i32 6, i64 %11
  %13 = load i16, i16* %12, align 2, !llvm.mem.parallel_loop_access !3
  %14 = add nuw nsw i64 %x, %10
  %15 = getelementptr %f32Matrix, %f32Matrix* %4, i64 0, i32 6, i64 %14
  %16 = sitofp i16 %13 to float
  store float %16, float* %15, align 4, !llvm.mem.parallel_loop_access !3
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
define private void @covariance_tmp_thunk3({ %f32Matrix*, %f32Matrix* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32Matrix*, %f32Matrix* }, { %f32Matrix*, %f32Matrix* }* %0, i64 0, i32 0
  %4 = load %f32Matrix*, %f32Matrix** %3, align 8
  %5 = getelementptr inbounds { %f32Matrix*, %f32Matrix* }, { %f32Matrix*, %f32Matrix* }* %0, i64 0, i32 1
  %6 = load %f32Matrix*, %f32Matrix** %5, align 8
  %7 = getelementptr inbounds %f32Matrix, %f32Matrix* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !2
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
  %11 = load float, float* %10, align 4, !llvm.mem.parallel_loop_access !4
  %12 = getelementptr %f32Matrix, %f32Matrix* %6, i64 0, i32 6, i64 %x
  %13 = load float, float* %12, align 4, !llvm.mem.parallel_loop_access !4
  %14 = fsub fast float %11, %13
  store float %14, float* %10, align 4, !llvm.mem.parallel_loop_access !4
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
define private void @covariance_tmp_thunk4({ %f32Matrix*, %f32Matrix*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, i32 }, { %f32Matrix*, %f32Matrix*, i32 }* %0, i64 0, i32 0
  %4 = load %f32Matrix*, %f32Matrix** %3, align 8
  %5 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, i32 }, { %f32Matrix*, %f32Matrix*, i32 }* %0, i64 0, i32 1
  %6 = load %f32Matrix*, %f32Matrix** %5, align 8
  %7 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, i32 }, { %f32Matrix*, %f32Matrix*, i32 }* %0, i64 0, i32 2
  %8 = load i32, i32* %7, align 4
  %9 = getelementptr inbounds %f32Matrix, %f32Matrix* %6, i64 0, i32 3
  %columns1 = load i32, i32* %9, align 4, !range !2
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
  %19 = load float, float* %18, align 4, !llvm.mem.parallel_loop_access !5
  %20 = fpext float %19 to double
  %21 = add nuw nsw i64 %16, %y
  %22 = getelementptr %f32Matrix, %f32Matrix* %6, i64 0, i32 6, i64 %21
  %23 = load float, float* %22, align 4, !llvm.mem.parallel_loop_access !5
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
  store float %31, float* %30, align 4, !llvm.mem.parallel_loop_access !5
  %32 = mul nuw nsw i64 %x, %dst_y_step
  %33 = add nuw nsw i64 %32, %y
  %34 = getelementptr %f32Matrix, %f32Matrix* %4, i64 0, i32 6, i64 %33
  store float %31, float* %34, align 4, !llvm.mem.parallel_loop_access !5
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
define noalias %f32Matrix* @covariance(%u16Matrix* noalias nocapture) #2 {
entry:
  %1 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !2
  %2 = call %u0Matrix* @likely_new(i32 9504, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !2
  %4 = zext i32 %columns to i64
  %5 = alloca { %f32Matrix*, i32 }, align 8
  %6 = bitcast { %f32Matrix*, i32 }* %5 to %u0Matrix**
  store %u0Matrix* %2, %u0Matrix** %6, align 8
  %7 = getelementptr inbounds { %f32Matrix*, i32 }, { %f32Matrix*, i32 }* %5, i64 0, i32 1
  store i32 0, i32* %7, align 8
  %8 = bitcast { %f32Matrix*, i32 }* %5 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, i32 }*, i64, i64)* @covariance_tmp_thunk0 to i8*), i8* %8, i64 %4) #2
  %9 = zext i32 %rows to i64
  %10 = getelementptr inbounds %u0Matrix, %u0Matrix* %2, i64 1
  %11 = bitcast %u0Matrix* %10 to float*
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %12 = mul nuw nsw i64 %y, %4
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %13 = getelementptr float, float* %11, i64 %x
  %14 = load float, float* %13, align 4
  %15 = add nuw nsw i64 %x, %12
  %16 = getelementptr %u16Matrix, %u16Matrix* %0, i64 0, i32 6, i64 %15
  %17 = load i16, i16* %16, align 2
  %18 = sitofp i16 %17 to float
  %19 = fadd fast float %18, %14
  store float %19, float* %13, align 4
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %4
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %9
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %20 = icmp eq i32 %rows, 1
  br i1 %20, label %exit, label %true_entry

true_entry:                                       ; preds = %y_exit
  %21 = uitofp i32 %rows to float
  %22 = fdiv fast float 1.000000e+00, %21
  %23 = alloca { %f32Matrix*, float }, align 8
  %24 = bitcast { %f32Matrix*, float }* %23 to %u0Matrix**
  store %u0Matrix* %2, %u0Matrix** %24, align 8
  %25 = getelementptr inbounds { %f32Matrix*, float }, { %f32Matrix*, float }* %23, i64 0, i32 1
  store float %22, float* %25, align 8
  %26 = bitcast { %f32Matrix*, float }* %23 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, float }*, i64, i64)* @covariance_tmp_thunk1 to i8*), i8* %26, i64 %4) #2
  %columns7.pre = load i32, i32* %1, align 4, !range !2
  %rows8.pre = load i32, i32* %3, align 4, !range !2
  br label %exit

exit:                                             ; preds = %y_exit, %true_entry
  %rows8 = phi i32 [ 1, %y_exit ], [ %rows8.pre, %true_entry ]
  %columns7 = phi i32 [ %columns, %y_exit ], [ %columns7.pre, %true_entry ]
  %27 = call %u0Matrix* @likely_new(i32 25888, i32 1, i32 %columns7, i32 %rows8, i32 1, i8* null)
  %28 = zext i32 %rows8 to i64
  %29 = alloca { %f32Matrix*, %u16Matrix* }, align 8
  %30 = bitcast { %f32Matrix*, %u16Matrix* }* %29 to %u0Matrix**
  store %u0Matrix* %27, %u0Matrix** %30, align 8
  %31 = getelementptr inbounds { %f32Matrix*, %u16Matrix* }, { %f32Matrix*, %u16Matrix* }* %29, i64 0, i32 1
  store %u16Matrix* %0, %u16Matrix** %31, align 8
  %32 = bitcast { %f32Matrix*, %u16Matrix* }* %29 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, %u16Matrix* }*, i64, i64)* @covariance_tmp_thunk2 to i8*), i8* %32, i64 %28) #2
  %33 = alloca { %f32Matrix*, %f32Matrix* }, align 8
  %34 = bitcast { %f32Matrix*, %f32Matrix* }* %33 to %u0Matrix**
  store %u0Matrix* %27, %u0Matrix** %34, align 8
  %35 = getelementptr inbounds { %f32Matrix*, %f32Matrix* }, { %f32Matrix*, %f32Matrix* }* %33, i64 0, i32 1
  %36 = bitcast %f32Matrix** %35 to %u0Matrix**
  store %u0Matrix* %2, %u0Matrix** %36, align 8
  %37 = bitcast { %f32Matrix*, %f32Matrix* }* %33 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, %f32Matrix* }*, i64, i64)* @covariance_tmp_thunk3 to i8*), i8* %37, i64 %28) #2
  %38 = call %u0Matrix* @likely_new(i32 25888, i32 1, i32 %columns7, i32 %columns7, i32 1, i8* null)
  %dst = bitcast %u0Matrix* %38 to %f32Matrix*
  %39 = zext i32 %columns7 to i64
  %40 = alloca { %f32Matrix*, %f32Matrix*, i32 }, align 8
  %41 = bitcast { %f32Matrix*, %f32Matrix*, i32 }* %40 to %u0Matrix**
  store %u0Matrix* %38, %u0Matrix** %41, align 8
  %42 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, i32 }, { %f32Matrix*, %f32Matrix*, i32 }* %40, i64 0, i32 1
  %43 = bitcast %f32Matrix** %42 to %u0Matrix**
  store %u0Matrix* %27, %u0Matrix** %43, align 8
  %44 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, i32 }, { %f32Matrix*, %f32Matrix*, i32 }* %40, i64 0, i32 2
  store i32 %rows8, i32* %44, align 8
  %45 = bitcast { %f32Matrix*, %f32Matrix*, i32 }* %40 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, %f32Matrix*, i32 }*, i64, i64)* @covariance_tmp_thunk4 to i8*), i8* %45, i64 %39) #2
  %46 = bitcast %u0Matrix* %2 to i8*
  call void @likely_release_mat(i8* %46) #2
  %47 = bitcast %u0Matrix* %27 to i8*
  call void @likely_release_mat(i8* %47) #2
  ret %f32Matrix* %dst
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { argmemonly nounwind }
attributes #1 = { norecurse nounwind }
attributes #2 = { nounwind }

!0 = distinct !{!0}
!1 = distinct !{!1}
!2 = !{i32 1, i32 -1}
!3 = distinct !{!3}
!4 = distinct !{!4}
!5 = distinct !{!5}
