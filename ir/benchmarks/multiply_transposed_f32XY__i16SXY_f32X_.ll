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
  %24 = sext i32 %rows to i64
  %25 = icmp slt i32 %rows, 1
  %26 = sext i32 %columns to i64
  %27 = icmp sgt i32 %columns, -1
  %28 = and i1 %27, %25
  br i1 %28, label %polly.merge, label %y_body33

y_body33:                                         ; preds = %y_exit16, %x_exit37
  %y35 = phi i64 [ %y_increment43, %x_exit37 ], [ 0, %y_exit16 ]
  %29 = mul nuw nsw i64 %y35, %mat_y_step
  br label %x_body36

x_body36:                                         ; preds = %y_body33, %exit
  %x38 = phi i64 [ %x_increment41, %exit ], [ 0, %y_body33 ]
  %30 = icmp ugt i64 %y35, %x38
  br i1 %30, label %exit, label %true_entry39

true_entry39:                                     ; preds = %x_body36, %true_entry39
  %31 = phi i32 [ %45, %true_entry39 ], [ 0, %x_body36 ]
  %32 = phi double [ %44, %true_entry39 ], [ 0.000000e+00, %x_body36 ]
  %33 = zext i32 %31 to i64
  %34 = mul nuw nsw i64 %33, %mat_y_step
  %35 = add nuw nsw i64 %34, %x38
  %36 = getelementptr float, float* %7, i64 %35
  %37 = load float, float* %36, align 4, !llvm.mem.parallel_loop_access !3
  %38 = fpext float %37 to double
  %39 = add nuw nsw i64 %34, %y35
  %40 = getelementptr float, float* %7, i64 %39
  %41 = load float, float* %40, align 4, !llvm.mem.parallel_loop_access !3
  %42 = fpext float %41 to double
  %43 = fmul fast double %42, %38
  %44 = fadd fast double %43, %32
  %45 = add nuw nsw i32 %31, 1
  %46 = icmp eq i32 %45, %rows
  br i1 %46, label %exit40, label %true_entry39

exit40:                                           ; preds = %true_entry39
  %47 = add nuw nsw i64 %x38, %29
  %48 = getelementptr float, float* %23, i64 %47
  %49 = fptrunc double %44 to float
  store float %49, float* %48, align 4, !llvm.mem.parallel_loop_access !3
  %50 = mul nuw nsw i64 %x38, %mat_y_step
  %51 = add nuw nsw i64 %50, %y35
  %52 = getelementptr float, float* %23, i64 %51
  store float %49, float* %52, align 4, !llvm.mem.parallel_loop_access !3
  br label %exit

exit:                                             ; preds = %exit40, %x_body36
  %x_increment41 = add nuw nsw i64 %x38, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %mat_y_step
  br i1 %x_postcondition42, label %x_exit37, label %x_body36

x_exit37:                                         ; preds = %exit
  %y_increment43 = add nuw nsw i64 %y35, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %mat_y_step
  br i1 %y_postcondition44, label %y_exit34, label %y_body33

y_exit34:                                         ; preds = %x_exit37, %polly.loop_exit48, %polly.merge
  %dst = bitcast %u0Matrix* %21 to %f32Matrix*
  %53 = bitcast %u0Matrix* %4 to i8*
  call void @likely_release_mat(i8* %53) #2
  ret %f32Matrix* %dst

polly.merge:                                      ; preds = %y_exit16
  %54 = add nsw i64 %26, -1
  %polly.fdiv_q.shr34 = ashr i64 %54, 5
  %polly.loop_guard39 = icmp sgt i64 %polly.fdiv_q.shr34, -1
  br i1 %polly.loop_guard39, label %polly.loop_preheader37, label %y_exit34

polly.loop_header36:                              ; preds = %polly.loop_exit48, %polly.loop_preheader37
  %polly.indvar40 = phi i64 [ 0, %polly.loop_preheader37 ], [ %polly.indvar_next41, %polly.loop_exit48 ]
  %polly.loop_guard49 = icmp sgt i64 %polly.indvar40, %pexp.p_div_q44
  br i1 %polly.loop_guard49, label %polly.loop_exit48, label %polly.loop_header46.preheader

polly.loop_header46.preheader:                    ; preds = %polly.loop_header36
  %55 = shl nsw i64 %polly.indvar40, 5
  %56 = sub nsw i64 %26, %55
  %57 = add nsw i64 %56, -1
  %58 = icmp sgt i64 %57, 31
  %59 = select i1 %58, i64 31, i64 %57
  %polly.loop_guard68 = icmp sgt i64 %59, -1
  %polly.adjust_ub71 = add i64 %59, -1
  br label %polly.loop_header46

polly.loop_exit48:                                ; preds = %polly.loop_exit58, %polly.loop_header36
  %polly.indvar_next41 = add nuw nsw i64 %polly.indvar40, 1
  %polly.loop_cond43 = icmp slt i64 %polly.indvar40, %polly.fdiv_q.shr34
  br i1 %polly.loop_cond43, label %polly.loop_header36, label %y_exit34

polly.loop_preheader37:                           ; preds = %polly.merge
  %pexp.p_div_q44 = lshr i64 %54, 5
  %60 = add nsw i64 %24, -1
  %polly.fdiv_q.shr54 = ashr i64 %60, 5
  %polly.loop_guard59 = icmp sgt i64 %polly.fdiv_q.shr54, -1
  br label %polly.loop_header36

polly.loop_header46:                              ; preds = %polly.loop_header46.preheader, %polly.loop_exit58
  %polly.indvar50 = phi i64 [ %polly.indvar_next51, %polly.loop_exit58 ], [ %polly.indvar40, %polly.loop_header46.preheader ]
  br i1 %polly.loop_guard59, label %polly.loop_header56.preheader, label %polly.loop_exit58

polly.loop_header56.preheader:                    ; preds = %polly.loop_header46
  %61 = shl nsw i64 %polly.indvar50, 5
  %62 = sub nsw i64 %55, %61
  %63 = sub nsw i64 %26, %61
  %64 = add nsw i64 %63, -1
  %65 = icmp sgt i64 %64, 31
  %66 = select i1 %65, i64 31, i64 %64
  %polly.adjust_ub80 = add i64 %66, -1
  br label %polly.loop_header56

polly.loop_exit58:                                ; preds = %polly.loop_exit67, %polly.loop_header46
  %polly.indvar_next51 = add nuw nsw i64 %polly.indvar50, 1
  %polly.loop_cond53 = icmp slt i64 %polly.indvar50, %pexp.p_div_q44
  br i1 %polly.loop_cond53, label %polly.loop_header46, label %polly.loop_exit48

polly.loop_header56:                              ; preds = %polly.loop_header56.preheader, %polly.loop_exit67
  %polly.indvar60 = phi i64 [ %polly.indvar_next61, %polly.loop_exit67 ], [ 0, %polly.loop_header56.preheader ]
  br i1 %polly.loop_guard68, label %polly.loop_header65.preheader, label %polly.loop_exit67

polly.loop_header65.preheader:                    ; preds = %polly.loop_header56
  %67 = shl nsw i64 %polly.indvar60, 5
  %68 = sub nsw i64 %24, %67
  %69 = add nsw i64 %68, -1
  %70 = icmp sgt i64 %69, 31
  %71 = select i1 %70, i64 31, i64 %69
  %polly.adjust_ub89 = add i64 %71, -1
  br label %polly.loop_header65

polly.loop_exit67:                                ; preds = %polly.loop_exit76, %polly.loop_header56
  %polly.indvar_next61 = add nuw nsw i64 %polly.indvar60, 1
  %polly.loop_cond63 = icmp slt i64 %polly.indvar60, %polly.fdiv_q.shr54
  br i1 %polly.loop_cond63, label %polly.loop_header56, label %polly.loop_exit58

polly.loop_header65:                              ; preds = %polly.loop_header65.preheader, %polly.loop_exit76
  %polly.indvar69 = phi i64 [ %polly.indvar_next70, %polly.loop_exit76 ], [ 0, %polly.loop_header65.preheader ]
  %72 = add nsw i64 %polly.indvar69, %62
  %73 = icmp slt i64 %72, 0
  %74 = select i1 %73, i64 0, i64 %72
  %polly.loop_guard77 = icmp sgt i64 %74, %66
  %polly.loop_guard86.not = icmp slt i64 %71, 0
  %brmerge = or i1 %polly.loop_guard77, %polly.loop_guard86.not
  br i1 %brmerge, label %polly.loop_exit76, label %polly.loop_header74.us

polly.loop_header74.us:                           ; preds = %polly.loop_header65, %polly.loop_exit85.loopexit.us
  %polly.indvar78.us = phi i64 [ %polly.indvar_next79.us, %polly.loop_exit85.loopexit.us ], [ %74, %polly.loop_header65 ]
  br label %polly.loop_header83.us

polly.loop_header83.us:                           ; preds = %polly.loop_header74.us, %polly.loop_header83.us
  %polly.indvar87.us = phi i64 [ %polly.indvar_next88.us, %polly.loop_header83.us ], [ 0, %polly.loop_header74.us ]
  %polly.indvar_next88.us = add nuw nsw i64 %polly.indvar87.us, 1
  %polly.loop_cond90.us = icmp sgt i64 %polly.indvar87.us, %polly.adjust_ub89
  br i1 %polly.loop_cond90.us, label %polly.loop_exit85.loopexit.us, label %polly.loop_header83.us

polly.loop_exit85.loopexit.us:                    ; preds = %polly.loop_header83.us
  %polly.indvar_next79.us = add nuw nsw i64 %polly.indvar78.us, 1
  %polly.loop_cond81.us = icmp sgt i64 %polly.indvar78.us, %polly.adjust_ub80
  br i1 %polly.loop_cond81.us, label %polly.loop_exit76, label %polly.loop_header74.us

polly.loop_exit76:                                ; preds = %polly.loop_exit85.loopexit.us, %polly.loop_header65
  %polly.indvar_next70 = add nuw nsw i64 %polly.indvar69, 1
  %polly.loop_cond72 = icmp sgt i64 %polly.indvar69, %polly.adjust_ub71
  br i1 %polly.loop_cond72, label %polly.loop_exit67, label %polly.loop_header65
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind "polly-optimized" }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
