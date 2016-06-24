; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define noalias %f32Matrix* @covariance(%f32Matrix* noalias nocapture readonly) #1 {
entry:
  %1 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0Matrix* @likely_new(i32 8480, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0Matrix, %u0Matrix* %2, i64 1
  %6 = bitcast %u0Matrix* %5 to i8*
  %7 = bitcast %u0Matrix* %5 to float*
  %8 = shl nuw nsw i64 %4, 2
  call void @llvm.memset.p0i8.i64(i8* %6, i8 0, i64 %8, i32 4, i1 false)
  %9 = zext i32 %rows to i64
  %10 = sext i32 %rows to i64
  br label %y_body

y_body:                                           ; preds = %entry, %x_exit8
  %y = phi i64 [ %y_increment, %x_exit8 ], [ 0, %entry ]
  %11 = mul nuw nsw i64 %y, %4
  br label %x_body7

x_body7:                                          ; preds = %y_body, %x_body7
  %x9 = phi i64 [ %x_increment10, %x_body7 ], [ 0, %y_body ]
  %12 = getelementptr float, float* %7, i64 %x9
  %13 = load float, float* %12, align 4
  %14 = add nuw nsw i64 %x9, %11
  %15 = getelementptr %f32Matrix, %f32Matrix* %0, i64 0, i32 6, i64 %14
  %16 = load float, float* %15, align 4
  %17 = fadd fast float %16, %13
  store float %17, float* %12, align 4
  %x_increment10 = add nuw nsw i64 %x9, 1
  %x_postcondition11 = icmp eq i64 %x_increment10, %4
  br i1 %x_postcondition11, label %x_exit8, label %x_body7

x_exit8:                                          ; preds = %x_body7
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %9
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit8
  %18 = sext i32 %columns to i64
  %19 = icmp sgt i32 %columns, -1
  %20 = icmp eq i32 %rows, 1
  br i1 %20, label %Flow7, label %true_entry

true_entry:                                       ; preds = %y_exit
  %21 = uitofp i32 %rows to float
  %22 = fdiv fast float 1.000000e+00, %21
  br label %x_body15

Flow7:                                            ; preds = %x_body15, %y_exit
  %23 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %scevgep = getelementptr %u0Matrix, %u0Matrix* %23, i64 1, i32 0
  %scevgep3 = getelementptr %f32Matrix, %f32Matrix* %0, i64 1, i32 0
  br label %y_body28

x_body15:                                         ; preds = %true_entry, %x_body15
  %x17 = phi i64 [ %x_increment18, %x_body15 ], [ 0, %true_entry ]
  %24 = getelementptr float, float* %7, i64 %x17
  %25 = load float, float* %24, align 4, !llvm.mem.parallel_loop_access !1
  %26 = fmul fast float %25, %22
  store float %26, float* %24, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment18 = add nuw nsw i64 %x17, 1
  %x_postcondition19 = icmp eq i64 %x_increment18, %4
  br i1 %x_postcondition19, label %Flow7, label %x_body15

y_body28:                                         ; preds = %y_body28, %Flow7
  %y30 = phi i64 [ 0, %Flow7 ], [ %y_increment36, %y_body28 ]
  %27 = mul i64 %y30, %4
  %scevgep1 = getelementptr i32, i32* %scevgep, i64 %27
  %scevgep12 = bitcast i32* %scevgep1 to i8*
  %scevgep4 = getelementptr i32, i32* %scevgep3, i64 %27
  %scevgep45 = bitcast i32* %scevgep4 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep12, i8* %scevgep45, i64 %8, i32 4, i1 false)
  %y_increment36 = add nuw nsw i64 %y30, 1
  %y_postcondition37 = icmp eq i64 %y_increment36, %9
  br i1 %y_postcondition37, label %y_body47.preheader, label %y_body28

y_body47.preheader:                               ; preds = %y_body28
  %28 = getelementptr inbounds %u0Matrix, %u0Matrix* %23, i64 1
  %29 = bitcast %u0Matrix* %28 to float*
  br label %y_body47

y_body47:                                         ; preds = %x_exit51, %y_body47.preheader
  %y49 = phi i64 [ 0, %y_body47.preheader ], [ %y_increment55, %x_exit51 ]
  %30 = mul nuw nsw i64 %y49, %4
  br label %x_body50

x_body50:                                         ; preds = %y_body47, %x_body50
  %x52 = phi i64 [ %x_increment53, %x_body50 ], [ 0, %y_body47 ]
  %31 = add nuw nsw i64 %x52, %30
  %32 = getelementptr float, float* %29, i64 %31
  %33 = load float, float* %32, align 4, !llvm.mem.parallel_loop_access !2
  %34 = getelementptr float, float* %7, i64 %x52
  %35 = load float, float* %34, align 4, !llvm.mem.parallel_loop_access !2
  %36 = fsub fast float %33, %35
  store float %36, float* %32, align 4, !llvm.mem.parallel_loop_access !2
  %x_increment53 = add nuw nsw i64 %x52, 1
  %x_postcondition54 = icmp eq i64 %x_increment53, %4
  br i1 %x_postcondition54, label %x_exit51, label %x_body50

x_exit51:                                         ; preds = %x_body50
  %y_increment55 = add nuw nsw i64 %y49, 1
  %y_postcondition56 = icmp eq i64 %y_increment55, %9
  br i1 %y_postcondition56, label %y_exit48, label %y_body47

y_exit48:                                         ; preds = %x_exit51
  %37 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %38 = getelementptr inbounds %u0Matrix, %u0Matrix* %37, i64 1
  %39 = bitcast %u0Matrix* %38 to float*
  %40 = icmp slt i32 %rows, 1
  %41 = and i1 %19, %40
  br i1 %41, label %polly.loop_exit72, label %y_body68

y_body68:                                         ; preds = %y_exit48, %x_exit72
  %y70 = phi i64 [ %y_increment80, %x_exit72 ], [ 0, %y_exit48 ]
  %42 = mul nuw nsw i64 %y70, %4
  br label %x_body71

x_body71:                                         ; preds = %y_body68, %exit75
  %x73 = phi i64 [ %x_increment78, %exit75 ], [ 0, %y_body68 ]
  %43 = icmp ugt i64 %y70, %x73
  br i1 %43, label %exit75, label %true_entry76

true_entry76:                                     ; preds = %x_body71, %true_entry76
  %44 = phi i32 [ %58, %true_entry76 ], [ 0, %x_body71 ]
  %45 = phi double [ %57, %true_entry76 ], [ 0.000000e+00, %x_body71 ]
  %46 = zext i32 %44 to i64
  %47 = mul nuw nsw i64 %46, %4
  %48 = add nuw nsw i64 %47, %x73
  %49 = getelementptr float, float* %29, i64 %48
  %50 = load float, float* %49, align 4, !llvm.mem.parallel_loop_access !3
  %51 = fpext float %50 to double
  %52 = add nuw nsw i64 %47, %y70
  %53 = getelementptr float, float* %29, i64 %52
  %54 = load float, float* %53, align 4, !llvm.mem.parallel_loop_access !3
  %55 = fpext float %54 to double
  %56 = fmul fast double %55, %51
  %57 = fadd fast double %56, %45
  %58 = add nuw nsw i32 %44, 1
  %59 = icmp eq i32 %58, %rows
  br i1 %59, label %exit77, label %true_entry76

exit77:                                           ; preds = %true_entry76
  %60 = add nuw nsw i64 %x73, %42
  %61 = getelementptr float, float* %39, i64 %60
  %62 = fptrunc double %57 to float
  store float %62, float* %61, align 4, !llvm.mem.parallel_loop_access !3
  %63 = mul nuw nsw i64 %x73, %4
  %64 = add nuw nsw i64 %63, %y70
  %65 = getelementptr float, float* %39, i64 %64
  store float %62, float* %65, align 4, !llvm.mem.parallel_loop_access !3
  br label %exit75

exit75:                                           ; preds = %exit77, %x_body71
  %x_increment78 = add nuw nsw i64 %x73, 1
  %x_postcondition79 = icmp eq i64 %x_increment78, %4
  br i1 %x_postcondition79, label %x_exit72, label %x_body71

x_exit72:                                         ; preds = %exit75
  %y_increment80 = add nuw nsw i64 %y70, 1
  %y_postcondition81 = icmp eq i64 %y_increment80, %4
  br i1 %y_postcondition81, label %y_exit69, label %y_body68

y_exit69:                                         ; preds = %x_exit72, %polly.loop_exit178, %polly.loop_exit72, %polly.cond160
  %dst = bitcast %u0Matrix* %37 to %f32Matrix*
  %66 = bitcast %u0Matrix* %2 to i8*
  call void @likely_release_mat(i8* %66) #2
  %67 = bitcast %u0Matrix* %23 to i8*
  call void @likely_release_mat(i8* %67) #2
  ret %f32Matrix* %dst

polly.loop_exit72:                                ; preds = %y_exit48
  %68 = add nsw i64 %18, -1
  %polly.fdiv_q.shr88 = ashr i64 %68, 5
  %polly.loop_guard93 = icmp sgt i64 %polly.fdiv_q.shr88, -1
  br i1 %polly.loop_guard93, label %polly.loop_preheader91, label %y_exit69

polly.cond160:                                    ; preds = %polly.loop_exit101
  %.not = icmp slt i32 %rows, 1
  %polly.loop_guard93.not = xor i1 %polly.loop_guard93, true
  %brmerge = or i1 %.not, %polly.loop_guard93.not
  br i1 %brmerge, label %y_exit69, label %polly.loop_preheader167

polly.loop_header90:                              ; preds = %polly.loop_exit101, %polly.loop_preheader91
  %.s2a159.1 = phi double [ undef, %polly.loop_preheader91 ], [ %.s2a159.2, %polly.loop_exit101 ]
  %.phiops.3 = phi double [ 0.000000e+00, %polly.loop_preheader91 ], [ %.phiops.4, %polly.loop_exit101 ]
  %polly.indvar94 = phi i64 [ 0, %polly.loop_preheader91 ], [ %polly.indvar_next95, %polly.loop_exit101 ]
  %polly.loop_guard102 = icmp sgt i64 %polly.indvar94, %pexp.p_div_q
  br i1 %polly.loop_guard102, label %polly.loop_exit101, label %polly.loop_header99.preheader

polly.loop_header99.preheader:                    ; preds = %polly.loop_header90
  %69 = shl nsw i64 %polly.indvar94, 5
  %70 = sub nsw i64 %18, %69
  %71 = add nsw i64 %70, -1
  %72 = icmp sgt i64 %71, 31
  %73 = select i1 %72, i64 31, i64 %71
  %polly.loop_guard121 = icmp sgt i64 %73, -1
  %polly.adjust_ub124 = add i64 %73, -1
  br label %polly.loop_header99

polly.loop_exit101:                               ; preds = %polly.loop_exit111, %polly.loop_header90
  %.s2a159.2 = phi double [ %.s2a159.1, %polly.loop_header90 ], [ %.s2a159.4, %polly.loop_exit111 ]
  %.phiops.4 = phi double [ %.phiops.3, %polly.loop_header90 ], [ %.phiops.6, %polly.loop_exit111 ]
  %polly.indvar_next95 = add nuw nsw i64 %polly.indvar94, 1
  %polly.loop_cond97 = icmp slt i64 %polly.indvar94, %polly.fdiv_q.shr88
  br i1 %polly.loop_cond97, label %polly.loop_header90, label %polly.cond160

polly.loop_preheader91:                           ; preds = %polly.loop_exit72
  %scevgep144145 = bitcast %u0Matrix* %28 to i8*
  %pexp.p_div_q = lshr i64 %68, 5
  %74 = add nsw i64 %10, -1
  %polly.fdiv_q.shr107 = ashr i64 %74, 5
  %polly.loop_guard112 = icmp sgt i64 %polly.fdiv_q.shr107, -1
  br label %polly.loop_header90

polly.loop_header99:                              ; preds = %polly.loop_header99.preheader, %polly.loop_exit111
  %.s2a159.3 = phi double [ %.s2a159.4, %polly.loop_exit111 ], [ %.s2a159.1, %polly.loop_header99.preheader ]
  %.phiops.5 = phi double [ %.phiops.6, %polly.loop_exit111 ], [ %.phiops.3, %polly.loop_header99.preheader ]
  %polly.indvar103 = phi i64 [ %polly.indvar_next104, %polly.loop_exit111 ], [ %polly.indvar94, %polly.loop_header99.preheader ]
  br i1 %polly.loop_guard112, label %polly.loop_header109.preheader, label %polly.loop_exit111

polly.loop_header109.preheader:                   ; preds = %polly.loop_header99
  %75 = shl nsw i64 %polly.indvar103, 5
  %76 = sub nsw i64 %69, %75
  %77 = sub nsw i64 %18, %75
  %78 = add nsw i64 %77, -1
  %79 = icmp sgt i64 %78, 31
  %80 = select i1 %79, i64 31, i64 %78
  %polly.adjust_ub133 = add i64 %80, -1
  br label %polly.loop_header109

polly.loop_exit111:                               ; preds = %polly.loop_exit120, %polly.loop_header99
  %.s2a159.4 = phi double [ %.s2a159.3, %polly.loop_header99 ], [ %.s2a159.6, %polly.loop_exit120 ]
  %.phiops.6 = phi double [ %.phiops.5, %polly.loop_header99 ], [ %.phiops.8, %polly.loop_exit120 ]
  %polly.indvar_next104 = add nuw nsw i64 %polly.indvar103, 1
  %polly.loop_cond106 = icmp slt i64 %polly.indvar103, %pexp.p_div_q
  br i1 %polly.loop_cond106, label %polly.loop_header99, label %polly.loop_exit101

polly.loop_header109:                             ; preds = %polly.loop_header109.preheader, %polly.loop_exit120
  %.s2a159.5 = phi double [ %.s2a159.6, %polly.loop_exit120 ], [ %.s2a159.3, %polly.loop_header109.preheader ]
  %.phiops.7 = phi double [ %.phiops.8, %polly.loop_exit120 ], [ %.phiops.5, %polly.loop_header109.preheader ]
  %polly.indvar113 = phi i64 [ %polly.indvar_next114, %polly.loop_exit120 ], [ 0, %polly.loop_header109.preheader ]
  br i1 %polly.loop_guard121, label %polly.loop_header118.preheader, label %polly.loop_exit120

polly.loop_header118.preheader:                   ; preds = %polly.loop_header109
  %81 = shl nsw i64 %polly.indvar113, 5
  %82 = sub nsw i64 %10, %81
  %83 = add nsw i64 %82, -1
  %84 = icmp sgt i64 %83, 31
  %85 = select i1 %84, i64 31, i64 %83
  %polly.loop_guard139 = icmp sgt i64 %85, -1
  %polly.adjust_ub142 = add i64 %85, -1
  br i1 %polly.loop_guard139, label %polly.loop_header118.us, label %polly.loop_exit120

polly.loop_header118.us:                          ; preds = %polly.loop_header118.preheader, %polly.loop_exit129.us
  %.s2a159.7.us = phi double [ %.s2a159.8.us, %polly.loop_exit129.us ], [ %.s2a159.5, %polly.loop_header118.preheader ]
  %.phiops.9.us = phi double [ %.phiops.10.us, %polly.loop_exit129.us ], [ %.phiops.7, %polly.loop_header118.preheader ]
  %polly.indvar122.us = phi i64 [ %polly.indvar_next123.us, %polly.loop_exit129.us ], [ 0, %polly.loop_header118.preheader ]
  %86 = add nsw i64 %polly.indvar122.us, %76
  %87 = icmp slt i64 %86, 0
  %88 = select i1 %87, i64 0, i64 %86
  %polly.loop_guard130.us = icmp sgt i64 %88, %80
  br i1 %polly.loop_guard130.us, label %polly.loop_exit129.us, label %polly.loop_header127.preheader.us

polly.loop_exit129.us:                            ; preds = %polly.loop_exit138.loopexit.us.us, %polly.loop_header118.us
  %.s2a159.8.us = phi double [ %.s2a159.7.us, %polly.loop_header118.us ], [ %p_157.us.us, %polly.loop_exit138.loopexit.us.us ]
  %.phiops.10.us = phi double [ %.phiops.9.us, %polly.loop_header118.us ], [ %p_157.us.us, %polly.loop_exit138.loopexit.us.us ]
  %polly.indvar_next123.us = add nuw nsw i64 %polly.indvar122.us, 1
  %polly.loop_cond125.us = icmp sgt i64 %polly.indvar122.us, %polly.adjust_ub124
  br i1 %polly.loop_cond125.us, label %polly.loop_exit120, label %polly.loop_header118.us

polly.loop_header127.preheader.us:                ; preds = %polly.loop_header118.us
  %89 = add nuw nsw i64 %polly.indvar122.us, %69
  %90 = shl i64 %89, 2
  br label %polly.loop_header127.us.us

polly.loop_header127.us.us:                       ; preds = %polly.loop_header127.preheader.us, %polly.loop_exit138.loopexit.us.us
  %.phiops.11.us.us = phi double [ %p_157.us.us, %polly.loop_exit138.loopexit.us.us ], [ %.phiops.9.us, %polly.loop_header127.preheader.us ]
  %polly.indvar131.us.us = phi i64 [ %polly.indvar_next132.us.us, %polly.loop_exit138.loopexit.us.us ], [ %88, %polly.loop_header127.preheader.us ]
  %91 = add nsw i64 %polly.indvar131.us.us, %75
  %92 = shl i64 %91, 2
  br label %polly.loop_header136.us.us

polly.loop_exit138.loopexit.us.us:                ; preds = %polly.loop_header136.us.us
  %polly.indvar_next132.us.us = add nuw nsw i64 %polly.indvar131.us.us, 1
  %polly.loop_cond134.us.us = icmp sgt i64 %polly.indvar131.us.us, %polly.adjust_ub133
  br i1 %polly.loop_cond134.us.us, label %polly.loop_exit129.us, label %polly.loop_header127.us.us

polly.loop_header136.us.us:                       ; preds = %polly.loop_header136.us.us, %polly.loop_header127.us.us
  %.phiops.13.us.us = phi double [ %p_157.us.us, %polly.loop_header136.us.us ], [ %.phiops.11.us.us, %polly.loop_header127.us.us ]
  %polly.indvar140.us.us = phi i64 [ %polly.indvar_next141.us.us, %polly.loop_header136.us.us ], [ 0, %polly.loop_header127.us.us ]
  %93 = add nuw nsw i64 %polly.indvar140.us.us, %81
  %94 = mul i64 %93, %8
  %95 = add i64 %94, %92
  %uglygep146.us.us = getelementptr i8, i8* %scevgep144145, i64 %95
  %uglygep146147.us.us = bitcast i8* %uglygep146.us.us to float*
  %_p_scalar_148.us.us = load float, float* %uglygep146147.us.us, align 4, !alias.scope !4, !noalias !6
  %p_149.us.us = fpext float %_p_scalar_148.us.us to double
  %96 = add i64 %94, %90
  %uglygep152.us.us = getelementptr i8, i8* %scevgep144145, i64 %96
  %uglygep152153.us.us = bitcast i8* %uglygep152.us.us to float*
  %_p_scalar_154.us.us = load float, float* %uglygep152153.us.us, align 4, !alias.scope !4, !noalias !6
  %p_155.us.us = fpext float %_p_scalar_154.us.us to double
  %p_156.us.us = fmul fast double %p_155.us.us, %p_149.us.us
  %p_157.us.us = fadd fast double %p_156.us.us, %.phiops.13.us.us
  %polly.indvar_next141.us.us = add nuw nsw i64 %polly.indvar140.us.us, 1
  %polly.loop_cond143.us.us = icmp sgt i64 %polly.indvar140.us.us, %polly.adjust_ub142
  br i1 %polly.loop_cond143.us.us, label %polly.loop_exit138.loopexit.us.us, label %polly.loop_header136.us.us

polly.loop_exit120:                               ; preds = %polly.loop_exit129.us, %polly.loop_header118.preheader, %polly.loop_header109
  %.s2a159.6 = phi double [ %.s2a159.5, %polly.loop_header109 ], [ %.s2a159.5, %polly.loop_header118.preheader ], [ %.s2a159.8.us, %polly.loop_exit129.us ]
  %.phiops.8 = phi double [ %.phiops.7, %polly.loop_header109 ], [ %.phiops.7, %polly.loop_header118.preheader ], [ %.phiops.10.us, %polly.loop_exit129.us ]
  %polly.indvar_next114 = add nuw nsw i64 %polly.indvar113, 1
  %polly.loop_cond116 = icmp slt i64 %polly.indvar113, %polly.fdiv_q.shr107
  br i1 %polly.loop_cond116, label %polly.loop_header109, label %polly.loop_exit111

polly.loop_header166:                             ; preds = %polly.loop_exit178, %polly.loop_preheader167
  %polly.indvar170 = phi i64 [ 0, %polly.loop_preheader167 ], [ %polly.indvar_next171, %polly.loop_exit178 ]
  %polly.loop_guard179 = icmp sgt i64 %polly.indvar170, %pexp.p_div_q174
  br i1 %polly.loop_guard179, label %polly.loop_exit178, label %polly.loop_header176.preheader

polly.loop_header176.preheader:                   ; preds = %polly.loop_header166
  %97 = shl nsw i64 %polly.indvar170, 5
  %98 = sub nsw i64 %18, %97
  %99 = add nsw i64 %98, -1
  %100 = icmp sgt i64 %99, 31
  %101 = select i1 %100, i64 31, i64 %99
  %polly.loop_guard188 = icmp sgt i64 %101, -1
  %polly.adjust_ub191 = add i64 %101, -1
  br i1 %polly.loop_guard188, label %polly.loop_header176.us, label %polly.loop_exit178

polly.loop_header176.us:                          ; preds = %polly.loop_header176.preheader, %polly.loop_exit187.loopexit.us
  %polly.indvar180.us = phi i64 [ %polly.indvar_next181.us, %polly.loop_exit187.loopexit.us ], [ %polly.indvar170, %polly.loop_header176.preheader ]
  %102 = shl nsw i64 %polly.indvar180.us, 5
  %103 = sub nsw i64 %97, %102
  %104 = sub nsw i64 %18, %102
  %105 = add nsw i64 %104, -1
  %106 = icmp sgt i64 %105, 31
  %107 = select i1 %106, i64 31, i64 %105
  %polly.adjust_ub200.us = add i64 %107, -1
  br label %polly.loop_header185.us

polly.loop_header185.us:                          ; preds = %polly.loop_header176.us, %polly.loop_exit196.us
  %polly.indvar189.us = phi i64 [ %polly.indvar_next190.us, %polly.loop_exit196.us ], [ 0, %polly.loop_header176.us ]
  %108 = add nsw i64 %polly.indvar189.us, %103
  %109 = icmp slt i64 %108, 0
  %110 = select i1 %109, i64 0, i64 %108
  %polly.loop_guard197.us = icmp sgt i64 %110, %107
  br i1 %polly.loop_guard197.us, label %polly.loop_exit196.us, label %polly.loop_header194.preheader.us

polly.loop_header194.us:                          ; preds = %polly.loop_header194.preheader.us, %polly.loop_header194.us
  %polly.indvar198.us = phi i64 [ %polly.indvar_next199.us, %polly.loop_header194.us ], [ %110, %polly.loop_header194.preheader.us ]
  %111 = add nsw i64 %polly.indvar198.us, %102
  %112 = shl i64 %111, 2
  %113 = add i64 %112, %117
  %uglygep205.us = getelementptr i8, i8* %scevgep203204, i64 %113
  %uglygep205206.us = bitcast i8* %uglygep205.us to float*
  store float %p_202, float* %uglygep205206.us, align 4, !alias.scope !7, !noalias !10
  %114 = mul i64 %111, %8
  %115 = add i64 %114, %118
  %uglygep209.us = getelementptr i8, i8* %scevgep203204, i64 %115
  %uglygep209210.us = bitcast i8* %uglygep209.us to float*
  store float %p_202, float* %uglygep209210.us, align 4, !alias.scope !7, !noalias !10
  %polly.indvar_next199.us = add nuw nsw i64 %polly.indvar198.us, 1
  %polly.loop_cond201.us = icmp sgt i64 %polly.indvar198.us, %polly.adjust_ub200.us
  br i1 %polly.loop_cond201.us, label %polly.loop_exit196.us, label %polly.loop_header194.us

polly.loop_exit196.us:                            ; preds = %polly.loop_header194.us, %polly.loop_header185.us
  %polly.indvar_next190.us = add nuw nsw i64 %polly.indvar189.us, 1
  %polly.loop_cond192.us = icmp sgt i64 %polly.indvar189.us, %polly.adjust_ub191
  br i1 %polly.loop_cond192.us, label %polly.loop_exit187.loopexit.us, label %polly.loop_header185.us

polly.loop_header194.preheader.us:                ; preds = %polly.loop_header185.us
  %116 = add nuw nsw i64 %polly.indvar189.us, %97
  %117 = mul i64 %116, %8
  %118 = shl i64 %116, 2
  br label %polly.loop_header194.us

polly.loop_exit187.loopexit.us:                   ; preds = %polly.loop_exit196.us
  %polly.indvar_next181.us = add nuw nsw i64 %polly.indvar180.us, 1
  %polly.loop_cond183.us = icmp slt i64 %polly.indvar180.us, %pexp.p_div_q174
  br i1 %polly.loop_cond183.us, label %polly.loop_header176.us, label %polly.loop_exit178

polly.loop_exit178:                               ; preds = %polly.loop_exit187.loopexit.us, %polly.loop_header176.preheader, %polly.loop_header166
  %polly.indvar_next171 = add nuw nsw i64 %polly.indvar170, 1
  %polly.loop_cond173 = icmp slt i64 %polly.indvar170, %polly.fdiv_q.shr88
  br i1 %polly.loop_cond173, label %polly.loop_header166, label %y_exit69

polly.loop_preheader167:                          ; preds = %polly.cond160
  %scevgep203204 = bitcast %u0Matrix* %38 to i8*
  %pexp.p_div_q174 = lshr i64 %68, 5
  %p_202 = fptrunc double %.s2a159.2 to float
  br label %polly.loop_header166
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1) #0

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
!4 = distinct !{!4, !5, !"polly.alias.scope."}
!5 = distinct !{!5, !"polly.alias.scope.domain"}
!6 = !{!7, !8, !9}
!7 = distinct !{!7, !5, !"polly.alias.scope."}
!8 = distinct !{!8, !5, !"polly.alias.scope."}
!9 = distinct !{!9, !5, !"polly.alias.scope."}
!10 = !{!8, !4, !9}
