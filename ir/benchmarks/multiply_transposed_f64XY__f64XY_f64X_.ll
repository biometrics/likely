; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64Matrix = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define noalias %f64Matrix* @multiply_transposed(%f64Matrix* noalias nocapture readonly, %f64Matrix* noalias nocapture readonly) #1 {
entry:
  %2 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = call %u0Matrix* @likely_new(i32 24896, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %5 = zext i32 %rows to i64
  %mat_y_step = zext i32 %columns to i64
  %scevgep = getelementptr %u0Matrix, %u0Matrix* %4, i64 1, i32 0
  %6 = shl nuw nsw i64 %mat_y_step, 1
  %scevgep3 = getelementptr %f64Matrix, %f64Matrix* %0, i64 1, i32 0
  %7 = shl nuw nsw i64 %mat_y_step, 3
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %8 = mul i64 %y, %6
  %scevgep1 = getelementptr i32, i32* %scevgep, i64 %8
  %scevgep12 = bitcast i32* %scevgep1 to i8*
  %scevgep4 = getelementptr i32, i32* %scevgep3, i64 %8
  %scevgep45 = bitcast i32* %scevgep4 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep12, i8* %scevgep45, i64 %7, i32 8, i1 false)
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %5
  br i1 %y_postcondition, label %y_body15.preheader, label %y_body

y_body15.preheader:                               ; preds = %y_body
  %9 = getelementptr inbounds %u0Matrix, %u0Matrix* %4, i64 1
  %10 = bitcast %u0Matrix* %9 to double*
  br label %y_body15

y_body15:                                         ; preds = %x_exit19, %y_body15.preheader
  %y17 = phi i64 [ 0, %y_body15.preheader ], [ %y_increment23, %x_exit19 ]
  %11 = mul nuw nsw i64 %y17, %mat_y_step
  br label %x_body18

x_body18:                                         ; preds = %y_body15, %x_body18
  %x20 = phi i64 [ %x_increment21, %x_body18 ], [ 0, %y_body15 ]
  %12 = add nuw nsw i64 %x20, %11
  %13 = getelementptr double, double* %10, i64 %12
  %14 = load double, double* %13, align 8, !llvm.mem.parallel_loop_access !1
  %15 = getelementptr %f64Matrix, %f64Matrix* %1, i64 0, i32 6, i64 %x20
  %16 = load double, double* %15, align 8, !llvm.mem.parallel_loop_access !1
  %17 = fsub fast double %14, %16
  store double %17, double* %13, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment21 = add nuw nsw i64 %x20, 1
  %x_postcondition22 = icmp eq i64 %x_increment21, %mat_y_step
  br i1 %x_postcondition22, label %x_exit19, label %x_body18

x_exit19:                                         ; preds = %x_body18
  %y_increment23 = add nuw nsw i64 %y17, 1
  %y_postcondition24 = icmp eq i64 %y_increment23, %5
  br i1 %y_postcondition24, label %y_exit16, label %y_body15

y_exit16:                                         ; preds = %x_exit19
  %18 = call %u0Matrix* @likely_new(i32 24896, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %19 = getelementptr inbounds %u0Matrix, %u0Matrix* %18, i64 1
  %20 = bitcast %u0Matrix* %19 to double*
  %21 = sext i32 %rows to i64
  %22 = icmp slt i32 %rows, 1
  %23 = sext i32 %columns to i64
  %24 = icmp sgt i32 %columns, -1
  %25 = and i1 %24, %22
  br i1 %25, label %polly.merge, label %y_body33

y_body33:                                         ; preds = %y_exit16, %x_exit37
  %y35 = phi i64 [ %y_increment43, %x_exit37 ], [ 0, %y_exit16 ]
  %26 = mul nuw nsw i64 %y35, %mat_y_step
  br label %x_body36

x_body36:                                         ; preds = %y_body33, %exit
  %x38 = phi i64 [ %x_increment41, %exit ], [ 0, %y_body33 ]
  %27 = icmp ugt i64 %y35, %x38
  br i1 %27, label %exit, label %true_entry39

true_entry39:                                     ; preds = %x_body36, %true_entry39
  %28 = phi i32 [ %40, %true_entry39 ], [ 0, %x_body36 ]
  %29 = phi double [ %39, %true_entry39 ], [ 0.000000e+00, %x_body36 ]
  %30 = zext i32 %28 to i64
  %31 = mul nuw nsw i64 %30, %mat_y_step
  %32 = add nuw nsw i64 %31, %x38
  %33 = getelementptr double, double* %10, i64 %32
  %34 = load double, double* %33, align 8, !llvm.mem.parallel_loop_access !2
  %35 = add nuw nsw i64 %31, %y35
  %36 = getelementptr double, double* %10, i64 %35
  %37 = load double, double* %36, align 8, !llvm.mem.parallel_loop_access !2
  %38 = fmul fast double %37, %34
  %39 = fadd fast double %38, %29
  %40 = add nuw nsw i32 %28, 1
  %41 = icmp eq i32 %40, %rows
  br i1 %41, label %exit40, label %true_entry39

exit40:                                           ; preds = %true_entry39
  %42 = add nuw nsw i64 %x38, %26
  %43 = getelementptr double, double* %20, i64 %42
  store double %39, double* %43, align 8, !llvm.mem.parallel_loop_access !2
  %44 = mul nuw nsw i64 %x38, %mat_y_step
  %45 = add nuw nsw i64 %44, %y35
  %46 = getelementptr double, double* %20, i64 %45
  store double %39, double* %46, align 8, !llvm.mem.parallel_loop_access !2
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
  %dst = bitcast %u0Matrix* %18 to %f64Matrix*
  %47 = bitcast %u0Matrix* %4 to i8*
  call void @likely_release_mat(i8* %47) #2
  ret %f64Matrix* %dst

polly.merge:                                      ; preds = %y_exit16
  %48 = add nsw i64 %23, -1
  %polly.fdiv_q.shr40 = ashr i64 %48, 5
  %polly.loop_guard45 = icmp sgt i64 %polly.fdiv_q.shr40, -1
  br i1 %polly.loop_guard45, label %polly.loop_preheader43, label %y_exit34

polly.loop_header42:                              ; preds = %polly.loop_exit54, %polly.loop_preheader43
  %polly.indvar46 = phi i64 [ 0, %polly.loop_preheader43 ], [ %polly.indvar_next47, %polly.loop_exit54 ]
  %polly.loop_guard55 = icmp sgt i64 %polly.indvar46, %pexp.p_div_q50
  br i1 %polly.loop_guard55, label %polly.loop_exit54, label %polly.loop_header52.preheader

polly.loop_header52.preheader:                    ; preds = %polly.loop_header42
  %49 = shl nsw i64 %polly.indvar46, 5
  %50 = sub nsw i64 %23, %49
  %51 = add nsw i64 %50, -1
  %52 = icmp sgt i64 %51, 31
  %53 = select i1 %52, i64 31, i64 %51
  %polly.loop_guard74 = icmp sgt i64 %53, -1
  %polly.adjust_ub77 = add i64 %53, -1
  br label %polly.loop_header52

polly.loop_exit54:                                ; preds = %polly.loop_exit64, %polly.loop_header42
  %polly.indvar_next47 = add nuw nsw i64 %polly.indvar46, 1
  %polly.loop_cond49 = icmp slt i64 %polly.indvar46, %polly.fdiv_q.shr40
  br i1 %polly.loop_cond49, label %polly.loop_header42, label %y_exit34

polly.loop_preheader43:                           ; preds = %polly.merge
  %pexp.p_div_q50 = lshr i64 %48, 5
  %54 = add nsw i64 %21, -1
  %polly.fdiv_q.shr60 = ashr i64 %54, 5
  %polly.loop_guard65 = icmp sgt i64 %polly.fdiv_q.shr60, -1
  br label %polly.loop_header42

polly.loop_header52:                              ; preds = %polly.loop_header52.preheader, %polly.loop_exit64
  %polly.indvar56 = phi i64 [ %polly.indvar_next57, %polly.loop_exit64 ], [ %polly.indvar46, %polly.loop_header52.preheader ]
  br i1 %polly.loop_guard65, label %polly.loop_header62.preheader, label %polly.loop_exit64

polly.loop_header62.preheader:                    ; preds = %polly.loop_header52
  %55 = shl nsw i64 %polly.indvar56, 5
  %56 = sub nsw i64 %49, %55
  %57 = sub nsw i64 %23, %55
  %58 = add nsw i64 %57, -1
  %59 = icmp sgt i64 %58, 31
  %60 = select i1 %59, i64 31, i64 %58
  %polly.adjust_ub86 = add i64 %60, -1
  br label %polly.loop_header62

polly.loop_exit64:                                ; preds = %polly.loop_exit73, %polly.loop_header52
  %polly.indvar_next57 = add nuw nsw i64 %polly.indvar56, 1
  %polly.loop_cond59 = icmp slt i64 %polly.indvar56, %pexp.p_div_q50
  br i1 %polly.loop_cond59, label %polly.loop_header52, label %polly.loop_exit54

polly.loop_header62:                              ; preds = %polly.loop_header62.preheader, %polly.loop_exit73
  %polly.indvar66 = phi i64 [ %polly.indvar_next67, %polly.loop_exit73 ], [ 0, %polly.loop_header62.preheader ]
  br i1 %polly.loop_guard74, label %polly.loop_header71.preheader, label %polly.loop_exit73

polly.loop_header71.preheader:                    ; preds = %polly.loop_header62
  %61 = shl nsw i64 %polly.indvar66, 5
  %62 = sub nsw i64 %21, %61
  %63 = add nsw i64 %62, -1
  %64 = icmp sgt i64 %63, 31
  %65 = select i1 %64, i64 31, i64 %63
  %polly.loop_guard92 = icmp sgt i64 %65, -1
  %polly.adjust_ub95 = add i64 %65, -1
  br i1 %polly.loop_guard92, label %polly.loop_header71.us, label %polly.loop_exit73

polly.loop_header71.us:                           ; preds = %polly.loop_header71.preheader, %polly.loop_exit82.us
  %polly.indvar75.us = phi i64 [ %polly.indvar_next76.us, %polly.loop_exit82.us ], [ 0, %polly.loop_header71.preheader ]
  %66 = add nsw i64 %polly.indvar75.us, %56
  %67 = icmp slt i64 %66, 0
  %68 = select i1 %67, i64 0, i64 %66
  %polly.loop_guard83.us = icmp sgt i64 %68, %60
  br i1 %polly.loop_guard83.us, label %polly.loop_exit82.us, label %polly.loop_header80.us.us

polly.loop_exit82.us:                             ; preds = %polly.loop_exit91.loopexit.us.us, %polly.loop_header71.us
  %polly.indvar_next76.us = add nuw nsw i64 %polly.indvar75.us, 1
  %polly.loop_cond78.us = icmp sgt i64 %polly.indvar75.us, %polly.adjust_ub77
  br i1 %polly.loop_cond78.us, label %polly.loop_exit73, label %polly.loop_header71.us

polly.loop_header80.us.us:                        ; preds = %polly.loop_header71.us, %polly.loop_exit91.loopexit.us.us
  %polly.indvar84.us.us = phi i64 [ %polly.indvar_next85.us.us, %polly.loop_exit91.loopexit.us.us ], [ %68, %polly.loop_header71.us ]
  br label %polly.loop_header89.us.us

polly.loop_exit91.loopexit.us.us:                 ; preds = %polly.loop_header89.us.us
  %polly.indvar_next85.us.us = add nuw nsw i64 %polly.indvar84.us.us, 1
  %polly.loop_cond87.us.us = icmp sgt i64 %polly.indvar84.us.us, %polly.adjust_ub86
  br i1 %polly.loop_cond87.us.us, label %polly.loop_exit82.us, label %polly.loop_header80.us.us

polly.loop_header89.us.us:                        ; preds = %polly.loop_header89.us.us, %polly.loop_header80.us.us
  %polly.indvar93.us.us = phi i64 [ %polly.indvar_next94.us.us, %polly.loop_header89.us.us ], [ 0, %polly.loop_header80.us.us ]
  %polly.indvar_next94.us.us = add nuw nsw i64 %polly.indvar93.us.us, 1
  %polly.loop_cond96.us.us = icmp sgt i64 %polly.indvar93.us.us, %polly.adjust_ub95
  br i1 %polly.loop_cond96.us.us, label %polly.loop_exit91.loopexit.us.us, label %polly.loop_header89.us.us

polly.loop_exit73:                                ; preds = %polly.loop_exit82.us, %polly.loop_header71.preheader, %polly.loop_header62
  %polly.indvar_next67 = add nuw nsw i64 %polly.indvar66, 1
  %polly.loop_cond69 = icmp slt i64 %polly.indvar66, %polly.fdiv_q.shr60
  br i1 %polly.loop_cond69, label %polly.loop_header62, label %polly.loop_exit64
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
