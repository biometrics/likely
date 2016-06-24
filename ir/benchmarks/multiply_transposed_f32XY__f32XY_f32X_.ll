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
  %scevgep = getelementptr %u0Matrix, %u0Matrix* %4, i64 1, i32 0
  %scevgep3 = getelementptr %f32Matrix, %f32Matrix* %0, i64 1, i32 0
  %6 = shl nuw nsw i64 %mat_y_step, 2
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %7 = mul i64 %y, %mat_y_step
  %scevgep1 = getelementptr i32, i32* %scevgep, i64 %7
  %scevgep12 = bitcast i32* %scevgep1 to i8*
  %scevgep4 = getelementptr i32, i32* %scevgep3, i64 %7
  %scevgep45 = bitcast i32* %scevgep4 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep12, i8* %scevgep45, i64 %6, i32 4, i1 false)
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %5
  br i1 %y_postcondition, label %y_body15.preheader, label %y_body

y_body15.preheader:                               ; preds = %y_body
  %8 = getelementptr inbounds %u0Matrix, %u0Matrix* %4, i64 1
  %9 = bitcast %u0Matrix* %8 to float*
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
  %20 = sext i32 %rows to i64
  %21 = icmp slt i32 %rows, 1
  %22 = sext i32 %columns to i64
  %23 = icmp sgt i32 %columns, -1
  %24 = and i1 %23, %21
  br i1 %24, label %polly.merge, label %y_body33

y_body33:                                         ; preds = %y_exit16, %x_exit37
  %y35 = phi i64 [ %y_increment43, %x_exit37 ], [ 0, %y_exit16 ]
  %25 = mul nuw nsw i64 %y35, %mat_y_step
  br label %x_body36

x_body36:                                         ; preds = %y_body33, %exit
  %x38 = phi i64 [ %x_increment41, %exit ], [ 0, %y_body33 ]
  %26 = icmp ugt i64 %y35, %x38
  br i1 %26, label %exit, label %true_entry39

true_entry39:                                     ; preds = %x_body36, %true_entry39
  %27 = phi i32 [ %41, %true_entry39 ], [ 0, %x_body36 ]
  %28 = phi double [ %40, %true_entry39 ], [ 0.000000e+00, %x_body36 ]
  %29 = zext i32 %27 to i64
  %30 = mul nuw nsw i64 %29, %mat_y_step
  %31 = add nuw nsw i64 %30, %x38
  %32 = getelementptr float, float* %9, i64 %31
  %33 = load float, float* %32, align 4, !llvm.mem.parallel_loop_access !2
  %34 = fpext float %33 to double
  %35 = add nuw nsw i64 %30, %y35
  %36 = getelementptr float, float* %9, i64 %35
  %37 = load float, float* %36, align 4, !llvm.mem.parallel_loop_access !2
  %38 = fpext float %37 to double
  %39 = fmul fast double %38, %34
  %40 = fadd fast double %39, %28
  %41 = add nuw nsw i32 %27, 1
  %42 = icmp eq i32 %41, %rows
  br i1 %42, label %exit40, label %true_entry39

exit40:                                           ; preds = %true_entry39
  %43 = add nuw nsw i64 %x38, %25
  %44 = getelementptr float, float* %19, i64 %43
  %45 = fptrunc double %40 to float
  store float %45, float* %44, align 4, !llvm.mem.parallel_loop_access !2
  %46 = mul nuw nsw i64 %x38, %mat_y_step
  %47 = add nuw nsw i64 %46, %y35
  %48 = getelementptr float, float* %19, i64 %47
  store float %45, float* %48, align 4, !llvm.mem.parallel_loop_access !2
  br label %exit

exit:                                             ; preds = %exit40, %x_body36
  %x_increment41 = add nuw nsw i64 %x38, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %mat_y_step
  br i1 %x_postcondition42, label %x_exit37, label %x_body36

x_exit37:                                         ; preds = %exit
  %y_increment43 = add nuw nsw i64 %y35, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %mat_y_step
  br i1 %y_postcondition44, label %y_exit34, label %y_body33

y_exit34:                                         ; preds = %x_exit37, %polly.loop_exit54, %polly.merge
  %dst = bitcast %u0Matrix* %17 to %f32Matrix*
  %49 = bitcast %u0Matrix* %4 to i8*
  call void @likely_release_mat(i8* %49) #2
  ret %f32Matrix* %dst

polly.merge:                                      ; preds = %y_exit16
  %50 = add nsw i64 %22, -1
  %polly.fdiv_q.shr40 = ashr i64 %50, 5
  %polly.loop_guard45 = icmp sgt i64 %polly.fdiv_q.shr40, -1
  br i1 %polly.loop_guard45, label %polly.loop_preheader43, label %y_exit34

polly.loop_header42:                              ; preds = %polly.loop_exit54, %polly.loop_preheader43
  %polly.indvar46 = phi i64 [ 0, %polly.loop_preheader43 ], [ %polly.indvar_next47, %polly.loop_exit54 ]
  %polly.loop_guard55 = icmp sgt i64 %polly.indvar46, %pexp.p_div_q50
  br i1 %polly.loop_guard55, label %polly.loop_exit54, label %polly.loop_header52.preheader

polly.loop_header52.preheader:                    ; preds = %polly.loop_header42
  %51 = shl nsw i64 %polly.indvar46, 5
  %52 = sub nsw i64 %22, %51
  %53 = add nsw i64 %52, -1
  %54 = icmp sgt i64 %53, 31
  %55 = select i1 %54, i64 31, i64 %53
  %polly.loop_guard74 = icmp sgt i64 %55, -1
  %polly.adjust_ub77 = add i64 %55, -1
  br label %polly.loop_header52

polly.loop_exit54:                                ; preds = %polly.loop_exit64, %polly.loop_header42
  %polly.indvar_next47 = add nuw nsw i64 %polly.indvar46, 1
  %polly.loop_cond49 = icmp slt i64 %polly.indvar46, %polly.fdiv_q.shr40
  br i1 %polly.loop_cond49, label %polly.loop_header42, label %y_exit34

polly.loop_preheader43:                           ; preds = %polly.merge
  %pexp.p_div_q50 = lshr i64 %50, 5
  %56 = add nsw i64 %20, -1
  %polly.fdiv_q.shr60 = ashr i64 %56, 5
  %polly.loop_guard65 = icmp sgt i64 %polly.fdiv_q.shr60, -1
  br label %polly.loop_header42

polly.loop_header52:                              ; preds = %polly.loop_header52.preheader, %polly.loop_exit64
  %polly.indvar56 = phi i64 [ %polly.indvar_next57, %polly.loop_exit64 ], [ %polly.indvar46, %polly.loop_header52.preheader ]
  br i1 %polly.loop_guard65, label %polly.loop_header62.preheader, label %polly.loop_exit64

polly.loop_header62.preheader:                    ; preds = %polly.loop_header52
  %57 = shl nsw i64 %polly.indvar56, 5
  %58 = sub nsw i64 %51, %57
  %59 = sub nsw i64 %22, %57
  %60 = add nsw i64 %59, -1
  %61 = icmp sgt i64 %60, 31
  %62 = select i1 %61, i64 31, i64 %60
  %polly.adjust_ub86 = add i64 %62, -1
  br label %polly.loop_header62

polly.loop_exit64:                                ; preds = %polly.loop_exit73, %polly.loop_header52
  %polly.indvar_next57 = add nuw nsw i64 %polly.indvar56, 1
  %polly.loop_cond59 = icmp slt i64 %polly.indvar56, %pexp.p_div_q50
  br i1 %polly.loop_cond59, label %polly.loop_header52, label %polly.loop_exit54

polly.loop_header62:                              ; preds = %polly.loop_header62.preheader, %polly.loop_exit73
  %polly.indvar66 = phi i64 [ %polly.indvar_next67, %polly.loop_exit73 ], [ 0, %polly.loop_header62.preheader ]
  br i1 %polly.loop_guard74, label %polly.loop_header71.preheader, label %polly.loop_exit73

polly.loop_header71.preheader:                    ; preds = %polly.loop_header62
  %63 = shl nsw i64 %polly.indvar66, 5
  %64 = sub nsw i64 %20, %63
  %65 = add nsw i64 %64, -1
  %66 = icmp sgt i64 %65, 31
  %67 = select i1 %66, i64 31, i64 %65
  %polly.adjust_ub95 = add i64 %67, -1
  br label %polly.loop_header71

polly.loop_exit73:                                ; preds = %polly.loop_exit82, %polly.loop_header62
  %polly.indvar_next67 = add nuw nsw i64 %polly.indvar66, 1
  %polly.loop_cond69 = icmp slt i64 %polly.indvar66, %polly.fdiv_q.shr60
  br i1 %polly.loop_cond69, label %polly.loop_header62, label %polly.loop_exit64

polly.loop_header71:                              ; preds = %polly.loop_header71.preheader, %polly.loop_exit82
  %polly.indvar75 = phi i64 [ %polly.indvar_next76, %polly.loop_exit82 ], [ 0, %polly.loop_header71.preheader ]
  %68 = add nsw i64 %polly.indvar75, %58
  %69 = icmp slt i64 %68, 0
  %70 = select i1 %69, i64 0, i64 %68
  %polly.loop_guard83 = icmp sgt i64 %70, %62
  %polly.loop_guard92.not = icmp slt i64 %67, 0
  %brmerge = or i1 %polly.loop_guard83, %polly.loop_guard92.not
  br i1 %brmerge, label %polly.loop_exit82, label %polly.loop_header80.us

polly.loop_header80.us:                           ; preds = %polly.loop_header71, %polly.loop_exit91.loopexit.us
  %polly.indvar84.us = phi i64 [ %polly.indvar_next85.us, %polly.loop_exit91.loopexit.us ], [ %70, %polly.loop_header71 ]
  br label %polly.loop_header89.us

polly.loop_header89.us:                           ; preds = %polly.loop_header80.us, %polly.loop_header89.us
  %polly.indvar93.us = phi i64 [ %polly.indvar_next94.us, %polly.loop_header89.us ], [ 0, %polly.loop_header80.us ]
  %polly.indvar_next94.us = add nuw nsw i64 %polly.indvar93.us, 1
  %polly.loop_cond96.us = icmp sgt i64 %polly.indvar93.us, %polly.adjust_ub95
  br i1 %polly.loop_cond96.us, label %polly.loop_exit91.loopexit.us, label %polly.loop_header89.us

polly.loop_exit91.loopexit.us:                    ; preds = %polly.loop_header89.us
  %polly.indvar_next85.us = add nuw nsw i64 %polly.indvar84.us, 1
  %polly.loop_cond87.us = icmp sgt i64 %polly.indvar84.us, %polly.adjust_ub86
  br i1 %polly.loop_cond87.us, label %polly.loop_exit82, label %polly.loop_header80.us

polly.loop_exit82:                                ; preds = %polly.loop_exit91.loopexit.us, %polly.loop_header71
  %polly.indvar_next76 = add nuw nsw i64 %polly.indvar75, 1
  %polly.loop_cond78 = icmp sgt i64 %polly.indvar75, %polly.adjust_ub77
  br i1 %polly.loop_cond78, label %polly.loop_exit73, label %polly.loop_header71
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1) #0

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind "polly-optimized" }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
