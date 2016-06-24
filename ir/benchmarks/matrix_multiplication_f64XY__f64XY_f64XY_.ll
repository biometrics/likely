; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64Matrix = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
define noalias %f64Matrix* @matrix_multiplication(%f64Matrix* noalias nocapture readonly, %f64Matrix* noalias nocapture readonly) #2 {
entry:
  %2 = getelementptr inbounds %f64Matrix, %f64Matrix* %1, i64 0, i32 4
  %rows = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %3, align 4, !range !0
  %4 = icmp eq i32 %rows, %columns
  call void @llvm.assume(i1 %4)
  %5 = getelementptr inbounds %f64Matrix, %f64Matrix* %1, i64 0, i32 3
  %columns1 = load i32, i32* %5, align 4, !range !0
  %6 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 4
  %rows2 = load i32, i32* %6, align 4, !range !0
  %7 = call %u0Matrix* @likely_new(i32 24896, i32 1, i32 %columns1, i32 %rows2, i32 1, i8* null)
  %8 = zext i32 %rows2 to i64
  %C_y_step = zext i32 %columns1 to i64
  %9 = getelementptr inbounds %u0Matrix, %u0Matrix* %7, i64 1
  %10 = bitcast %u0Matrix* %9 to double*
  %A_y_step = zext i32 %columns to i64
  %11 = sext i32 %rows to i64
  %12 = icmp slt i32 %rows, 1
  %13 = sext i32 %columns1 to i64
  %14 = or i32 %columns1, %columns
  %15 = sext i32 %rows2 to i64
  %16 = icmp sgt i32 %14, -1
  %17 = and i1 %12, %16
  br i1 %17, label %polly.merge, label %y_body

y_body:                                           ; preds = %entry, %x_exit
  %y = phi i64 [ %y_increment, %x_exit ], [ 0, %entry ]
  %18 = mul nuw nsw i64 %y, %C_y_step
  %19 = mul nuw nsw i64 %y, %A_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %exit
  %x = phi i64 [ %x_increment, %exit ], [ 0, %y_body ]
  br label %true_entry

true_entry:                                       ; preds = %x_body, %true_entry
  %20 = phi i32 [ %32, %true_entry ], [ 0, %x_body ]
  %21 = phi double [ %31, %true_entry ], [ 0.000000e+00, %x_body ]
  %22 = zext i32 %20 to i64
  %23 = add nuw nsw i64 %22, %19
  %24 = getelementptr %f64Matrix, %f64Matrix* %0, i64 0, i32 6, i64 %23
  %25 = load double, double* %24, align 8, !llvm.mem.parallel_loop_access !1
  %26 = mul nuw nsw i64 %22, %C_y_step
  %27 = add nuw nsw i64 %26, %x
  %28 = getelementptr %f64Matrix, %f64Matrix* %1, i64 0, i32 6, i64 %27
  %29 = load double, double* %28, align 8, !llvm.mem.parallel_loop_access !1
  %30 = fmul fast double %29, %25
  %31 = fadd fast double %30, %21
  %32 = add nuw nsw i32 %20, 1
  %33 = icmp eq i32 %32, %rows
  br i1 %33, label %exit, label %true_entry

exit:                                             ; preds = %true_entry
  %34 = add nuw nsw i64 %x, %18
  %35 = getelementptr double, double* %10, i64 %34
  store double %31, double* %35, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %C_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %8
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit, %polly.loop_exit63, %polly.merge
  %C = bitcast %u0Matrix* %7 to %f64Matrix*
  ret %f64Matrix* %C

polly.merge:                                      ; preds = %entry
  %36 = add nsw i64 %15, -1
  %polly.fdiv_q.shr49 = ashr i64 %36, 5
  %polly.loop_guard54 = icmp sgt i64 %polly.fdiv_q.shr49, -1
  br i1 %polly.loop_guard54, label %polly.loop_preheader52, label %y_exit

polly.loop_header51:                              ; preds = %polly.loop_exit63, %polly.loop_preheader52
  %polly.indvar55 = phi i64 [ 0, %polly.loop_preheader52 ], [ %polly.indvar_next56, %polly.loop_exit63 ]
  br i1 %polly.loop_guard64, label %polly.loop_header61.preheader, label %polly.loop_exit63

polly.loop_header61.preheader:                    ; preds = %polly.loop_header51
  %37 = shl nsw i64 %polly.indvar55, 5
  %38 = sub nsw i64 %15, %37
  %39 = add nsw i64 %38, -1
  %40 = icmp sgt i64 %39, 31
  %41 = select i1 %40, i64 31, i64 %39
  %polly.loop_guard83 = icmp sgt i64 %41, -1
  %polly.adjust_ub86 = add i64 %41, -1
  br label %polly.loop_header61

polly.loop_exit63:                                ; preds = %polly.loop_exit73, %polly.loop_header51
  %polly.indvar_next56 = add nuw nsw i64 %polly.indvar55, 1
  %polly.loop_cond58 = icmp slt i64 %polly.indvar55, %polly.fdiv_q.shr49
  br i1 %polly.loop_cond58, label %polly.loop_header51, label %y_exit

polly.loop_preheader52:                           ; preds = %polly.merge
  %42 = add nsw i64 %13, -1
  %polly.fdiv_q.shr59 = ashr i64 %42, 5
  %polly.loop_guard64 = icmp sgt i64 %polly.fdiv_q.shr59, -1
  %43 = add nsw i64 %11, -1
  %polly.fdiv_q.shr69 = ashr i64 %43, 5
  %polly.loop_guard74 = icmp sgt i64 %polly.fdiv_q.shr69, -1
  br label %polly.loop_header51

polly.loop_header61:                              ; preds = %polly.loop_header61.preheader, %polly.loop_exit73
  %polly.indvar65 = phi i64 [ %polly.indvar_next66, %polly.loop_exit73 ], [ 0, %polly.loop_header61.preheader ]
  br i1 %polly.loop_guard74, label %polly.loop_header71.preheader, label %polly.loop_exit73

polly.loop_header71.preheader:                    ; preds = %polly.loop_header61
  %44 = shl nsw i64 %polly.indvar65, 5
  %45 = sub nsw i64 %13, %44
  %46 = add nsw i64 %45, -1
  %47 = icmp sgt i64 %46, 31
  %48 = select i1 %47, i64 31, i64 %46
  %polly.loop_guard92 = icmp sgt i64 %48, -1
  %polly.adjust_ub95 = add i64 %48, -1
  br label %polly.loop_header71

polly.loop_exit73:                                ; preds = %polly.loop_exit82, %polly.loop_header61
  %polly.indvar_next66 = add nuw nsw i64 %polly.indvar65, 1
  %polly.loop_cond68 = icmp slt i64 %polly.indvar65, %polly.fdiv_q.shr59
  br i1 %polly.loop_cond68, label %polly.loop_header61, label %polly.loop_exit63

polly.loop_header71:                              ; preds = %polly.loop_header71.preheader, %polly.loop_exit82
  %polly.indvar75 = phi i64 [ %polly.indvar_next76, %polly.loop_exit82 ], [ 0, %polly.loop_header71.preheader ]
  br i1 %polly.loop_guard83, label %polly.loop_header80.preheader, label %polly.loop_exit82

polly.loop_header80.preheader:                    ; preds = %polly.loop_header71
  %49 = shl nsw i64 %polly.indvar75, 5
  %50 = sub nsw i64 %11, %49
  %51 = add nsw i64 %50, -1
  %52 = icmp sgt i64 %51, 31
  %53 = select i1 %52, i64 31, i64 %51
  %polly.loop_guard101 = icmp sgt i64 %53, -1
  %polly.adjust_ub104 = add i64 %53, -1
  br i1 %polly.loop_guard92, label %polly.loop_header80.us, label %polly.loop_exit82

polly.loop_header80.us:                           ; preds = %polly.loop_header80.preheader, %polly.loop_exit91.loopexit.us
  %polly.indvar84.us = phi i64 [ %polly.indvar_next85.us, %polly.loop_exit91.loopexit.us ], [ 0, %polly.loop_header80.preheader ]
  br i1 %polly.loop_guard101, label %polly.loop_header89.us.us, label %polly.loop_exit91.loopexit.us

polly.loop_exit91.loopexit.us:                    ; preds = %polly.loop_exit100.loopexit.us.us, %polly.loop_header80.us
  %polly.indvar_next85.us = add nuw nsw i64 %polly.indvar84.us, 1
  %polly.loop_cond87.us = icmp sgt i64 %polly.indvar84.us, %polly.adjust_ub86
  br i1 %polly.loop_cond87.us, label %polly.loop_exit82, label %polly.loop_header80.us

polly.loop_header89.us.us:                        ; preds = %polly.loop_header80.us, %polly.loop_exit100.loopexit.us.us
  %polly.indvar93.us.us = phi i64 [ %polly.indvar_next94.us.us, %polly.loop_exit100.loopexit.us.us ], [ 0, %polly.loop_header80.us ]
  br label %polly.loop_header98.us.us

polly.loop_exit100.loopexit.us.us:                ; preds = %polly.loop_header98.us.us
  %polly.indvar_next94.us.us = add nuw nsw i64 %polly.indvar93.us.us, 1
  %polly.loop_cond96.us.us = icmp sgt i64 %polly.indvar93.us.us, %polly.adjust_ub95
  br i1 %polly.loop_cond96.us.us, label %polly.loop_exit91.loopexit.us, label %polly.loop_header89.us.us

polly.loop_header98.us.us:                        ; preds = %polly.loop_header98.us.us, %polly.loop_header89.us.us
  %polly.indvar102.us.us = phi i64 [ %polly.indvar_next103.us.us, %polly.loop_header98.us.us ], [ 0, %polly.loop_header89.us.us ]
  %polly.indvar_next103.us.us = add nuw nsw i64 %polly.indvar102.us.us, 1
  %polly.loop_cond105.us.us = icmp sgt i64 %polly.indvar102.us.us, %polly.adjust_ub104
  br i1 %polly.loop_cond105.us.us, label %polly.loop_exit100.loopexit.us.us, label %polly.loop_header98.us.us

polly.loop_exit82:                                ; preds = %polly.loop_exit91.loopexit.us, %polly.loop_header80.preheader, %polly.loop_header71
  %polly.indvar_next76 = add nuw nsw i64 %polly.indvar75, 1
  %polly.loop_cond78 = icmp slt i64 %polly.indvar75, %polly.fdiv_q.shr69
  br i1 %polly.loop_cond78, label %polly.loop_header71, label %polly.loop_exit73
}

attributes #0 = { nounwind }
attributes #1 = { argmemonly nounwind }
attributes #2 = { nounwind "polly-optimized" }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
