; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%u8Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define noalias %f32Matrix* @covariance(%u8Matrix* noalias nocapture readonly) #1 {
entry:
  %1 = getelementptr inbounds %u8Matrix, %u8Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0Matrix* @likely_new(i32 9504, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %u8Matrix, %u8Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0Matrix, %u0Matrix* %2, i64 1
  %6 = bitcast %u0Matrix* %5 to i8*
  %7 = bitcast %u0Matrix* %5 to float*
  %8 = shl nuw nsw i64 %4, 2
  call void @llvm.memset.p0i8.i64(i8* %6, i8 0, i64 %8, i32 4, i1 false)
  %9 = zext i32 %rows to i64
  br label %y_body

y_body:                                           ; preds = %x_exit8, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit8 ]
  %10 = mul nuw nsw i64 %y, %4
  br label %x_body7

x_body7:                                          ; preds = %y_body, %x_body7
  %x9 = phi i64 [ %x_increment10, %x_body7 ], [ 0, %y_body ]
  %11 = getelementptr float, float* %7, i64 %x9
  %12 = load float, float* %11, align 4
  %13 = add nuw nsw i64 %x9, %10
  %14 = getelementptr %u8Matrix, %u8Matrix* %0, i64 0, i32 6, i64 %13
  %15 = load i8, i8* %14, align 1
  %16 = uitofp i8 %15 to float
  %17 = fadd fast float %16, %12
  store float %17, float* %11, align 4
  %x_increment10 = add nuw nsw i64 %x9, 1
  %x_postcondition11 = icmp eq i64 %x_increment10, %4
  br i1 %x_postcondition11, label %x_exit8, label %x_body7

x_exit8:                                          ; preds = %x_body7
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %9
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit8
  %18 = icmp eq i32 %rows, 1
  br i1 %18, label %Flow2, label %true_entry

true_entry:                                       ; preds = %y_exit
  %19 = uitofp i32 %rows to float
  %20 = fdiv fast float 1.000000e+00, %19
  br label %x_body15

Flow2:                                            ; preds = %x_body15, %y_exit
  %21 = call %u0Matrix* @likely_new(i32 25888, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %22 = getelementptr inbounds %u0Matrix, %u0Matrix* %21, i64 1
  %23 = bitcast %u0Matrix* %22 to float*
  br label %y_body28

x_body15:                                         ; preds = %true_entry, %x_body15
  %x17 = phi i64 [ %x_increment18, %x_body15 ], [ 0, %true_entry ]
  %24 = getelementptr float, float* %7, i64 %x17
  %25 = load float, float* %24, align 4, !llvm.mem.parallel_loop_access !1
  %26 = fmul fast float %25, %20
  store float %26, float* %24, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment18 = add nuw nsw i64 %x17, 1
  %x_postcondition19 = icmp eq i64 %x_increment18, %4
  br i1 %x_postcondition19, label %Flow2, label %x_body15

y_body28:                                         ; preds = %x_exit32, %Flow2
  %y30 = phi i64 [ 0, %Flow2 ], [ %y_increment36, %x_exit32 ]
  %27 = mul nuw nsw i64 %y30, %4
  br label %x_body31

x_body31:                                         ; preds = %y_body28, %x_body31
  %x33 = phi i64 [ %x_increment34, %x_body31 ], [ 0, %y_body28 ]
  %28 = add nuw nsw i64 %x33, %27
  %29 = getelementptr %u8Matrix, %u8Matrix* %0, i64 0, i32 6, i64 %28
  %30 = load i8, i8* %29, align 1, !llvm.mem.parallel_loop_access !2
  %31 = getelementptr float, float* %23, i64 %28
  %32 = uitofp i8 %30 to float
  store float %32, float* %31, align 4, !llvm.mem.parallel_loop_access !2
  %x_increment34 = add nuw nsw i64 %x33, 1
  %x_postcondition35 = icmp eq i64 %x_increment34, %4
  br i1 %x_postcondition35, label %x_exit32, label %x_body31

x_exit32:                                         ; preds = %x_body31
  %y_increment36 = add nuw nsw i64 %y30, 1
  %y_postcondition37 = icmp eq i64 %y_increment36, %9
  br i1 %y_postcondition37, label %y_body47, label %y_body28

y_body47:                                         ; preds = %x_exit32, %x_exit51
  %y49 = phi i64 [ %y_increment55, %x_exit51 ], [ 0, %x_exit32 ]
  %33 = mul nuw nsw i64 %y49, %4
  br label %x_body50

x_body50:                                         ; preds = %y_body47, %x_body50
  %x52 = phi i64 [ %x_increment53, %x_body50 ], [ 0, %y_body47 ]
  %34 = add nuw nsw i64 %x52, %33
  %35 = getelementptr float, float* %23, i64 %34
  %36 = load float, float* %35, align 4, !llvm.mem.parallel_loop_access !3
  %37 = getelementptr float, float* %7, i64 %x52
  %38 = load float, float* %37, align 4, !llvm.mem.parallel_loop_access !3
  %39 = fsub fast float %36, %38
  store float %39, float* %35, align 4, !llvm.mem.parallel_loop_access !3
  %x_increment53 = add nuw nsw i64 %x52, 1
  %x_postcondition54 = icmp eq i64 %x_increment53, %4
  br i1 %x_postcondition54, label %x_exit51, label %x_body50

x_exit51:                                         ; preds = %x_body50
  %y_increment55 = add nuw nsw i64 %y49, 1
  %y_postcondition56 = icmp eq i64 %y_increment55, %9
  br i1 %y_postcondition56, label %y_exit48, label %y_body47

y_exit48:                                         ; preds = %x_exit51
  %40 = call %u0Matrix* @likely_new(i32 25888, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %41 = getelementptr inbounds %u0Matrix, %u0Matrix* %40, i64 1
  %42 = bitcast %u0Matrix* %41 to float*
  %43 = sext i32 %rows to i64
  %44 = icmp slt i32 %rows, 1
  %45 = sext i32 %columns to i64
  %46 = icmp sgt i32 %columns, -1
  %47 = and i1 %46, %44
  br i1 %47, label %polly.loop_exit, label %y_body68

y_body68:                                         ; preds = %y_exit48, %x_exit72
  %y70 = phi i64 [ %y_increment80, %x_exit72 ], [ 0, %y_exit48 ]
  %48 = mul nuw nsw i64 %y70, %4
  br label %x_body71

x_body71:                                         ; preds = %y_body68, %exit75
  %x73 = phi i64 [ %x_increment78, %exit75 ], [ 0, %y_body68 ]
  %49 = icmp ugt i64 %y70, %x73
  br i1 %49, label %exit75, label %true_entry76

true_entry76:                                     ; preds = %x_body71, %true_entry76
  %50 = phi i32 [ %64, %true_entry76 ], [ 0, %x_body71 ]
  %51 = phi double [ %63, %true_entry76 ], [ 0.000000e+00, %x_body71 ]
  %52 = zext i32 %50 to i64
  %53 = mul nuw nsw i64 %52, %4
  %54 = add nuw nsw i64 %53, %x73
  %55 = getelementptr float, float* %23, i64 %54
  %56 = load float, float* %55, align 4, !llvm.mem.parallel_loop_access !4
  %57 = fpext float %56 to double
  %58 = add nuw nsw i64 %53, %y70
  %59 = getelementptr float, float* %23, i64 %58
  %60 = load float, float* %59, align 4, !llvm.mem.parallel_loop_access !4
  %61 = fpext float %60 to double
  %62 = fmul fast double %61, %57
  %63 = fadd fast double %62, %51
  %64 = add nuw nsw i32 %50, 1
  %65 = icmp eq i32 %64, %rows
  br i1 %65, label %exit77, label %true_entry76

exit77:                                           ; preds = %true_entry76
  %66 = add nuw nsw i64 %x73, %48
  %67 = getelementptr float, float* %42, i64 %66
  %68 = fptrunc double %63 to float
  store float %68, float* %67, align 4, !llvm.mem.parallel_loop_access !4
  %69 = mul nuw nsw i64 %x73, %4
  %70 = add nuw nsw i64 %69, %y70
  %71 = getelementptr float, float* %42, i64 %70
  store float %68, float* %71, align 4, !llvm.mem.parallel_loop_access !4
  br label %exit75

exit75:                                           ; preds = %exit77, %x_body71
  %x_increment78 = add nuw nsw i64 %x73, 1
  %x_postcondition79 = icmp eq i64 %x_increment78, %4
  br i1 %x_postcondition79, label %x_exit72, label %x_body71

x_exit72:                                         ; preds = %exit75
  %y_increment80 = add nuw nsw i64 %y70, 1
  %y_postcondition81 = icmp eq i64 %y_increment80, %4
  br i1 %y_postcondition81, label %y_exit69, label %y_body68

y_exit69:                                         ; preds = %x_exit72, %polly.loop_exit93, %polly.loop_exit, %polly.cond
  %dst = bitcast %u0Matrix* %40 to %f32Matrix*
  %72 = bitcast %u0Matrix* %2 to i8*
  call void @likely_release_mat(i8* %72) #2
  %73 = bitcast %u0Matrix* %21 to i8*
  call void @likely_release_mat(i8* %73) #2
  ret %f32Matrix* %dst

polly.loop_exit:                                  ; preds = %y_exit48
  %74 = add nsw i64 %45, -1
  %polly.fdiv_q.shr = ashr i64 %74, 5
  %polly.loop_guard16 = icmp sgt i64 %polly.fdiv_q.shr, -1
  br i1 %polly.loop_guard16, label %polly.loop_preheader14, label %y_exit69

polly.cond:                                       ; preds = %polly.loop_exit24
  %.not = icmp slt i32 %rows, 1
  %polly.loop_guard16.not = xor i1 %polly.loop_guard16, true
  %brmerge = or i1 %.not, %polly.loop_guard16.not
  br i1 %brmerge, label %y_exit69, label %polly.loop_preheader82

polly.loop_header13:                              ; preds = %polly.loop_exit24, %polly.loop_preheader14
  %.s2a.1 = phi double [ undef, %polly.loop_preheader14 ], [ %.s2a.2, %polly.loop_exit24 ]
  %.phiops.3 = phi double [ 0.000000e+00, %polly.loop_preheader14 ], [ %.phiops.4, %polly.loop_exit24 ]
  %polly.indvar17 = phi i64 [ 0, %polly.loop_preheader14 ], [ %polly.indvar_next18, %polly.loop_exit24 ]
  %polly.loop_guard25 = icmp sgt i64 %polly.indvar17, %pexp.p_div_q
  br i1 %polly.loop_guard25, label %polly.loop_exit24, label %polly.loop_header22.preheader

polly.loop_header22.preheader:                    ; preds = %polly.loop_header13
  %75 = shl nsw i64 %polly.indvar17, 5
  %76 = sub nsw i64 %45, %75
  %77 = add nsw i64 %76, -1
  %78 = icmp sgt i64 %77, 31
  %79 = select i1 %78, i64 31, i64 %77
  %polly.loop_guard44 = icmp sgt i64 %79, -1
  %polly.adjust_ub47 = add i64 %79, -1
  br label %polly.loop_header22

polly.loop_exit24:                                ; preds = %polly.loop_exit34, %polly.loop_header13
  %.s2a.2 = phi double [ %.s2a.1, %polly.loop_header13 ], [ %.s2a.4, %polly.loop_exit34 ]
  %.phiops.4 = phi double [ %.phiops.3, %polly.loop_header13 ], [ %.phiops.6, %polly.loop_exit34 ]
  %polly.indvar_next18 = add nuw nsw i64 %polly.indvar17, 1
  %polly.loop_cond20 = icmp slt i64 %polly.indvar17, %polly.fdiv_q.shr
  br i1 %polly.loop_cond20, label %polly.loop_header13, label %polly.cond

polly.loop_preheader14:                           ; preds = %polly.loop_exit
  %scevgep67 = bitcast %u0Matrix* %22 to i8*
  %pexp.p_div_q = lshr i64 %74, 5
  %80 = add nsw i64 %43, -1
  %polly.fdiv_q.shr30 = ashr i64 %80, 5
  %polly.loop_guard35 = icmp sgt i64 %polly.fdiv_q.shr30, -1
  br label %polly.loop_header13

polly.loop_header22:                              ; preds = %polly.loop_header22.preheader, %polly.loop_exit34
  %.s2a.3 = phi double [ %.s2a.4, %polly.loop_exit34 ], [ %.s2a.1, %polly.loop_header22.preheader ]
  %.phiops.5 = phi double [ %.phiops.6, %polly.loop_exit34 ], [ %.phiops.3, %polly.loop_header22.preheader ]
  %polly.indvar26 = phi i64 [ %polly.indvar_next27, %polly.loop_exit34 ], [ %polly.indvar17, %polly.loop_header22.preheader ]
  br i1 %polly.loop_guard35, label %polly.loop_header32.preheader, label %polly.loop_exit34

polly.loop_header32.preheader:                    ; preds = %polly.loop_header22
  %81 = shl nsw i64 %polly.indvar26, 5
  %82 = sub nsw i64 %75, %81
  %83 = sub nsw i64 %45, %81
  %84 = add nsw i64 %83, -1
  %85 = icmp sgt i64 %84, 31
  %86 = select i1 %85, i64 31, i64 %84
  %polly.adjust_ub56 = add i64 %86, -1
  br label %polly.loop_header32

polly.loop_exit34:                                ; preds = %polly.loop_exit43, %polly.loop_header22
  %.s2a.4 = phi double [ %.s2a.3, %polly.loop_header22 ], [ %.s2a.6, %polly.loop_exit43 ]
  %.phiops.6 = phi double [ %.phiops.5, %polly.loop_header22 ], [ %.phiops.8, %polly.loop_exit43 ]
  %polly.indvar_next27 = add nuw nsw i64 %polly.indvar26, 1
  %polly.loop_cond29 = icmp slt i64 %polly.indvar26, %pexp.p_div_q
  br i1 %polly.loop_cond29, label %polly.loop_header22, label %polly.loop_exit24

polly.loop_header32:                              ; preds = %polly.loop_header32.preheader, %polly.loop_exit43
  %.s2a.5 = phi double [ %.s2a.6, %polly.loop_exit43 ], [ %.s2a.3, %polly.loop_header32.preheader ]
  %.phiops.7 = phi double [ %.phiops.8, %polly.loop_exit43 ], [ %.phiops.5, %polly.loop_header32.preheader ]
  %polly.indvar36 = phi i64 [ %polly.indvar_next37, %polly.loop_exit43 ], [ 0, %polly.loop_header32.preheader ]
  br i1 %polly.loop_guard44, label %polly.loop_header41.preheader, label %polly.loop_exit43

polly.loop_header41.preheader:                    ; preds = %polly.loop_header32
  %87 = shl nsw i64 %polly.indvar36, 5
  %88 = sub nsw i64 %43, %87
  %89 = add nsw i64 %88, -1
  %90 = icmp sgt i64 %89, 31
  %91 = select i1 %90, i64 31, i64 %89
  %polly.loop_guard62 = icmp sgt i64 %91, -1
  %polly.adjust_ub65 = add i64 %91, -1
  br i1 %polly.loop_guard62, label %polly.loop_header41.us, label %polly.loop_exit43

polly.loop_header41.us:                           ; preds = %polly.loop_header41.preheader, %polly.loop_exit52.us
  %.s2a.7.us = phi double [ %.s2a.8.us, %polly.loop_exit52.us ], [ %.s2a.5, %polly.loop_header41.preheader ]
  %.phiops.9.us = phi double [ %.phiops.10.us, %polly.loop_exit52.us ], [ %.phiops.7, %polly.loop_header41.preheader ]
  %polly.indvar45.us = phi i64 [ %polly.indvar_next46.us, %polly.loop_exit52.us ], [ 0, %polly.loop_header41.preheader ]
  %92 = add nsw i64 %polly.indvar45.us, %82
  %93 = icmp slt i64 %92, 0
  %94 = select i1 %93, i64 0, i64 %92
  %polly.loop_guard53.us = icmp sgt i64 %94, %86
  br i1 %polly.loop_guard53.us, label %polly.loop_exit52.us, label %polly.loop_header50.preheader.us

polly.loop_exit52.us:                             ; preds = %polly.loop_exit61.loopexit.us.us, %polly.loop_header41.us
  %.s2a.8.us = phi double [ %.s2a.7.us, %polly.loop_header41.us ], [ %p_77.us.us, %polly.loop_exit61.loopexit.us.us ]
  %.phiops.10.us = phi double [ %.phiops.9.us, %polly.loop_header41.us ], [ %p_77.us.us, %polly.loop_exit61.loopexit.us.us ]
  %polly.indvar_next46.us = add nuw nsw i64 %polly.indvar45.us, 1
  %polly.loop_cond48.us = icmp sgt i64 %polly.indvar45.us, %polly.adjust_ub47
  br i1 %polly.loop_cond48.us, label %polly.loop_exit43, label %polly.loop_header41.us

polly.loop_header50.preheader.us:                 ; preds = %polly.loop_header41.us
  %95 = add nuw nsw i64 %polly.indvar45.us, %75
  %96 = shl i64 %95, 2
  br label %polly.loop_header50.us.us

polly.loop_header50.us.us:                        ; preds = %polly.loop_header50.preheader.us, %polly.loop_exit61.loopexit.us.us
  %.phiops.11.us.us = phi double [ %p_77.us.us, %polly.loop_exit61.loopexit.us.us ], [ %.phiops.9.us, %polly.loop_header50.preheader.us ]
  %polly.indvar54.us.us = phi i64 [ %polly.indvar_next55.us.us, %polly.loop_exit61.loopexit.us.us ], [ %94, %polly.loop_header50.preheader.us ]
  %97 = add nsw i64 %polly.indvar54.us.us, %81
  %98 = shl i64 %97, 2
  br label %polly.loop_header59.us.us

polly.loop_exit61.loopexit.us.us:                 ; preds = %polly.loop_header59.us.us
  %polly.indvar_next55.us.us = add nuw nsw i64 %polly.indvar54.us.us, 1
  %polly.loop_cond57.us.us = icmp sgt i64 %polly.indvar54.us.us, %polly.adjust_ub56
  br i1 %polly.loop_cond57.us.us, label %polly.loop_exit52.us, label %polly.loop_header50.us.us

polly.loop_header59.us.us:                        ; preds = %polly.loop_header59.us.us, %polly.loop_header50.us.us
  %.phiops.13.us.us = phi double [ %p_77.us.us, %polly.loop_header59.us.us ], [ %.phiops.11.us.us, %polly.loop_header50.us.us ]
  %polly.indvar63.us.us = phi i64 [ %polly.indvar_next64.us.us, %polly.loop_header59.us.us ], [ 0, %polly.loop_header50.us.us ]
  %99 = add nuw nsw i64 %polly.indvar63.us.us, %87
  %100 = mul i64 %99, %8
  %101 = add i64 %100, %98
  %uglygep.us.us = getelementptr i8, i8* %scevgep67, i64 %101
  %uglygep68.us.us = bitcast i8* %uglygep.us.us to float*
  %_p_scalar_.us.us = load float, float* %uglygep68.us.us, align 4, !alias.scope !5, !noalias !7
  %p_69.us.us = fpext float %_p_scalar_.us.us to double
  %102 = add i64 %100, %96
  %uglygep72.us.us = getelementptr i8, i8* %scevgep67, i64 %102
  %uglygep7273.us.us = bitcast i8* %uglygep72.us.us to float*
  %_p_scalar_74.us.us = load float, float* %uglygep7273.us.us, align 4, !alias.scope !5, !noalias !7
  %p_75.us.us = fpext float %_p_scalar_74.us.us to double
  %p_76.us.us = fmul fast double %p_75.us.us, %p_69.us.us
  %p_77.us.us = fadd fast double %p_76.us.us, %.phiops.13.us.us
  %polly.indvar_next64.us.us = add nuw nsw i64 %polly.indvar63.us.us, 1
  %polly.loop_cond66.us.us = icmp sgt i64 %polly.indvar63.us.us, %polly.adjust_ub65
  br i1 %polly.loop_cond66.us.us, label %polly.loop_exit61.loopexit.us.us, label %polly.loop_header59.us.us

polly.loop_exit43:                                ; preds = %polly.loop_exit52.us, %polly.loop_header41.preheader, %polly.loop_header32
  %.s2a.6 = phi double [ %.s2a.5, %polly.loop_header32 ], [ %.s2a.5, %polly.loop_header41.preheader ], [ %.s2a.8.us, %polly.loop_exit52.us ]
  %.phiops.8 = phi double [ %.phiops.7, %polly.loop_header32 ], [ %.phiops.7, %polly.loop_header41.preheader ], [ %.phiops.10.us, %polly.loop_exit52.us ]
  %polly.indvar_next37 = add nuw nsw i64 %polly.indvar36, 1
  %polly.loop_cond39 = icmp slt i64 %polly.indvar36, %polly.fdiv_q.shr30
  br i1 %polly.loop_cond39, label %polly.loop_header32, label %polly.loop_exit34

polly.loop_header81:                              ; preds = %polly.loop_exit93, %polly.loop_preheader82
  %polly.indvar85 = phi i64 [ 0, %polly.loop_preheader82 ], [ %polly.indvar_next86, %polly.loop_exit93 ]
  %polly.loop_guard94 = icmp sgt i64 %polly.indvar85, %pexp.p_div_q89
  br i1 %polly.loop_guard94, label %polly.loop_exit93, label %polly.loop_header91.preheader

polly.loop_header91.preheader:                    ; preds = %polly.loop_header81
  %103 = shl nsw i64 %polly.indvar85, 5
  %104 = sub nsw i64 %45, %103
  %105 = add nsw i64 %104, -1
  %106 = icmp sgt i64 %105, 31
  %107 = select i1 %106, i64 31, i64 %105
  %polly.loop_guard103 = icmp sgt i64 %107, -1
  %polly.adjust_ub106 = add i64 %107, -1
  br i1 %polly.loop_guard103, label %polly.loop_header91.us, label %polly.loop_exit93

polly.loop_header91.us:                           ; preds = %polly.loop_header91.preheader, %polly.loop_exit102.loopexit.us
  %polly.indvar95.us = phi i64 [ %polly.indvar_next96.us, %polly.loop_exit102.loopexit.us ], [ %polly.indvar85, %polly.loop_header91.preheader ]
  %108 = shl nsw i64 %polly.indvar95.us, 5
  %109 = sub nsw i64 %103, %108
  %110 = sub nsw i64 %45, %108
  %111 = add nsw i64 %110, -1
  %112 = icmp sgt i64 %111, 31
  %113 = select i1 %112, i64 31, i64 %111
  %polly.adjust_ub115.us = add i64 %113, -1
  br label %polly.loop_header100.us

polly.loop_header100.us:                          ; preds = %polly.loop_header91.us, %polly.loop_exit111.us
  %polly.indvar104.us = phi i64 [ %polly.indvar_next105.us, %polly.loop_exit111.us ], [ 0, %polly.loop_header91.us ]
  %114 = add nsw i64 %polly.indvar104.us, %109
  %115 = icmp slt i64 %114, 0
  %116 = select i1 %115, i64 0, i64 %114
  %polly.loop_guard112.us = icmp sgt i64 %116, %113
  br i1 %polly.loop_guard112.us, label %polly.loop_exit111.us, label %polly.loop_header109.preheader.us

polly.loop_header109.us:                          ; preds = %polly.loop_header109.preheader.us, %polly.loop_header109.us
  %polly.indvar113.us = phi i64 [ %polly.indvar_next114.us, %polly.loop_header109.us ], [ %116, %polly.loop_header109.preheader.us ]
  %117 = add nsw i64 %polly.indvar113.us, %108
  %118 = shl i64 %117, 2
  %119 = add i64 %118, %123
  %uglygep120.us = getelementptr i8, i8* %scevgep118119, i64 %119
  %uglygep120121.us = bitcast i8* %uglygep120.us to float*
  store float %p_117, float* %uglygep120121.us, align 4, !alias.scope !8, !noalias !11
  %120 = mul i64 %117, %8
  %121 = add i64 %120, %124
  %uglygep124.us = getelementptr i8, i8* %scevgep118119, i64 %121
  %uglygep124125.us = bitcast i8* %uglygep124.us to float*
  store float %p_117, float* %uglygep124125.us, align 4, !alias.scope !8, !noalias !11
  %polly.indvar_next114.us = add nuw nsw i64 %polly.indvar113.us, 1
  %polly.loop_cond116.us = icmp sgt i64 %polly.indvar113.us, %polly.adjust_ub115.us
  br i1 %polly.loop_cond116.us, label %polly.loop_exit111.us, label %polly.loop_header109.us

polly.loop_exit111.us:                            ; preds = %polly.loop_header109.us, %polly.loop_header100.us
  %polly.indvar_next105.us = add nuw nsw i64 %polly.indvar104.us, 1
  %polly.loop_cond107.us = icmp sgt i64 %polly.indvar104.us, %polly.adjust_ub106
  br i1 %polly.loop_cond107.us, label %polly.loop_exit102.loopexit.us, label %polly.loop_header100.us

polly.loop_header109.preheader.us:                ; preds = %polly.loop_header100.us
  %122 = add nuw nsw i64 %polly.indvar104.us, %103
  %123 = mul i64 %122, %8
  %124 = shl i64 %122, 2
  br label %polly.loop_header109.us

polly.loop_exit102.loopexit.us:                   ; preds = %polly.loop_exit111.us
  %polly.indvar_next96.us = add nuw nsw i64 %polly.indvar95.us, 1
  %polly.loop_cond98.us = icmp slt i64 %polly.indvar95.us, %pexp.p_div_q89
  br i1 %polly.loop_cond98.us, label %polly.loop_header91.us, label %polly.loop_exit93

polly.loop_exit93:                                ; preds = %polly.loop_exit102.loopexit.us, %polly.loop_header91.preheader, %polly.loop_header81
  %polly.indvar_next86 = add nuw nsw i64 %polly.indvar85, 1
  %polly.loop_cond88 = icmp slt i64 %polly.indvar85, %polly.fdiv_q.shr
  br i1 %polly.loop_cond88, label %polly.loop_header81, label %y_exit69

polly.loop_preheader82:                           ; preds = %polly.cond
  %scevgep118119 = bitcast %u0Matrix* %41 to i8*
  %pexp.p_div_q89 = lshr i64 %74, 5
  %p_117 = fptrunc double %.s2a.2 to float
  br label %polly.loop_header81
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #0

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind "polly-optimized" }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
!4 = distinct !{!4}
!5 = distinct !{!5, !6, !"polly.alias.scope."}
!6 = distinct !{!6, !"polly.alias.scope.domain"}
!7 = !{!8, !9, !10}
!8 = distinct !{!8, !6, !"polly.alias.scope."}
!9 = distinct !{!9, !6, !"polly.alias.scope."}
!10 = distinct !{!10, !6, !"polly.alias.scope."}
!11 = !{!5, !9, !10}
