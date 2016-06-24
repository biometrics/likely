; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
define noalias %f32Matrix* @gemm(%f32Matrix* noalias nocapture readonly, %f32Matrix* noalias nocapture readonly, double, %f32Matrix* noalias nocapture readonly, double) #2 {
entry:
  %5 = getelementptr inbounds %f32Matrix, %f32Matrix* %1, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %6, align 4, !range !0
  %7 = icmp eq i32 %rows, %columns
  call void @llvm.assume(i1 %7)
  %8 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 4
  %rows1 = load i32, i32* %8, align 4, !range !0
  %9 = getelementptr inbounds %f32Matrix, %f32Matrix* %3, i64 0, i32 4
  %rows2 = load i32, i32* %9, align 4, !range !0
  %10 = icmp eq i32 %rows1, %rows2
  call void @llvm.assume(i1 %10)
  %11 = getelementptr inbounds %f32Matrix, %f32Matrix* %1, i64 0, i32 3
  %columns3 = load i32, i32* %11, align 4, !range !0
  %12 = getelementptr inbounds %f32Matrix, %f32Matrix* %3, i64 0, i32 3
  %columns4 = load i32, i32* %12, align 4, !range !0
  %13 = icmp eq i32 %columns3, %columns4
  call void @llvm.assume(i1 %13)
  %14 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns4, i32 %rows2, i32 1, i8* null)
  %15 = zext i32 %rows2 to i64
  %dst_y_step = zext i32 %columns4 to i64
  %16 = getelementptr inbounds %u0Matrix, %u0Matrix* %14, i64 1
  %17 = bitcast %u0Matrix* %16 to float*
  %src1_y_step = zext i32 %columns to i64
  %18 = sext i32 %rows to i64
  %19 = icmp slt i32 %rows, 1
  %20 = sext i32 %columns4 to i64
  %21 = or i32 %columns4, %columns
  %22 = sext i32 %rows2 to i64
  %23 = icmp sgt i32 %21, -1
  %24 = and i1 %19, %23
  br i1 %24, label %polly.loop_exit, label %y_body

y_body:                                           ; preds = %entry, %x_exit
  %y = phi i64 [ %y_increment, %x_exit ], [ 0, %entry ]
  %25 = mul nuw nsw i64 %y, %dst_y_step
  %26 = mul nuw nsw i64 %y, %src1_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %exit
  %x = phi i64 [ %x_increment, %exit ], [ 0, %y_body ]
  br label %true_entry

true_entry:                                       ; preds = %x_body, %true_entry
  %27 = phi i32 [ %39, %true_entry ], [ 0, %x_body ]
  %28 = phi float [ %38, %true_entry ], [ 0.000000e+00, %x_body ]
  %29 = zext i32 %27 to i64
  %30 = add nuw nsw i64 %29, %26
  %31 = getelementptr %f32Matrix, %f32Matrix* %0, i64 0, i32 6, i64 %30
  %32 = load float, float* %31, align 4, !llvm.mem.parallel_loop_access !1
  %33 = mul nuw nsw i64 %29, %dst_y_step
  %34 = add nuw nsw i64 %33, %x
  %35 = getelementptr %f32Matrix, %f32Matrix* %1, i64 0, i32 6, i64 %34
  %36 = load float, float* %35, align 4, !llvm.mem.parallel_loop_access !1
  %37 = fmul fast float %36, %32
  %38 = fadd fast float %37, %28
  %39 = add nuw nsw i32 %27, 1
  %40 = icmp eq i32 %39, %rows
  br i1 %40, label %exit, label %true_entry

exit:                                             ; preds = %true_entry
  %41 = add nuw nsw i64 %x, %25
  %42 = getelementptr float, float* %17, i64 %41
  %43 = fpext float %38 to double
  %44 = fmul fast double %43, %2
  %45 = fptrunc double %44 to float
  %46 = getelementptr %f32Matrix, %f32Matrix* %3, i64 0, i32 6, i64 %41
  %47 = load float, float* %46, align 4, !llvm.mem.parallel_loop_access !1
  %48 = fpext float %47 to double
  %49 = fmul fast double %48, %4
  %50 = fptrunc double %49 to float
  %51 = fadd fast float %50, %45
  store float %51, float* %42, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %15
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit, %polly.loop_exit90, %polly.loop_exit, %polly.cond
  %dst = bitcast %u0Matrix* %14 to %f32Matrix*
  ret %f32Matrix* %dst

polly.loop_exit:                                  ; preds = %entry
  %52 = add nsw i64 %22, -1
  %polly.fdiv_q.shr = ashr i64 %52, 5
  %polly.loop_guard15 = icmp sgt i64 %polly.fdiv_q.shr, -1
  br i1 %polly.loop_guard15, label %polly.loop_preheader13, label %y_exit

polly.cond:                                       ; preds = %polly.loop_exit24
  %.not = icmp slt i32 %rows, 1
  %polly.loop_guard15.not = xor i1 %polly.loop_guard15, true
  %brmerge = or i1 %.not, %polly.loop_guard15.not
  br i1 %brmerge, label %y_exit, label %polly.loop_preheader79

polly.loop_header12:                              ; preds = %polly.loop_exit24, %polly.loop_preheader13
  %.s2a.1 = phi float [ undef, %polly.loop_preheader13 ], [ %.s2a.2, %polly.loop_exit24 ]
  %.phiops.3 = phi float [ 0.000000e+00, %polly.loop_preheader13 ], [ %.phiops.4, %polly.loop_exit24 ]
  %polly.indvar16 = phi i64 [ 0, %polly.loop_preheader13 ], [ %polly.indvar_next17, %polly.loop_exit24 ]
  br i1 %polly.loop_guard25, label %polly.loop_header22.preheader, label %polly.loop_exit24

polly.loop_header22.preheader:                    ; preds = %polly.loop_header12
  %53 = shl nsw i64 %polly.indvar16, 5
  %54 = sub nsw i64 %22, %53
  %55 = add nsw i64 %54, -1
  %56 = icmp sgt i64 %55, 31
  %57 = select i1 %56, i64 31, i64 %55
  %polly.loop_guard44 = icmp sgt i64 %57, -1
  %polly.adjust_ub47 = add i64 %57, -1
  br label %polly.loop_header22

polly.loop_exit24:                                ; preds = %polly.loop_exit34, %polly.loop_header12
  %.s2a.2 = phi float [ %.s2a.1, %polly.loop_header12 ], [ %.s2a.4, %polly.loop_exit34 ]
  %.phiops.4 = phi float [ %.phiops.3, %polly.loop_header12 ], [ %.phiops.6, %polly.loop_exit34 ]
  %polly.indvar_next17 = add nuw nsw i64 %polly.indvar16, 1
  %polly.loop_cond19 = icmp slt i64 %polly.indvar16, %polly.fdiv_q.shr
  br i1 %polly.loop_cond19, label %polly.loop_header12, label %polly.cond

polly.loop_preheader13:                           ; preds = %polly.loop_exit
  %scevgep = getelementptr %f32Matrix, %f32Matrix* %0, i64 1
  %scevgep67 = bitcast %f32Matrix* %scevgep to i8*
  %scevgep69 = getelementptr %f32Matrix, %f32Matrix* %1, i64 1
  %scevgep6970 = bitcast %f32Matrix* %scevgep69 to i8*
  %58 = add nsw i64 %20, -1
  %polly.fdiv_q.shr20 = ashr i64 %58, 5
  %polly.loop_guard25 = icmp sgt i64 %polly.fdiv_q.shr20, -1
  %59 = add nsw i64 %18, -1
  %polly.fdiv_q.shr30 = ashr i64 %59, 5
  %polly.loop_guard35 = icmp sgt i64 %polly.fdiv_q.shr30, -1
  br label %polly.loop_header12

polly.loop_header22:                              ; preds = %polly.loop_header22.preheader, %polly.loop_exit34
  %.s2a.3 = phi float [ %.s2a.4, %polly.loop_exit34 ], [ %.s2a.1, %polly.loop_header22.preheader ]
  %.phiops.5 = phi float [ %.phiops.6, %polly.loop_exit34 ], [ %.phiops.3, %polly.loop_header22.preheader ]
  %polly.indvar26 = phi i64 [ %polly.indvar_next27, %polly.loop_exit34 ], [ 0, %polly.loop_header22.preheader ]
  br i1 %polly.loop_guard35, label %polly.loop_header32.preheader, label %polly.loop_exit34

polly.loop_header32.preheader:                    ; preds = %polly.loop_header22
  %60 = shl nsw i64 %polly.indvar26, 5
  %61 = sub nsw i64 %20, %60
  %62 = add nsw i64 %61, -1
  %63 = icmp sgt i64 %62, 31
  %64 = select i1 %63, i64 31, i64 %62
  %polly.loop_guard53 = icmp sgt i64 %64, -1
  %polly.adjust_ub56 = add i64 %64, -1
  br label %polly.loop_header32

polly.loop_exit34:                                ; preds = %polly.loop_exit43, %polly.loop_header22
  %.s2a.4 = phi float [ %.s2a.3, %polly.loop_header22 ], [ %.s2a.6, %polly.loop_exit43 ]
  %.phiops.6 = phi float [ %.phiops.5, %polly.loop_header22 ], [ %.phiops.8, %polly.loop_exit43 ]
  %polly.indvar_next27 = add nuw nsw i64 %polly.indvar26, 1
  %polly.loop_cond29 = icmp slt i64 %polly.indvar26, %polly.fdiv_q.shr20
  br i1 %polly.loop_cond29, label %polly.loop_header22, label %polly.loop_exit24

polly.loop_header32:                              ; preds = %polly.loop_header32.preheader, %polly.loop_exit43
  %.s2a.5 = phi float [ %.s2a.6, %polly.loop_exit43 ], [ %.s2a.3, %polly.loop_header32.preheader ]
  %.phiops.7 = phi float [ %.phiops.8, %polly.loop_exit43 ], [ %.phiops.5, %polly.loop_header32.preheader ]
  %polly.indvar36 = phi i64 [ %polly.indvar_next37, %polly.loop_exit43 ], [ 0, %polly.loop_header32.preheader ]
  br i1 %polly.loop_guard44, label %polly.loop_header41.preheader, label %polly.loop_exit43

polly.loop_header41.preheader:                    ; preds = %polly.loop_header32
  %65 = shl nsw i64 %polly.indvar36, 5
  %66 = sub nsw i64 %18, %65
  %67 = add nsw i64 %66, -1
  %68 = icmp sgt i64 %67, 31
  %69 = select i1 %68, i64 31, i64 %67
  %polly.loop_guard62 = icmp sgt i64 %69, -1
  %polly.adjust_ub65 = add i64 %69, -1
  br i1 %polly.loop_guard53, label %polly.loop_header41.us, label %polly.loop_exit43

polly.loop_header41.us:                           ; preds = %polly.loop_header41.preheader, %polly.loop_exit52.loopexit.us
  %.s2a.7.us = phi float [ %.s2a.10.lcssa.us, %polly.loop_exit52.loopexit.us ], [ %.s2a.5, %polly.loop_header41.preheader ]
  %.phiops.9.us = phi float [ %.phiops.12.lcssa.us, %polly.loop_exit52.loopexit.us ], [ %.phiops.7, %polly.loop_header41.preheader ]
  %polly.indvar45.us = phi i64 [ %polly.indvar_next46.us, %polly.loop_exit52.loopexit.us ], [ 0, %polly.loop_header41.preheader ]
  %70 = add nuw nsw i64 %polly.indvar45.us, %53
  %71 = mul i64 %70, %src1_y_step
  br i1 %polly.loop_guard62, label %polly.loop_header50.us.us, label %polly.loop_exit52.loopexit.us

polly.loop_exit52.loopexit.us:                    ; preds = %polly.loop_exit61.loopexit.us.us, %polly.loop_header41.us
  %.phiops.12.lcssa.us = phi float [ %.phiops.9.us, %polly.loop_header41.us ], [ %p_74.us.us, %polly.loop_exit61.loopexit.us.us ]
  %.s2a.10.lcssa.us = phi float [ %.s2a.7.us, %polly.loop_header41.us ], [ %p_74.us.us, %polly.loop_exit61.loopexit.us.us ]
  %polly.indvar_next46.us = add nuw nsw i64 %polly.indvar45.us, 1
  %polly.loop_cond48.us = icmp sgt i64 %polly.indvar45.us, %polly.adjust_ub47
  br i1 %polly.loop_cond48.us, label %polly.loop_exit43, label %polly.loop_header41.us

polly.loop_header50.us.us:                        ; preds = %polly.loop_header41.us, %polly.loop_exit61.loopexit.us.us
  %.phiops.11.us.us = phi float [ %p_74.us.us, %polly.loop_exit61.loopexit.us.us ], [ %.phiops.9.us, %polly.loop_header41.us ]
  %polly.indvar54.us.us = phi i64 [ %polly.indvar_next55.us.us, %polly.loop_exit61.loopexit.us.us ], [ 0, %polly.loop_header41.us ]
  %72 = add nuw nsw i64 %polly.indvar54.us.us, %60
  br label %polly.loop_header59.us.us

polly.loop_exit61.loopexit.us.us:                 ; preds = %polly.loop_header59.us.us
  %polly.indvar_next55.us.us = add nuw nsw i64 %polly.indvar54.us.us, 1
  %polly.loop_cond57.us.us = icmp sgt i64 %polly.indvar54.us.us, %polly.adjust_ub56
  br i1 %polly.loop_cond57.us.us, label %polly.loop_exit52.loopexit.us, label %polly.loop_header50.us.us

polly.loop_header59.us.us:                        ; preds = %polly.loop_header59.us.us, %polly.loop_header50.us.us
  %.phiops.13.us.us = phi float [ %p_74.us.us, %polly.loop_header59.us.us ], [ %.phiops.11.us.us, %polly.loop_header50.us.us ]
  %polly.indvar63.us.us = phi i64 [ %polly.indvar_next64.us.us, %polly.loop_header59.us.us ], [ 0, %polly.loop_header50.us.us ]
  %73 = add nuw nsw i64 %polly.indvar63.us.us, %65
  %tmp.us.us = add nuw i64 %73, %71
  %tmp132.us.us = shl i64 %tmp.us.us, 2
  %uglygep.us.us = getelementptr i8, i8* %scevgep67, i64 %tmp132.us.us
  %uglygep68.us.us = bitcast i8* %uglygep.us.us to float*
  %_p_scalar_.us.us = load float, float* %uglygep68.us.us, align 4, !alias.scope !2, !noalias !4
  %74 = mul i64 %73, %dst_y_step
  %tmp133.us.us = add nuw i64 %72, %74
  %tmp134.us.us = shl i64 %tmp133.us.us, 2
  %uglygep71.us.us = getelementptr i8, i8* %scevgep6970, i64 %tmp134.us.us
  %uglygep7172.us.us = bitcast i8* %uglygep71.us.us to float*
  %_p_scalar_73.us.us = load float, float* %uglygep7172.us.us, align 4, !alias.scope !9, !noalias !12
  %p_.us.us = fmul fast float %_p_scalar_73.us.us, %_p_scalar_.us.us
  %p_74.us.us = fadd fast float %p_.us.us, %.phiops.13.us.us
  %polly.indvar_next64.us.us = add nuw nsw i64 %polly.indvar63.us.us, 1
  %polly.loop_cond66.us.us = icmp sgt i64 %polly.indvar63.us.us, %polly.adjust_ub65
  br i1 %polly.loop_cond66.us.us, label %polly.loop_exit61.loopexit.us.us, label %polly.loop_header59.us.us

polly.loop_exit43:                                ; preds = %polly.loop_exit52.loopexit.us, %polly.loop_header41.preheader, %polly.loop_header32
  %.s2a.6 = phi float [ %.s2a.5, %polly.loop_header32 ], [ %.s2a.5, %polly.loop_header41.preheader ], [ %.s2a.10.lcssa.us, %polly.loop_exit52.loopexit.us ]
  %.phiops.8 = phi float [ %.phiops.7, %polly.loop_header32 ], [ %.phiops.7, %polly.loop_header41.preheader ], [ %.phiops.12.lcssa.us, %polly.loop_exit52.loopexit.us ]
  %polly.indvar_next37 = add nuw nsw i64 %polly.indvar36, 1
  %polly.loop_cond39 = icmp slt i64 %polly.indvar36, %polly.fdiv_q.shr30
  br i1 %polly.loop_cond39, label %polly.loop_header32, label %polly.loop_exit34

polly.loop_header78:                              ; preds = %polly.loop_exit90, %polly.loop_preheader79
  %polly.indvar82 = phi i64 [ 0, %polly.loop_preheader79 ], [ %polly.indvar_next83, %polly.loop_exit90 ]
  br i1 %polly.loop_guard91, label %polly.loop_header88.preheader, label %polly.loop_exit90

polly.loop_header88.preheader:                    ; preds = %polly.loop_header78
  %75 = shl nsw i64 %polly.indvar82, 5
  %76 = sub nsw i64 %22, %75
  %77 = add nsw i64 %76, -1
  %78 = icmp sgt i64 %77, 31
  %79 = select i1 %78, i64 31, i64 %77
  %polly.loop_guard100 = icmp sgt i64 %79, -1
  %polly.adjust_ub103 = add i64 %79, -1
  br i1 %polly.loop_guard100, label %polly.loop_header88.us, label %polly.loop_exit90

polly.loop_header88.us:                           ; preds = %polly.loop_header88.preheader, %polly.loop_exit99.loopexit.us
  %polly.indvar92.us = phi i64 [ %polly.indvar_next93.us, %polly.loop_exit99.loopexit.us ], [ 0, %polly.loop_header88.preheader ]
  %80 = shl nsw i64 %polly.indvar92.us, 5
  %81 = sub nsw i64 %20, %80
  %82 = add nsw i64 %81, -1
  %83 = icmp sgt i64 %82, 31
  %84 = select i1 %83, i64 31, i64 %82
  %polly.loop_guard109.us = icmp sgt i64 %84, -1
  %polly.adjust_ub112.us = add i64 %84, -1
  br i1 %polly.loop_guard109.us, label %polly.loop_header97.us.us, label %polly.loop_exit99.loopexit.us

polly.loop_exit99.loopexit.us:                    ; preds = %polly.loop_exit108.loopexit.us.us, %polly.loop_header88.us
  %polly.indvar_next93.us = add nuw nsw i64 %polly.indvar92.us, 1
  %polly.loop_cond95.us = icmp slt i64 %polly.indvar92.us, %polly.fdiv_q.shr86
  br i1 %polly.loop_cond95.us, label %polly.loop_header88.us, label %polly.loop_exit90

polly.loop_header97.us.us:                        ; preds = %polly.loop_header88.us, %polly.loop_exit108.loopexit.us.us
  %polly.indvar101.us.us = phi i64 [ %polly.indvar_next102.us.us, %polly.loop_exit108.loopexit.us.us ], [ 0, %polly.loop_header88.us ]
  %85 = add nuw nsw i64 %polly.indvar101.us.us, %75
  %86 = mul i64 %90, %85
  br label %polly.loop_header106.us.us

polly.loop_exit108.loopexit.us.us:                ; preds = %polly.loop_header106.us.us
  %polly.indvar_next102.us.us = add nuw nsw i64 %polly.indvar101.us.us, 1
  %polly.loop_cond104.us.us = icmp sgt i64 %polly.indvar101.us.us, %polly.adjust_ub103
  br i1 %polly.loop_cond104.us.us, label %polly.loop_exit99.loopexit.us, label %polly.loop_header97.us.us

polly.loop_header106.us.us:                       ; preds = %polly.loop_header106.us.us, %polly.loop_header97.us.us
  %polly.indvar110.us.us = phi i64 [ %polly.indvar_next111.us.us, %polly.loop_header106.us.us ], [ 0, %polly.loop_header97.us.us ]
  %87 = add nuw nsw i64 %polly.indvar110.us.us, %80
  %88 = shl i64 %87, 2
  %89 = add i64 %88, %86
  %uglygep121.us.us = getelementptr i8, i8* %scevgep119120, i64 %89
  %uglygep121122.us.us = bitcast i8* %uglygep121.us.us to float*
  %_p_scalar_123.us.us = load float, float* %uglygep121122.us.us, align 4, !alias.scope !7, !noalias !13
  %p_124.us.us = fpext float %_p_scalar_123.us.us to double
  %p_125.us.us = fmul fast double %p_124.us.us, %4
  %p_126.us.us = fptrunc double %p_125.us.us to float
  %p_127.us.us = fadd fast float %p_126.us.us, %p_118
  %uglygep130.us.us = getelementptr i8, i8* %scevgep128129, i64 %89
  %uglygep130131.us.us = bitcast i8* %uglygep130.us.us to float*
  store float %p_127.us.us, float* %uglygep130131.us.us, align 4, !alias.scope !11, !noalias !14
  %polly.indvar_next111.us.us = add nuw nsw i64 %polly.indvar110.us.us, 1
  %polly.loop_cond113.us.us = icmp sgt i64 %polly.indvar110.us.us, %polly.adjust_ub112.us
  br i1 %polly.loop_cond113.us.us, label %polly.loop_exit108.loopexit.us.us, label %polly.loop_header106.us.us

polly.loop_exit90:                                ; preds = %polly.loop_exit99.loopexit.us, %polly.loop_header88.preheader, %polly.loop_header78
  %polly.indvar_next83 = add nuw nsw i64 %polly.indvar82, 1
  %polly.loop_cond85 = icmp slt i64 %polly.indvar82, %polly.fdiv_q.shr
  br i1 %polly.loop_cond85, label %polly.loop_header78, label %y_exit

polly.loop_preheader79:                           ; preds = %polly.cond
  %scevgep119 = getelementptr %f32Matrix, %f32Matrix* %3, i64 1
  %scevgep119120 = bitcast %f32Matrix* %scevgep119 to i8*
  %90 = shl nuw nsw i64 %dst_y_step, 2
  %scevgep128129 = bitcast %u0Matrix* %16 to i8*
  %91 = add nsw i64 %20, -1
  %polly.fdiv_q.shr86 = ashr i64 %91, 5
  %polly.loop_guard91 = icmp sgt i64 %polly.fdiv_q.shr86, -1
  %p_116 = fpext float %.s2a.2 to double
  %p_117 = fmul fast double %p_116, %2
  %p_118 = fptrunc double %p_117 to float
  br label %polly.loop_header78
}

attributes #0 = { nounwind }
attributes #1 = { argmemonly nounwind }
attributes #2 = { nounwind "polly-optimized" }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2, !3, !"polly.alias.scope."}
!3 = distinct !{!3, !"polly.alias.scope.domain"}
!4 = !{!5, !6, !7, !8, !9, !10, !11}
!5 = distinct !{!5, !3, !"polly.alias.scope."}
!6 = distinct !{!6, !3, !"polly.alias.scope."}
!7 = distinct !{!7, !3, !"polly.alias.scope."}
!8 = distinct !{!8, !3, !"polly.alias.scope."}
!9 = distinct !{!9, !3, !"polly.alias.scope."}
!10 = distinct !{!10, !3, !"polly.alias.scope."}
!11 = distinct !{!11, !3, !"polly.alias.scope."}
!12 = !{!5, !2, !6, !7, !8, !10, !11}
!13 = !{!5, !2, !6, !8, !9, !10, !11}
!14 = !{!5, !2, !6, !7, !8, !9, !10}
