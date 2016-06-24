; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: norecurse nounwind
define private void @average_tmp_thunk0({ %f32Matrix*, i32 }* noalias nocapture readonly, i64, i64) #1 {
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
define private void @average_tmp_thunk1({ %f32Matrix*, float }* noalias nocapture readonly, i64, i64) #1 {
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

; Function Attrs: nounwind
define noalias %f32Matrix* @average(%f32Matrix* noalias nocapture readonly) #2 {
entry:
  %1 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !2
  %2 = call %u0Matrix* @likely_new(i32 8480, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = bitcast %u0Matrix* %2 to %f32Matrix*
  %4 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !2
  %5 = zext i32 %columns to i64
  %6 = alloca { %f32Matrix*, i32 }, align 8
  %7 = bitcast { %f32Matrix*, i32 }* %6 to %u0Matrix**
  store %u0Matrix* %2, %u0Matrix** %7, align 8
  %8 = getelementptr inbounds { %f32Matrix*, i32 }, { %f32Matrix*, i32 }* %6, i64 0, i32 1
  store i32 0, i32* %8, align 8
  %9 = bitcast { %f32Matrix*, i32 }* %6 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, i32 }*, i64, i64)* @average_tmp_thunk0 to i8*), i8* %9, i64 %5) #3
  %10 = zext i32 %rows to i64
  %11 = getelementptr inbounds %u0Matrix, %u0Matrix* %2, i64 1
  %12 = bitcast %u0Matrix* %11 to float*
  %13 = sext i32 %rows to i64
  %14 = icmp slt i32 %rows, 1
  %15 = sext i32 %columns to i64
  %16 = icmp slt i32 %columns, 1
  %17 = or i1 %16, %14
  %notlhs = icmp slt i32 %rows, 2
  %notrhs = icmp sgt i32 %columns, -1
  %18 = and i1 %notrhs, %notlhs
  %19 = and i1 %17, %18
  br i1 %19, label %polly.merge, label %y_body

y_body:                                           ; preds = %entry, %x_exit
  %y = phi i64 [ %y_increment, %x_exit ], [ 0, %entry ]
  %20 = mul nuw nsw i64 %y, %5
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %21 = getelementptr float, float* %12, i64 %x
  %22 = load float, float* %21, align 4
  %23 = add nuw nsw i64 %x, %20
  %24 = getelementptr %f32Matrix, %f32Matrix* %0, i64 0, i32 6, i64 %23
  %25 = load float, float* %24, align 4
  %26 = fadd fast float %25, %22
  store float %26, float* %21, align 4
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %5
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %10
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %27 = icmp eq i32 %rows, 1
  br i1 %27, label %exit, label %true_entry

true_entry:                                       ; preds = %y_exit
  %28 = uitofp i32 %rows to float
  %29 = fdiv fast float 1.000000e+00, %28
  %30 = alloca { %f32Matrix*, float }, align 8
  %31 = bitcast { %f32Matrix*, float }* %30 to %u0Matrix**
  store %u0Matrix* %2, %u0Matrix** %31, align 8
  %32 = getelementptr inbounds { %f32Matrix*, float }, { %f32Matrix*, float }* %30, i64 0, i32 1
  store float %29, float* %32, align 8
  %33 = bitcast { %f32Matrix*, float }* %30 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, float }*, i64, i64)* @average_tmp_thunk1 to i8*), i8* %33, i64 %5) #3
  br label %exit

exit:                                             ; preds = %polly.loop_exit5, %polly.merge, %true_entry, %y_exit
  ret %f32Matrix* %3

polly.merge:                                      ; preds = %entry
  %34 = add nsw i64 %13, -1
  %polly.fdiv_q.shr = ashr i64 %34, 5
  %polly.loop_guard = icmp sgt i64 %polly.fdiv_q.shr, -1
  br i1 %polly.loop_guard, label %polly.loop_preheader, label %exit

polly.loop_header:                                ; preds = %polly.loop_exit5, %polly.loop_preheader
  %polly.indvar = phi i64 [ 0, %polly.loop_preheader ], [ %polly.indvar_next, %polly.loop_exit5 ]
  br i1 %polly.loop_guard6, label %polly.loop_header3.preheader, label %polly.loop_exit5

polly.loop_header3.preheader:                     ; preds = %polly.loop_header
  %35 = shl nsw i64 %polly.indvar, 5
  %36 = sub nsw i64 %13, %35
  %37 = add nsw i64 %36, -1
  %38 = icmp sgt i64 %37, 31
  %39 = select i1 %38, i64 31, i64 %37
  %polly.loop_guard15 = icmp sgt i64 %39, -1
  %polly.adjust_ub18 = add i64 %39, -1
  br i1 %polly.loop_guard15, label %polly.loop_header3.us, label %polly.loop_exit5

polly.loop_header3.us:                            ; preds = %polly.loop_header3.preheader, %polly.loop_exit14.loopexit.us
  %polly.indvar7.us = phi i64 [ %polly.indvar_next8.us, %polly.loop_exit14.loopexit.us ], [ 0, %polly.loop_header3.preheader ]
  %40 = shl nsw i64 %polly.indvar7.us, 5
  %41 = sub nsw i64 %15, %40
  %42 = add nsw i64 %41, -1
  %43 = icmp sgt i64 %42, 31
  %44 = select i1 %43, i64 31, i64 %42
  %polly.loop_guard24.us = icmp sgt i64 %44, -1
  %polly.adjust_ub27.us = add i64 %44, -1
  br i1 %polly.loop_guard24.us, label %polly.loop_header12.us.us, label %polly.loop_exit14.loopexit.us

polly.loop_exit14.loopexit.us:                    ; preds = %polly.loop_exit23.loopexit.us.us, %polly.loop_header3.us
  %polly.indvar_next8.us = add nuw nsw i64 %polly.indvar7.us, 1
  %polly.loop_cond10.us = icmp slt i64 %polly.indvar7.us, %polly.fdiv_q.shr1
  br i1 %polly.loop_cond10.us, label %polly.loop_header3.us, label %polly.loop_exit5

polly.loop_header12.us.us:                        ; preds = %polly.loop_header3.us, %polly.loop_exit23.loopexit.us.us
  %polly.indvar16.us.us = phi i64 [ %polly.indvar_next17.us.us, %polly.loop_exit23.loopexit.us.us ], [ 0, %polly.loop_header3.us ]
  %45 = add nuw nsw i64 %polly.indvar16.us.us, %35
  %46 = mul i64 %50, %45
  br label %polly.loop_header21.us.us

polly.loop_exit23.loopexit.us.us:                 ; preds = %polly.loop_header21.us.us
  %polly.indvar_next17.us.us = add nuw nsw i64 %polly.indvar16.us.us, 1
  %polly.loop_cond19.us.us = icmp sgt i64 %polly.indvar16.us.us, %polly.adjust_ub18
  br i1 %polly.loop_cond19.us.us, label %polly.loop_exit14.loopexit.us, label %polly.loop_header12.us.us

polly.loop_header21.us.us:                        ; preds = %polly.loop_header21.us.us, %polly.loop_header12.us.us
  %polly.indvar25.us.us = phi i64 [ %polly.indvar_next26.us.us, %polly.loop_header21.us.us ], [ 0, %polly.loop_header12.us.us ]
  %47 = add nuw nsw i64 %polly.indvar25.us.us, %40
  %48 = shl i64 %47, 2
  %uglygep.us.us = getelementptr i8, i8* %scevgep29, i64 %48
  %uglygep30.us.us = bitcast i8* %uglygep.us.us to float*
  %_p_scalar_.us.us = load float, float* %uglygep30.us.us, align 4, !alias.scope !3, !noalias !5
  %49 = add i64 %48, %46
  %uglygep33.us.us = getelementptr i8, i8* %scevgep3132, i64 %49
  %uglygep3334.us.us = bitcast i8* %uglygep33.us.us to float*
  %_p_scalar_35.us.us = load float, float* %uglygep3334.us.us, align 4, !alias.scope !6, !noalias !8
  %p_36.us.us = fadd fast float %_p_scalar_35.us.us, %_p_scalar_.us.us
  store float %p_36.us.us, float* %uglygep30.us.us, align 4, !alias.scope !3, !noalias !5
  %polly.indvar_next26.us.us = add nuw nsw i64 %polly.indvar25.us.us, 1
  %polly.loop_cond28.us.us = icmp sgt i64 %polly.indvar25.us.us, %polly.adjust_ub27.us
  br i1 %polly.loop_cond28.us.us, label %polly.loop_exit23.loopexit.us.us, label %polly.loop_header21.us.us

polly.loop_exit5:                                 ; preds = %polly.loop_exit14.loopexit.us, %polly.loop_header3.preheader, %polly.loop_header
  %polly.indvar_next = add nuw nsw i64 %polly.indvar, 1
  %polly.loop_cond = icmp slt i64 %polly.indvar, %polly.fdiv_q.shr
  br i1 %polly.loop_cond, label %polly.loop_header, label %exit

polly.loop_preheader:                             ; preds = %polly.merge
  %scevgep29 = bitcast %u0Matrix* %11 to i8*
  %scevgep31 = getelementptr %f32Matrix, %f32Matrix* %0, i64 1
  %scevgep3132 = bitcast %f32Matrix* %scevgep31 to i8*
  %50 = shl nuw nsw i64 %5, 2
  %51 = add nsw i64 %15, -1
  %polly.fdiv_q.shr1 = ashr i64 %51, 5
  %polly.loop_guard6 = icmp sgt i64 %polly.fdiv_q.shr1, -1
  br label %polly.loop_header
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { norecurse nounwind }
attributes #2 = { nounwind "polly-optimized" }
attributes #3 = { nounwind }

!0 = distinct !{!0}
!1 = distinct !{!1}
!2 = !{i32 1, i32 -1}
!3 = distinct !{!3, !4, !"polly.alias.scope."}
!4 = distinct !{!4, !"polly.alias.scope.domain"}
!5 = !{!6, !7}
!6 = distinct !{!6, !4, !"polly.alias.scope."}
!7 = distinct !{!7, !4, !"polly.alias.scope."}
!8 = !{!7, !3}
