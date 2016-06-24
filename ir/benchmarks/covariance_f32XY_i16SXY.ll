; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%u16Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define noalias %f32Matrix* @covariance(%u16Matrix* noalias nocapture readonly) #1 {
entry:
  %1 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0Matrix* @likely_new(i32 9504, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 4
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
  %15 = getelementptr %u16Matrix, %u16Matrix* %0, i64 0, i32 6, i64 %14
  %16 = load i16, i16* %15, align 2
  %17 = sitofp i16 %16 to float
  %18 = fadd fast float %17, %13
  store float %18, float* %12, align 4
  %x_increment10 = add nuw nsw i64 %x9, 1
  %x_postcondition11 = icmp eq i64 %x_increment10, %4
  br i1 %x_postcondition11, label %x_exit8, label %x_body7

x_exit8:                                          ; preds = %x_body7
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %9
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit8
  %19 = sext i32 %columns to i64
  %20 = icmp sgt i32 %columns, -1
  %21 = icmp eq i32 %rows, 1
  br i1 %21, label %Flow2, label %true_entry

true_entry:                                       ; preds = %y_exit
  %22 = uitofp i32 %rows to float
  %23 = fdiv fast float 1.000000e+00, %22
  br label %x_body15

Flow2:                                            ; preds = %x_body15, %y_exit
  %24 = call %u0Matrix* @likely_new(i32 25888, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %25 = getelementptr inbounds %u0Matrix, %u0Matrix* %24, i64 1
  %26 = bitcast %u0Matrix* %25 to float*
  br label %y_body28

x_body15:                                         ; preds = %true_entry, %x_body15
  %x17 = phi i64 [ %x_increment18, %x_body15 ], [ 0, %true_entry ]
  %27 = getelementptr float, float* %7, i64 %x17
  %28 = load float, float* %27, align 4, !llvm.mem.parallel_loop_access !1
  %29 = fmul fast float %28, %23
  store float %29, float* %27, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment18 = add nuw nsw i64 %x17, 1
  %x_postcondition19 = icmp eq i64 %x_increment18, %4
  br i1 %x_postcondition19, label %Flow2, label %x_body15

y_body28:                                         ; preds = %x_exit32, %Flow2
  %y30 = phi i64 [ 0, %Flow2 ], [ %y_increment36, %x_exit32 ]
  %30 = mul nuw nsw i64 %y30, %4
  br label %x_body31

x_body31:                                         ; preds = %y_body28, %x_body31
  %x33 = phi i64 [ %x_increment34, %x_body31 ], [ 0, %y_body28 ]
  %31 = add nuw nsw i64 %x33, %30
  %32 = getelementptr %u16Matrix, %u16Matrix* %0, i64 0, i32 6, i64 %31
  %33 = load i16, i16* %32, align 2, !llvm.mem.parallel_loop_access !2
  %34 = getelementptr float, float* %26, i64 %31
  %35 = sitofp i16 %33 to float
  store float %35, float* %34, align 4, !llvm.mem.parallel_loop_access !2
  %x_increment34 = add nuw nsw i64 %x33, 1
  %x_postcondition35 = icmp eq i64 %x_increment34, %4
  br i1 %x_postcondition35, label %x_exit32, label %x_body31

x_exit32:                                         ; preds = %x_body31
  %y_increment36 = add nuw nsw i64 %y30, 1
  %y_postcondition37 = icmp eq i64 %y_increment36, %9
  br i1 %y_postcondition37, label %y_body47, label %y_body28

y_body47:                                         ; preds = %x_exit32, %x_exit51
  %y49 = phi i64 [ %y_increment55, %x_exit51 ], [ 0, %x_exit32 ]
  %36 = mul nuw nsw i64 %y49, %4
  br label %x_body50

x_body50:                                         ; preds = %y_body47, %x_body50
  %x52 = phi i64 [ %x_increment53, %x_body50 ], [ 0, %y_body47 ]
  %37 = add nuw nsw i64 %x52, %36
  %38 = getelementptr float, float* %26, i64 %37
  %39 = load float, float* %38, align 4, !llvm.mem.parallel_loop_access !3
  %40 = getelementptr float, float* %7, i64 %x52
  %41 = load float, float* %40, align 4, !llvm.mem.parallel_loop_access !3
  %42 = fsub fast float %39, %41
  store float %42, float* %38, align 4, !llvm.mem.parallel_loop_access !3
  %x_increment53 = add nuw nsw i64 %x52, 1
  %x_postcondition54 = icmp eq i64 %x_increment53, %4
  br i1 %x_postcondition54, label %x_exit51, label %x_body50

x_exit51:                                         ; preds = %x_body50
  %y_increment55 = add nuw nsw i64 %y49, 1
  %y_postcondition56 = icmp eq i64 %y_increment55, %9
  br i1 %y_postcondition56, label %y_exit48, label %y_body47

y_exit48:                                         ; preds = %x_exit51
  %43 = call %u0Matrix* @likely_new(i32 25888, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %44 = getelementptr inbounds %u0Matrix, %u0Matrix* %43, i64 1
  %45 = bitcast %u0Matrix* %44 to float*
  %46 = icmp slt i32 %rows, 1
  %47 = and i1 %20, %46
  br i1 %47, label %polly.loop_exit67, label %y_body68

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
  %55 = getelementptr float, float* %26, i64 %54
  %56 = load float, float* %55, align 4, !llvm.mem.parallel_loop_access !4
  %57 = fpext float %56 to double
  %58 = add nuw nsw i64 %53, %y70
  %59 = getelementptr float, float* %26, i64 %58
  %60 = load float, float* %59, align 4, !llvm.mem.parallel_loop_access !4
  %61 = fpext float %60 to double
  %62 = fmul fast double %61, %57
  %63 = fadd fast double %62, %51
  %64 = add nuw nsw i32 %50, 1
  %65 = icmp eq i32 %64, %rows
  br i1 %65, label %exit77, label %true_entry76

exit77:                                           ; preds = %true_entry76
  %66 = add nuw nsw i64 %x73, %48
  %67 = getelementptr float, float* %45, i64 %66
  %68 = fptrunc double %63 to float
  store float %68, float* %67, align 4, !llvm.mem.parallel_loop_access !4
  %69 = mul nuw nsw i64 %x73, %4
  %70 = add nuw nsw i64 %69, %y70
  %71 = getelementptr float, float* %45, i64 %70
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

y_exit69:                                         ; preds = %x_exit72, %polly.loop_exit173, %polly.loop_exit67, %polly.cond155
  %dst = bitcast %u0Matrix* %43 to %f32Matrix*
  %72 = bitcast %u0Matrix* %2 to i8*
  call void @likely_release_mat(i8* %72) #2
  %73 = bitcast %u0Matrix* %24 to i8*
  call void @likely_release_mat(i8* %73) #2
  ret %f32Matrix* %dst

polly.loop_exit67:                                ; preds = %y_exit48
  %74 = add nsw i64 %19, -1
  %polly.fdiv_q.shr83 = ashr i64 %74, 5
  %polly.loop_guard88 = icmp sgt i64 %polly.fdiv_q.shr83, -1
  br i1 %polly.loop_guard88, label %polly.loop_preheader86, label %y_exit69

polly.cond155:                                    ; preds = %polly.loop_exit96
  %.not = icmp slt i32 %rows, 1
  %polly.loop_guard88.not = xor i1 %polly.loop_guard88, true
  %brmerge = or i1 %.not, %polly.loop_guard88.not
  br i1 %brmerge, label %y_exit69, label %polly.loop_preheader162

polly.loop_header85:                              ; preds = %polly.loop_exit96, %polly.loop_preheader86
  %.s2a154.1 = phi double [ undef, %polly.loop_preheader86 ], [ %.s2a154.2, %polly.loop_exit96 ]
  %.phiops.3 = phi double [ 0.000000e+00, %polly.loop_preheader86 ], [ %.phiops.4, %polly.loop_exit96 ]
  %polly.indvar89 = phi i64 [ 0, %polly.loop_preheader86 ], [ %polly.indvar_next90, %polly.loop_exit96 ]
  %polly.loop_guard97 = icmp sgt i64 %polly.indvar89, %pexp.p_div_q
  br i1 %polly.loop_guard97, label %polly.loop_exit96, label %polly.loop_header94.preheader

polly.loop_header94.preheader:                    ; preds = %polly.loop_header85
  %75 = shl nsw i64 %polly.indvar89, 5
  %76 = sub nsw i64 %19, %75
  %77 = add nsw i64 %76, -1
  %78 = icmp sgt i64 %77, 31
  %79 = select i1 %78, i64 31, i64 %77
  %polly.loop_guard116 = icmp sgt i64 %79, -1
  %polly.adjust_ub119 = add i64 %79, -1
  br label %polly.loop_header94

polly.loop_exit96:                                ; preds = %polly.loop_exit106, %polly.loop_header85
  %.s2a154.2 = phi double [ %.s2a154.1, %polly.loop_header85 ], [ %.s2a154.4, %polly.loop_exit106 ]
  %.phiops.4 = phi double [ %.phiops.3, %polly.loop_header85 ], [ %.phiops.6, %polly.loop_exit106 ]
  %polly.indvar_next90 = add nuw nsw i64 %polly.indvar89, 1
  %polly.loop_cond92 = icmp slt i64 %polly.indvar89, %polly.fdiv_q.shr83
  br i1 %polly.loop_cond92, label %polly.loop_header85, label %polly.cond155

polly.loop_preheader86:                           ; preds = %polly.loop_exit67
  %scevgep139140 = bitcast %u0Matrix* %25 to i8*
  %pexp.p_div_q = lshr i64 %74, 5
  %80 = add nsw i64 %10, -1
  %polly.fdiv_q.shr102 = ashr i64 %80, 5
  %polly.loop_guard107 = icmp sgt i64 %polly.fdiv_q.shr102, -1
  br label %polly.loop_header85

polly.loop_header94:                              ; preds = %polly.loop_header94.preheader, %polly.loop_exit106
  %.s2a154.3 = phi double [ %.s2a154.4, %polly.loop_exit106 ], [ %.s2a154.1, %polly.loop_header94.preheader ]
  %.phiops.5 = phi double [ %.phiops.6, %polly.loop_exit106 ], [ %.phiops.3, %polly.loop_header94.preheader ]
  %polly.indvar98 = phi i64 [ %polly.indvar_next99, %polly.loop_exit106 ], [ %polly.indvar89, %polly.loop_header94.preheader ]
  br i1 %polly.loop_guard107, label %polly.loop_header104.preheader, label %polly.loop_exit106

polly.loop_header104.preheader:                   ; preds = %polly.loop_header94
  %81 = shl nsw i64 %polly.indvar98, 5
  %82 = sub nsw i64 %75, %81
  %83 = sub nsw i64 %19, %81
  %84 = add nsw i64 %83, -1
  %85 = icmp sgt i64 %84, 31
  %86 = select i1 %85, i64 31, i64 %84
  %polly.adjust_ub128 = add i64 %86, -1
  br label %polly.loop_header104

polly.loop_exit106:                               ; preds = %polly.loop_exit115, %polly.loop_header94
  %.s2a154.4 = phi double [ %.s2a154.3, %polly.loop_header94 ], [ %.s2a154.6, %polly.loop_exit115 ]
  %.phiops.6 = phi double [ %.phiops.5, %polly.loop_header94 ], [ %.phiops.8, %polly.loop_exit115 ]
  %polly.indvar_next99 = add nuw nsw i64 %polly.indvar98, 1
  %polly.loop_cond101 = icmp slt i64 %polly.indvar98, %pexp.p_div_q
  br i1 %polly.loop_cond101, label %polly.loop_header94, label %polly.loop_exit96

polly.loop_header104:                             ; preds = %polly.loop_header104.preheader, %polly.loop_exit115
  %.s2a154.5 = phi double [ %.s2a154.6, %polly.loop_exit115 ], [ %.s2a154.3, %polly.loop_header104.preheader ]
  %.phiops.7 = phi double [ %.phiops.8, %polly.loop_exit115 ], [ %.phiops.5, %polly.loop_header104.preheader ]
  %polly.indvar108 = phi i64 [ %polly.indvar_next109, %polly.loop_exit115 ], [ 0, %polly.loop_header104.preheader ]
  br i1 %polly.loop_guard116, label %polly.loop_header113.preheader, label %polly.loop_exit115

polly.loop_header113.preheader:                   ; preds = %polly.loop_header104
  %87 = shl nsw i64 %polly.indvar108, 5
  %88 = sub nsw i64 %10, %87
  %89 = add nsw i64 %88, -1
  %90 = icmp sgt i64 %89, 31
  %91 = select i1 %90, i64 31, i64 %89
  %polly.loop_guard134 = icmp sgt i64 %91, -1
  %polly.adjust_ub137 = add i64 %91, -1
  br i1 %polly.loop_guard134, label %polly.loop_header113.us, label %polly.loop_exit115

polly.loop_header113.us:                          ; preds = %polly.loop_header113.preheader, %polly.loop_exit124.us
  %.s2a154.7.us = phi double [ %.s2a154.8.us, %polly.loop_exit124.us ], [ %.s2a154.5, %polly.loop_header113.preheader ]
  %.phiops.9.us = phi double [ %.phiops.10.us, %polly.loop_exit124.us ], [ %.phiops.7, %polly.loop_header113.preheader ]
  %polly.indvar117.us = phi i64 [ %polly.indvar_next118.us, %polly.loop_exit124.us ], [ 0, %polly.loop_header113.preheader ]
  %92 = add nsw i64 %polly.indvar117.us, %82
  %93 = icmp slt i64 %92, 0
  %94 = select i1 %93, i64 0, i64 %92
  %polly.loop_guard125.us = icmp sgt i64 %94, %86
  br i1 %polly.loop_guard125.us, label %polly.loop_exit124.us, label %polly.loop_header122.preheader.us

polly.loop_exit124.us:                            ; preds = %polly.loop_exit133.loopexit.us.us, %polly.loop_header113.us
  %.s2a154.8.us = phi double [ %.s2a154.7.us, %polly.loop_header113.us ], [ %p_152.us.us, %polly.loop_exit133.loopexit.us.us ]
  %.phiops.10.us = phi double [ %.phiops.9.us, %polly.loop_header113.us ], [ %p_152.us.us, %polly.loop_exit133.loopexit.us.us ]
  %polly.indvar_next118.us = add nuw nsw i64 %polly.indvar117.us, 1
  %polly.loop_cond120.us = icmp sgt i64 %polly.indvar117.us, %polly.adjust_ub119
  br i1 %polly.loop_cond120.us, label %polly.loop_exit115, label %polly.loop_header113.us

polly.loop_header122.preheader.us:                ; preds = %polly.loop_header113.us
  %95 = add nuw nsw i64 %polly.indvar117.us, %75
  %96 = shl i64 %95, 2
  br label %polly.loop_header122.us.us

polly.loop_header122.us.us:                       ; preds = %polly.loop_header122.preheader.us, %polly.loop_exit133.loopexit.us.us
  %.phiops.11.us.us = phi double [ %p_152.us.us, %polly.loop_exit133.loopexit.us.us ], [ %.phiops.9.us, %polly.loop_header122.preheader.us ]
  %polly.indvar126.us.us = phi i64 [ %polly.indvar_next127.us.us, %polly.loop_exit133.loopexit.us.us ], [ %94, %polly.loop_header122.preheader.us ]
  %97 = add nsw i64 %polly.indvar126.us.us, %81
  %98 = shl i64 %97, 2
  br label %polly.loop_header131.us.us

polly.loop_exit133.loopexit.us.us:                ; preds = %polly.loop_header131.us.us
  %polly.indvar_next127.us.us = add nuw nsw i64 %polly.indvar126.us.us, 1
  %polly.loop_cond129.us.us = icmp sgt i64 %polly.indvar126.us.us, %polly.adjust_ub128
  br i1 %polly.loop_cond129.us.us, label %polly.loop_exit124.us, label %polly.loop_header122.us.us

polly.loop_header131.us.us:                       ; preds = %polly.loop_header131.us.us, %polly.loop_header122.us.us
  %.phiops.13.us.us = phi double [ %p_152.us.us, %polly.loop_header131.us.us ], [ %.phiops.11.us.us, %polly.loop_header122.us.us ]
  %polly.indvar135.us.us = phi i64 [ %polly.indvar_next136.us.us, %polly.loop_header131.us.us ], [ 0, %polly.loop_header122.us.us ]
  %99 = add nuw nsw i64 %polly.indvar135.us.us, %87
  %100 = mul i64 %99, %8
  %101 = add i64 %100, %98
  %uglygep141.us.us = getelementptr i8, i8* %scevgep139140, i64 %101
  %uglygep141142.us.us = bitcast i8* %uglygep141.us.us to float*
  %_p_scalar_143.us.us = load float, float* %uglygep141142.us.us, align 4, !alias.scope !5, !noalias !7
  %p_144.us.us = fpext float %_p_scalar_143.us.us to double
  %102 = add i64 %100, %96
  %uglygep147.us.us = getelementptr i8, i8* %scevgep139140, i64 %102
  %uglygep147148.us.us = bitcast i8* %uglygep147.us.us to float*
  %_p_scalar_149.us.us = load float, float* %uglygep147148.us.us, align 4, !alias.scope !5, !noalias !7
  %p_150.us.us = fpext float %_p_scalar_149.us.us to double
  %p_151.us.us = fmul fast double %p_150.us.us, %p_144.us.us
  %p_152.us.us = fadd fast double %p_151.us.us, %.phiops.13.us.us
  %polly.indvar_next136.us.us = add nuw nsw i64 %polly.indvar135.us.us, 1
  %polly.loop_cond138.us.us = icmp sgt i64 %polly.indvar135.us.us, %polly.adjust_ub137
  br i1 %polly.loop_cond138.us.us, label %polly.loop_exit133.loopexit.us.us, label %polly.loop_header131.us.us

polly.loop_exit115:                               ; preds = %polly.loop_exit124.us, %polly.loop_header113.preheader, %polly.loop_header104
  %.s2a154.6 = phi double [ %.s2a154.5, %polly.loop_header104 ], [ %.s2a154.5, %polly.loop_header113.preheader ], [ %.s2a154.8.us, %polly.loop_exit124.us ]
  %.phiops.8 = phi double [ %.phiops.7, %polly.loop_header104 ], [ %.phiops.7, %polly.loop_header113.preheader ], [ %.phiops.10.us, %polly.loop_exit124.us ]
  %polly.indvar_next109 = add nuw nsw i64 %polly.indvar108, 1
  %polly.loop_cond111 = icmp slt i64 %polly.indvar108, %polly.fdiv_q.shr102
  br i1 %polly.loop_cond111, label %polly.loop_header104, label %polly.loop_exit106

polly.loop_header161:                             ; preds = %polly.loop_exit173, %polly.loop_preheader162
  %polly.indvar165 = phi i64 [ 0, %polly.loop_preheader162 ], [ %polly.indvar_next166, %polly.loop_exit173 ]
  %polly.loop_guard174 = icmp sgt i64 %polly.indvar165, %pexp.p_div_q169
  br i1 %polly.loop_guard174, label %polly.loop_exit173, label %polly.loop_header171.preheader

polly.loop_header171.preheader:                   ; preds = %polly.loop_header161
  %103 = shl nsw i64 %polly.indvar165, 5
  %104 = sub nsw i64 %19, %103
  %105 = add nsw i64 %104, -1
  %106 = icmp sgt i64 %105, 31
  %107 = select i1 %106, i64 31, i64 %105
  %polly.loop_guard183 = icmp sgt i64 %107, -1
  %polly.adjust_ub186 = add i64 %107, -1
  br i1 %polly.loop_guard183, label %polly.loop_header171.us, label %polly.loop_exit173

polly.loop_header171.us:                          ; preds = %polly.loop_header171.preheader, %polly.loop_exit182.loopexit.us
  %polly.indvar175.us = phi i64 [ %polly.indvar_next176.us, %polly.loop_exit182.loopexit.us ], [ %polly.indvar165, %polly.loop_header171.preheader ]
  %108 = shl nsw i64 %polly.indvar175.us, 5
  %109 = sub nsw i64 %103, %108
  %110 = sub nsw i64 %19, %108
  %111 = add nsw i64 %110, -1
  %112 = icmp sgt i64 %111, 31
  %113 = select i1 %112, i64 31, i64 %111
  %polly.adjust_ub195.us = add i64 %113, -1
  br label %polly.loop_header180.us

polly.loop_header180.us:                          ; preds = %polly.loop_header171.us, %polly.loop_exit191.us
  %polly.indvar184.us = phi i64 [ %polly.indvar_next185.us, %polly.loop_exit191.us ], [ 0, %polly.loop_header171.us ]
  %114 = add nsw i64 %polly.indvar184.us, %109
  %115 = icmp slt i64 %114, 0
  %116 = select i1 %115, i64 0, i64 %114
  %polly.loop_guard192.us = icmp sgt i64 %116, %113
  br i1 %polly.loop_guard192.us, label %polly.loop_exit191.us, label %polly.loop_header189.preheader.us

polly.loop_header189.us:                          ; preds = %polly.loop_header189.preheader.us, %polly.loop_header189.us
  %polly.indvar193.us = phi i64 [ %polly.indvar_next194.us, %polly.loop_header189.us ], [ %116, %polly.loop_header189.preheader.us ]
  %117 = add nsw i64 %polly.indvar193.us, %108
  %118 = shl i64 %117, 2
  %119 = add i64 %118, %123
  %uglygep200.us = getelementptr i8, i8* %scevgep198199, i64 %119
  %uglygep200201.us = bitcast i8* %uglygep200.us to float*
  store float %p_197, float* %uglygep200201.us, align 4, !alias.scope !8, !noalias !11
  %120 = mul i64 %117, %8
  %121 = add i64 %120, %124
  %uglygep204.us = getelementptr i8, i8* %scevgep198199, i64 %121
  %uglygep204205.us = bitcast i8* %uglygep204.us to float*
  store float %p_197, float* %uglygep204205.us, align 4, !alias.scope !8, !noalias !11
  %polly.indvar_next194.us = add nuw nsw i64 %polly.indvar193.us, 1
  %polly.loop_cond196.us = icmp sgt i64 %polly.indvar193.us, %polly.adjust_ub195.us
  br i1 %polly.loop_cond196.us, label %polly.loop_exit191.us, label %polly.loop_header189.us

polly.loop_exit191.us:                            ; preds = %polly.loop_header189.us, %polly.loop_header180.us
  %polly.indvar_next185.us = add nuw nsw i64 %polly.indvar184.us, 1
  %polly.loop_cond187.us = icmp sgt i64 %polly.indvar184.us, %polly.adjust_ub186
  br i1 %polly.loop_cond187.us, label %polly.loop_exit182.loopexit.us, label %polly.loop_header180.us

polly.loop_header189.preheader.us:                ; preds = %polly.loop_header180.us
  %122 = add nuw nsw i64 %polly.indvar184.us, %103
  %123 = mul i64 %122, %8
  %124 = shl i64 %122, 2
  br label %polly.loop_header189.us

polly.loop_exit182.loopexit.us:                   ; preds = %polly.loop_exit191.us
  %polly.indvar_next176.us = add nuw nsw i64 %polly.indvar175.us, 1
  %polly.loop_cond178.us = icmp slt i64 %polly.indvar175.us, %pexp.p_div_q169
  br i1 %polly.loop_cond178.us, label %polly.loop_header171.us, label %polly.loop_exit173

polly.loop_exit173:                               ; preds = %polly.loop_exit182.loopexit.us, %polly.loop_header171.preheader, %polly.loop_header161
  %polly.indvar_next166 = add nuw nsw i64 %polly.indvar165, 1
  %polly.loop_cond168 = icmp slt i64 %polly.indvar165, %polly.fdiv_q.shr83
  br i1 %polly.loop_cond168, label %polly.loop_header161, label %y_exit69

polly.loop_preheader162:                          ; preds = %polly.cond155
  %scevgep198199 = bitcast %u0Matrix* %44 to i8*
  %pexp.p_div_q169 = lshr i64 %74, 5
  %p_197 = fptrunc double %.s2a154.2 to float
  br label %polly.loop_header161
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
