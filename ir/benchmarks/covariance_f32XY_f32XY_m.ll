; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: norecurse nounwind
define private void @covariance_tmp_thunk0({ %f32Matrix*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  br label %entry.split

entry.split:                                      ; preds = %entry
  %3 = getelementptr inbounds { %f32Matrix*, i32 }, { %f32Matrix*, i32 }* %0, i64 0, i32 0
  %4 = load %f32Matrix*, %f32Matrix** %3, align 8
  %5 = getelementptr inbounds { %f32Matrix*, i32 }, { %f32Matrix*, i32 }* %0, i64 0, i32 1
  %6 = load i32, i32* %5, align 4
  %7 = sitofp i32 %6 to float
  br label %x_body

x_body:                                           ; preds = %x_body, %entry.split
  %x = phi i64 [ %1, %entry.split ], [ %x_increment, %x_body ]
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
  br label %entry.split

entry.split:                                      ; preds = %entry
  %3 = getelementptr inbounds { %f32Matrix*, float }, { %f32Matrix*, float }* %0, i64 0, i32 0
  %4 = load %f32Matrix*, %f32Matrix** %3, align 8
  %5 = getelementptr inbounds { %f32Matrix*, float }, { %f32Matrix*, float }* %0, i64 0, i32 1
  %6 = load float, float* %5, align 4
  br label %x_body

x_body:                                           ; preds = %x_body, %entry.split
  %x = phi i64 [ %1, %entry.split ], [ %x_increment, %x_body ]
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
define private void @covariance_tmp_thunk2({ %f32Matrix*, %f32Matrix* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  br label %entry.split

entry.split:                                      ; preds = %entry
  %3 = getelementptr inbounds { %f32Matrix*, %f32Matrix* }, { %f32Matrix*, %f32Matrix* }* %0, i64 0, i32 0
  %4 = load %f32Matrix*, %f32Matrix** %3, align 8
  %5 = getelementptr inbounds { %f32Matrix*, %f32Matrix* }, { %f32Matrix*, %f32Matrix* }* %0, i64 0, i32 1
  %6 = load %f32Matrix*, %f32Matrix** %5, align 8
  %7 = getelementptr inbounds %f32Matrix, %f32Matrix* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !2
  %mat_y_step = zext i32 %columns to i64
  %8 = getelementptr inbounds %f32Matrix, %f32Matrix* %6, i64 0, i32 3
  %columns1 = load i32, i32* %8, align 4, !range !2
  %val_y_step = zext i32 %columns1 to i64
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry.split
  %y = phi i64 [ %1, %entry.split ], [ %y_increment, %x_exit ]
  %9 = mul nuw nsw i64 %y, %val_y_step
  %10 = mul nuw nsw i64 %y, %mat_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %11 = add nuw nsw i64 %x, %9
  %12 = getelementptr %f32Matrix, %f32Matrix* %6, i64 0, i32 6, i64 %11
  %13 = load float, float* %12, align 4, !llvm.mem.parallel_loop_access !3
  %14 = add nuw nsw i64 %x, %10
  %15 = getelementptr %f32Matrix, %f32Matrix* %4, i64 0, i32 6, i64 %14
  store float %13, float* %15, align 4, !llvm.mem.parallel_loop_access !3
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
  br label %entry.split

entry.split:                                      ; preds = %entry
  %3 = getelementptr inbounds { %f32Matrix*, %f32Matrix* }, { %f32Matrix*, %f32Matrix* }* %0, i64 0, i32 0
  %4 = load %f32Matrix*, %f32Matrix** %3, align 8
  %5 = getelementptr inbounds { %f32Matrix*, %f32Matrix* }, { %f32Matrix*, %f32Matrix* }* %0, i64 0, i32 1
  %6 = load %f32Matrix*, %f32Matrix** %5, align 8
  %7 = getelementptr inbounds %f32Matrix, %f32Matrix* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !2
  %mat_y_step = zext i32 %columns to i64
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry.split
  %y = phi i64 [ %1, %entry.split ], [ %y_increment, %x_exit ]
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
  br label %entry.split

entry.split:                                      ; preds = %entry
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

y_body:                                           ; preds = %x_exit, %entry.split
  %y = phi i64 [ %1, %entry.split ], [ %y_increment, %x_exit ]
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
define noalias %f32Matrix* @covariance(%f32Matrix* noalias nocapture) #2 {
entry:
  %1 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !2
  %2 = call %u0Matrix* @likely_new(i32 8480, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !2
  %4 = zext i32 %columns to i64
  %5 = alloca { %f32Matrix*, i32 }, align 8
  %6 = bitcast { %f32Matrix*, i32 }* %5 to %u0Matrix**
  store %u0Matrix* %2, %u0Matrix** %6, align 8
  %7 = getelementptr inbounds { %f32Matrix*, i32 }, { %f32Matrix*, i32 }* %5, i64 0, i32 1
  store i32 0, i32* %7, align 8
  %8 = bitcast { %f32Matrix*, i32 }* %5 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, i32 }*, i64, i64)* @covariance_tmp_thunk0 to i8*), i8* %8, i64 %4) #3
  %9 = zext i32 %rows to i64
  %10 = getelementptr inbounds %u0Matrix, %u0Matrix* %2, i64 1
  %11 = bitcast %u0Matrix* %10 to float*
  %12 = sext i32 %rows to i64
  %13 = icmp slt i32 %rows, 1
  %14 = sext i32 %columns to i64
  %15 = icmp slt i32 %columns, 1
  %16 = or i1 %15, %13
  %notlhs = icmp slt i32 %rows, 2
  %notrhs = icmp sgt i32 %columns, -1
  %17 = and i1 %notrhs, %notlhs
  %18 = and i1 %16, %17
  br i1 %18, label %polly.merge, label %y_body

y_body:                                           ; preds = %entry, %x_exit
  %y = phi i64 [ %y_increment, %x_exit ], [ 0, %entry ]
  %19 = mul nuw nsw i64 %y, %4
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %20 = getelementptr float, float* %11, i64 %x
  %21 = load float, float* %20, align 4
  %22 = add nuw nsw i64 %x, %19
  %23 = getelementptr %f32Matrix, %f32Matrix* %0, i64 0, i32 6, i64 %22
  %24 = load float, float* %23, align 4
  %25 = fadd fast float %24, %21
  store float %25, float* %20, align 4
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %4
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %9
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %26 = icmp eq i32 %rows, 1
  br i1 %26, label %exit, label %true_entry

true_entry:                                       ; preds = %y_exit
  %27 = uitofp i32 %rows to float
  %28 = fdiv fast float 1.000000e+00, %27
  %29 = alloca { %f32Matrix*, float }, align 8
  %30 = bitcast { %f32Matrix*, float }* %29 to %u0Matrix**
  store %u0Matrix* %2, %u0Matrix** %30, align 8
  %31 = getelementptr inbounds { %f32Matrix*, float }, { %f32Matrix*, float }* %29, i64 0, i32 1
  store float %28, float* %31, align 8
  %32 = bitcast { %f32Matrix*, float }* %29 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, float }*, i64, i64)* @covariance_tmp_thunk1 to i8*), i8* %32, i64 %4) #3
  %columns7.pre = load i32, i32* %1, align 4, !range !2
  %rows8.pre = load i32, i32* %3, align 4, !range !2
  br label %exit

exit:                                             ; preds = %polly.loop_exit7, %polly.merge, %true_entry, %y_exit
  %rows8 = phi i32 [ %rows8.pre, %true_entry ], [ 1, %y_exit ], [ 1, %polly.merge ], [ 1, %polly.loop_exit7 ]
  %columns7 = phi i32 [ %columns7.pre, %true_entry ], [ %columns, %y_exit ], [ %columns, %polly.merge ], [ %columns, %polly.loop_exit7 ]
  %33 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns7, i32 %rows8, i32 1, i8* null)
  %34 = zext i32 %rows8 to i64
  %35 = alloca { %f32Matrix*, %f32Matrix* }, align 8
  %36 = bitcast { %f32Matrix*, %f32Matrix* }* %35 to %u0Matrix**
  store %u0Matrix* %33, %u0Matrix** %36, align 8
  %37 = getelementptr inbounds { %f32Matrix*, %f32Matrix* }, { %f32Matrix*, %f32Matrix* }* %35, i64 0, i32 1
  store %f32Matrix* %0, %f32Matrix** %37, align 8
  %38 = bitcast { %f32Matrix*, %f32Matrix* }* %35 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, %f32Matrix* }*, i64, i64)* @covariance_tmp_thunk2 to i8*), i8* %38, i64 %34) #3
  %39 = alloca { %f32Matrix*, %f32Matrix* }, align 8
  %40 = bitcast { %f32Matrix*, %f32Matrix* }* %39 to %u0Matrix**
  store %u0Matrix* %33, %u0Matrix** %40, align 8
  %41 = getelementptr inbounds { %f32Matrix*, %f32Matrix* }, { %f32Matrix*, %f32Matrix* }* %39, i64 0, i32 1
  %42 = bitcast %f32Matrix** %41 to %u0Matrix**
  store %u0Matrix* %2, %u0Matrix** %42, align 8
  %43 = bitcast { %f32Matrix*, %f32Matrix* }* %39 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, %f32Matrix* }*, i64, i64)* @covariance_tmp_thunk3 to i8*), i8* %43, i64 %34) #3
  %44 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns7, i32 %columns7, i32 1, i8* null)
  %dst = bitcast %u0Matrix* %44 to %f32Matrix*
  %45 = zext i32 %columns7 to i64
  %46 = alloca { %f32Matrix*, %f32Matrix*, i32 }, align 8
  %47 = bitcast { %f32Matrix*, %f32Matrix*, i32 }* %46 to %u0Matrix**
  store %u0Matrix* %44, %u0Matrix** %47, align 8
  %48 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, i32 }, { %f32Matrix*, %f32Matrix*, i32 }* %46, i64 0, i32 1
  %49 = bitcast %f32Matrix** %48 to %u0Matrix**
  store %u0Matrix* %33, %u0Matrix** %49, align 8
  %50 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, i32 }, { %f32Matrix*, %f32Matrix*, i32 }* %46, i64 0, i32 2
  store i32 %rows8, i32* %50, align 8
  %51 = bitcast { %f32Matrix*, %f32Matrix*, i32 }* %46 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, %f32Matrix*, i32 }*, i64, i64)* @covariance_tmp_thunk4 to i8*), i8* %51, i64 %45) #3
  %52 = bitcast %u0Matrix* %2 to i8*
  call void @likely_release_mat(i8* %52) #3
  %53 = bitcast %u0Matrix* %33 to i8*
  call void @likely_release_mat(i8* %53) #3
  ret %f32Matrix* %dst

polly.merge:                                      ; preds = %entry
  %54 = add nsw i64 %12, -1
  %polly.fdiv_q.shr = ashr i64 %54, 5
  %polly.loop_guard = icmp sgt i64 %polly.fdiv_q.shr, -1
  br i1 %polly.loop_guard, label %polly.loop_preheader, label %exit

polly.loop_header:                                ; preds = %polly.loop_exit7, %polly.loop_preheader
  %polly.indvar = phi i64 [ 0, %polly.loop_preheader ], [ %polly.indvar_next, %polly.loop_exit7 ]
  br i1 %polly.loop_guard8, label %polly.loop_header5.preheader, label %polly.loop_exit7

polly.loop_header5.preheader:                     ; preds = %polly.loop_header
  %55 = shl nsw i64 %polly.indvar, 5
  %56 = sub nsw i64 %12, %55
  %57 = add nsw i64 %56, -1
  %58 = icmp sgt i64 %57, 31
  %59 = select i1 %58, i64 31, i64 %57
  %polly.loop_guard17 = icmp sgt i64 %59, -1
  %polly.adjust_ub20 = add i64 %59, -1
  br i1 %polly.loop_guard17, label %polly.loop_header5.us, label %polly.loop_exit7

polly.loop_header5.us:                            ; preds = %polly.loop_header5.preheader, %polly.loop_exit16.loopexit.us
  %polly.indvar9.us = phi i64 [ %polly.indvar_next10.us, %polly.loop_exit16.loopexit.us ], [ 0, %polly.loop_header5.preheader ]
  %60 = shl nsw i64 %polly.indvar9.us, 5
  %61 = sub nsw i64 %14, %60
  %62 = add nsw i64 %61, -1
  %63 = icmp sgt i64 %62, 31
  %64 = select i1 %63, i64 31, i64 %62
  %polly.loop_guard26.us = icmp sgt i64 %64, -1
  %polly.adjust_ub29.us = add i64 %64, -1
  br i1 %polly.loop_guard26.us, label %polly.loop_header14.us.us, label %polly.loop_exit16.loopexit.us

polly.loop_exit16.loopexit.us:                    ; preds = %polly.loop_exit25.loopexit.us.us, %polly.loop_header5.us
  %polly.indvar_next10.us = add nuw nsw i64 %polly.indvar9.us, 1
  %polly.loop_cond12.us = icmp slt i64 %polly.indvar9.us, %polly.fdiv_q.shr3
  br i1 %polly.loop_cond12.us, label %polly.loop_header5.us, label %polly.loop_exit7

polly.loop_header14.us.us:                        ; preds = %polly.loop_header5.us, %polly.loop_exit25.loopexit.us.us
  %polly.indvar18.us.us = phi i64 [ %polly.indvar_next19.us.us, %polly.loop_exit25.loopexit.us.us ], [ 0, %polly.loop_header5.us ]
  %65 = add nuw nsw i64 %polly.indvar18.us.us, %55
  %66 = mul i64 %70, %65
  br label %polly.loop_header23.us.us

polly.loop_exit25.loopexit.us.us:                 ; preds = %polly.loop_header23.us.us
  %polly.indvar_next19.us.us = add nuw nsw i64 %polly.indvar18.us.us, 1
  %polly.loop_cond21.us.us = icmp sgt i64 %polly.indvar18.us.us, %polly.adjust_ub20
  br i1 %polly.loop_cond21.us.us, label %polly.loop_exit16.loopexit.us, label %polly.loop_header14.us.us

polly.loop_header23.us.us:                        ; preds = %polly.loop_header23.us.us, %polly.loop_header14.us.us
  %polly.indvar27.us.us = phi i64 [ %polly.indvar_next28.us.us, %polly.loop_header23.us.us ], [ 0, %polly.loop_header14.us.us ]
  %67 = add nuw nsw i64 %polly.indvar27.us.us, %60
  %68 = shl i64 %67, 2
  %uglygep.us.us = getelementptr i8, i8* %scevgep31, i64 %68
  %uglygep32.us.us = bitcast i8* %uglygep.us.us to float*
  %_p_scalar_.us.us = load float, float* %uglygep32.us.us, align 4, !alias.scope !6, !noalias !8
  %69 = add i64 %68, %66
  %uglygep35.us.us = getelementptr i8, i8* %scevgep3334, i64 %69
  %uglygep3536.us.us = bitcast i8* %uglygep35.us.us to float*
  %_p_scalar_37.us.us = load float, float* %uglygep3536.us.us, align 4, !alias.scope !10, !noalias !12
  %p_38.us.us = fadd fast float %_p_scalar_37.us.us, %_p_scalar_.us.us
  store float %p_38.us.us, float* %uglygep32.us.us, align 4, !alias.scope !6, !noalias !8
  %polly.indvar_next28.us.us = add nuw nsw i64 %polly.indvar27.us.us, 1
  %polly.loop_cond30.us.us = icmp sgt i64 %polly.indvar27.us.us, %polly.adjust_ub29.us
  br i1 %polly.loop_cond30.us.us, label %polly.loop_exit25.loopexit.us.us, label %polly.loop_header23.us.us

polly.loop_exit7:                                 ; preds = %polly.loop_exit16.loopexit.us, %polly.loop_header5.preheader, %polly.loop_header
  %polly.indvar_next = add nuw nsw i64 %polly.indvar, 1
  %polly.loop_cond = icmp slt i64 %polly.indvar, %polly.fdiv_q.shr
  br i1 %polly.loop_cond, label %polly.loop_header, label %exit

polly.loop_preheader:                             ; preds = %polly.merge
  %scevgep31 = bitcast %u0Matrix* %10 to i8*
  %scevgep33 = getelementptr %f32Matrix, %f32Matrix* %0, i64 1
  %scevgep3334 = bitcast %f32Matrix* %scevgep33 to i8*
  %70 = shl nuw nsw i64 %4, 2
  %71 = add nsw i64 %14, -1
  %polly.fdiv_q.shr3 = ashr i64 %71, 5
  %polly.loop_guard8 = icmp sgt i64 %polly.fdiv_q.shr3, -1
  br label %polly.loop_header
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { argmemonly nounwind }
attributes #1 = { norecurse nounwind }
attributes #2 = { nounwind "polly-optimized" }
attributes #3 = { nounwind }

!0 = distinct !{!0}
!1 = distinct !{!1}
!2 = !{i32 1, i32 -1}
!3 = distinct !{!3}
!4 = distinct !{!4}
!5 = distinct !{!5}
!6 = distinct !{!6, !7, !"polly.alias.scope."}
!7 = distinct !{!7, !"polly.alias.scope.domain"}
!8 = !{!9, !10, !11}
!9 = distinct !{!9, !7, !"polly.alias.scope.columns7"}
!10 = distinct !{!10, !7, !"polly.alias.scope."}
!11 = distinct !{!11, !7, !"polly.alias.scope.rows8"}
!12 = !{!6, !9, !11}
