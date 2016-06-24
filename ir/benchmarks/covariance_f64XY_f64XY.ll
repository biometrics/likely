; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64Matrix = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define noalias %f64Matrix* @covariance(%f64Matrix* noalias nocapture readonly) #1 {
entry:
  %1 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0Matrix* @likely_new(i32 8512, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0Matrix, %u0Matrix* %2, i64 1
  %6 = bitcast %u0Matrix* %5 to i8*
  %7 = bitcast %u0Matrix* %5 to double*
  %8 = shl nuw nsw i64 %4, 3
  call void @llvm.memset.p0i8.i64(i8* %6, i8 0, i64 %8, i32 8, i1 false)
  %9 = zext i32 %rows to i64
  %10 = sext i32 %rows to i64
  br label %y_body

y_body:                                           ; preds = %entry, %x_exit8
  %y = phi i64 [ %y_increment, %x_exit8 ], [ 0, %entry ]
  %11 = mul nuw nsw i64 %y, %4
  br label %x_body7

x_body7:                                          ; preds = %y_body, %x_body7
  %x9 = phi i64 [ %x_increment10, %x_body7 ], [ 0, %y_body ]
  %12 = getelementptr double, double* %7, i64 %x9
  %13 = load double, double* %12, align 8
  %14 = add nuw nsw i64 %x9, %11
  %15 = getelementptr %f64Matrix, %f64Matrix* %0, i64 0, i32 6, i64 %14
  %16 = load double, double* %15, align 8
  %17 = fadd fast double %16, %13
  store double %17, double* %12, align 8
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
  %21 = uitofp i32 %rows to double
  %22 = fdiv fast double 1.000000e+00, %21
  br label %x_body15

Flow7:                                            ; preds = %x_body15, %y_exit
  %23 = call %u0Matrix* @likely_new(i32 24896, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %scevgep = getelementptr %u0Matrix, %u0Matrix* %23, i64 1, i32 0
  %24 = shl nuw nsw i64 %4, 1
  %scevgep3 = getelementptr %f64Matrix, %f64Matrix* %0, i64 1, i32 0
  br label %y_body28

x_body15:                                         ; preds = %true_entry, %x_body15
  %x17 = phi i64 [ %x_increment18, %x_body15 ], [ 0, %true_entry ]
  %25 = getelementptr double, double* %7, i64 %x17
  %26 = load double, double* %25, align 8, !llvm.mem.parallel_loop_access !1
  %27 = fmul fast double %26, %22
  store double %27, double* %25, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment18 = add nuw nsw i64 %x17, 1
  %x_postcondition19 = icmp eq i64 %x_increment18, %4
  br i1 %x_postcondition19, label %Flow7, label %x_body15

y_body28:                                         ; preds = %y_body28, %Flow7
  %y30 = phi i64 [ 0, %Flow7 ], [ %y_increment36, %y_body28 ]
  %28 = mul i64 %y30, %24
  %scevgep1 = getelementptr i32, i32* %scevgep, i64 %28
  %scevgep12 = bitcast i32* %scevgep1 to i8*
  %scevgep4 = getelementptr i32, i32* %scevgep3, i64 %28
  %scevgep45 = bitcast i32* %scevgep4 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep12, i8* %scevgep45, i64 %8, i32 8, i1 false)
  %y_increment36 = add nuw nsw i64 %y30, 1
  %y_postcondition37 = icmp eq i64 %y_increment36, %9
  br i1 %y_postcondition37, label %y_body47.preheader, label %y_body28

y_body47.preheader:                               ; preds = %y_body28
  %29 = getelementptr inbounds %u0Matrix, %u0Matrix* %23, i64 1
  %30 = bitcast %u0Matrix* %29 to double*
  br label %y_body47

y_body47:                                         ; preds = %x_exit51, %y_body47.preheader
  %y49 = phi i64 [ 0, %y_body47.preheader ], [ %y_increment55, %x_exit51 ]
  %31 = mul nuw nsw i64 %y49, %4
  br label %x_body50

x_body50:                                         ; preds = %y_body47, %x_body50
  %x52 = phi i64 [ %x_increment53, %x_body50 ], [ 0, %y_body47 ]
  %32 = add nuw nsw i64 %x52, %31
  %33 = getelementptr double, double* %30, i64 %32
  %34 = load double, double* %33, align 8, !llvm.mem.parallel_loop_access !2
  %35 = getelementptr double, double* %7, i64 %x52
  %36 = load double, double* %35, align 8, !llvm.mem.parallel_loop_access !2
  %37 = fsub fast double %34, %36
  store double %37, double* %33, align 8, !llvm.mem.parallel_loop_access !2
  %x_increment53 = add nuw nsw i64 %x52, 1
  %x_postcondition54 = icmp eq i64 %x_increment53, %4
  br i1 %x_postcondition54, label %x_exit51, label %x_body50

x_exit51:                                         ; preds = %x_body50
  %y_increment55 = add nuw nsw i64 %y49, 1
  %y_postcondition56 = icmp eq i64 %y_increment55, %9
  br i1 %y_postcondition56, label %y_exit48, label %y_body47

y_exit48:                                         ; preds = %x_exit51
  %38 = call %u0Matrix* @likely_new(i32 24896, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %39 = getelementptr inbounds %u0Matrix, %u0Matrix* %38, i64 1
  %40 = bitcast %u0Matrix* %39 to double*
  %41 = icmp slt i32 %rows, 1
  %42 = and i1 %19, %41
  br i1 %42, label %polly.loop_exit72, label %y_body68

y_body68:                                         ; preds = %y_exit48, %x_exit72
  %y70 = phi i64 [ %y_increment80, %x_exit72 ], [ 0, %y_exit48 ]
  %43 = mul nuw nsw i64 %y70, %4
  br label %x_body71

x_body71:                                         ; preds = %y_body68, %exit75
  %x73 = phi i64 [ %x_increment78, %exit75 ], [ 0, %y_body68 ]
  %44 = icmp ugt i64 %y70, %x73
  br i1 %44, label %exit75, label %true_entry76

true_entry76:                                     ; preds = %x_body71, %true_entry76
  %45 = phi i32 [ %57, %true_entry76 ], [ 0, %x_body71 ]
  %46 = phi double [ %56, %true_entry76 ], [ 0.000000e+00, %x_body71 ]
  %47 = zext i32 %45 to i64
  %48 = mul nuw nsw i64 %47, %4
  %49 = add nuw nsw i64 %48, %x73
  %50 = getelementptr double, double* %30, i64 %49
  %51 = load double, double* %50, align 8, !llvm.mem.parallel_loop_access !3
  %52 = add nuw nsw i64 %48, %y70
  %53 = getelementptr double, double* %30, i64 %52
  %54 = load double, double* %53, align 8, !llvm.mem.parallel_loop_access !3
  %55 = fmul fast double %54, %51
  %56 = fadd fast double %55, %46
  %57 = add nuw nsw i32 %45, 1
  %58 = icmp eq i32 %57, %rows
  br i1 %58, label %exit77, label %true_entry76

exit77:                                           ; preds = %true_entry76
  %59 = add nuw nsw i64 %x73, %43
  %60 = getelementptr double, double* %40, i64 %59
  store double %56, double* %60, align 8, !llvm.mem.parallel_loop_access !3
  %61 = mul nuw nsw i64 %x73, %4
  %62 = add nuw nsw i64 %61, %y70
  %63 = getelementptr double, double* %40, i64 %62
  store double %56, double* %63, align 8, !llvm.mem.parallel_loop_access !3
  br label %exit75

exit75:                                           ; preds = %exit77, %x_body71
  %x_increment78 = add nuw nsw i64 %x73, 1
  %x_postcondition79 = icmp eq i64 %x_increment78, %4
  br i1 %x_postcondition79, label %x_exit72, label %x_body71

x_exit72:                                         ; preds = %exit75
  %y_increment80 = add nuw nsw i64 %y70, 1
  %y_postcondition81 = icmp eq i64 %y_increment80, %4
  br i1 %y_postcondition81, label %y_exit69, label %y_body68

y_exit69:                                         ; preds = %x_exit72, %polly.loop_exit176, %polly.loop_exit72, %polly.cond158
  %dst = bitcast %u0Matrix* %38 to %f64Matrix*
  %64 = bitcast %u0Matrix* %2 to i8*
  call void @likely_release_mat(i8* %64) #2
  %65 = bitcast %u0Matrix* %23 to i8*
  call void @likely_release_mat(i8* %65) #2
  ret %f64Matrix* %dst

polly.loop_exit72:                                ; preds = %y_exit48
  %66 = add nsw i64 %18, -1
  %polly.fdiv_q.shr88 = ashr i64 %66, 5
  %polly.loop_guard93 = icmp sgt i64 %polly.fdiv_q.shr88, -1
  br i1 %polly.loop_guard93, label %polly.loop_preheader91, label %y_exit69

polly.cond158:                                    ; preds = %polly.loop_exit101
  %.not = icmp slt i32 %rows, 1
  %polly.loop_guard93.not = xor i1 %polly.loop_guard93, true
  %brmerge = or i1 %.not, %polly.loop_guard93.not
  br i1 %brmerge, label %y_exit69, label %polly.loop_preheader165

polly.loop_header90:                              ; preds = %polly.loop_exit101, %polly.loop_preheader91
  %.s2a157.1 = phi double [ undef, %polly.loop_preheader91 ], [ %.s2a157.2, %polly.loop_exit101 ]
  %.phiops.3 = phi double [ 0.000000e+00, %polly.loop_preheader91 ], [ %.phiops.4, %polly.loop_exit101 ]
  %polly.indvar94 = phi i64 [ 0, %polly.loop_preheader91 ], [ %polly.indvar_next95, %polly.loop_exit101 ]
  %polly.loop_guard102 = icmp sgt i64 %polly.indvar94, %pexp.p_div_q
  br i1 %polly.loop_guard102, label %polly.loop_exit101, label %polly.loop_header99.preheader

polly.loop_header99.preheader:                    ; preds = %polly.loop_header90
  %67 = shl nsw i64 %polly.indvar94, 5
  %68 = sub nsw i64 %18, %67
  %69 = add nsw i64 %68, -1
  %70 = icmp sgt i64 %69, 31
  %71 = select i1 %70, i64 31, i64 %69
  %polly.loop_guard121 = icmp sgt i64 %71, -1
  %polly.adjust_ub124 = add i64 %71, -1
  br label %polly.loop_header99

polly.loop_exit101:                               ; preds = %polly.loop_exit111, %polly.loop_header90
  %.s2a157.2 = phi double [ %.s2a157.1, %polly.loop_header90 ], [ %.s2a157.4, %polly.loop_exit111 ]
  %.phiops.4 = phi double [ %.phiops.3, %polly.loop_header90 ], [ %.phiops.6, %polly.loop_exit111 ]
  %polly.indvar_next95 = add nuw nsw i64 %polly.indvar94, 1
  %polly.loop_cond97 = icmp slt i64 %polly.indvar94, %polly.fdiv_q.shr88
  br i1 %polly.loop_cond97, label %polly.loop_header90, label %polly.cond158

polly.loop_preheader91:                           ; preds = %polly.loop_exit72
  %scevgep144145 = bitcast %u0Matrix* %29 to i8*
  %pexp.p_div_q = lshr i64 %66, 5
  %72 = add nsw i64 %10, -1
  %polly.fdiv_q.shr107 = ashr i64 %72, 5
  %polly.loop_guard112 = icmp sgt i64 %polly.fdiv_q.shr107, -1
  br label %polly.loop_header90

polly.loop_header99:                              ; preds = %polly.loop_header99.preheader, %polly.loop_exit111
  %.s2a157.3 = phi double [ %.s2a157.4, %polly.loop_exit111 ], [ %.s2a157.1, %polly.loop_header99.preheader ]
  %.phiops.5 = phi double [ %.phiops.6, %polly.loop_exit111 ], [ %.phiops.3, %polly.loop_header99.preheader ]
  %polly.indvar103 = phi i64 [ %polly.indvar_next104, %polly.loop_exit111 ], [ %polly.indvar94, %polly.loop_header99.preheader ]
  br i1 %polly.loop_guard112, label %polly.loop_header109.preheader, label %polly.loop_exit111

polly.loop_header109.preheader:                   ; preds = %polly.loop_header99
  %73 = shl nsw i64 %polly.indvar103, 5
  %74 = sub nsw i64 %67, %73
  %75 = sub nsw i64 %18, %73
  %76 = add nsw i64 %75, -1
  %77 = icmp sgt i64 %76, 31
  %78 = select i1 %77, i64 31, i64 %76
  %polly.adjust_ub133 = add i64 %78, -1
  br label %polly.loop_header109

polly.loop_exit111:                               ; preds = %polly.loop_exit120, %polly.loop_header99
  %.s2a157.4 = phi double [ %.s2a157.3, %polly.loop_header99 ], [ %.s2a157.6, %polly.loop_exit120 ]
  %.phiops.6 = phi double [ %.phiops.5, %polly.loop_header99 ], [ %.phiops.8, %polly.loop_exit120 ]
  %polly.indvar_next104 = add nuw nsw i64 %polly.indvar103, 1
  %polly.loop_cond106 = icmp slt i64 %polly.indvar103, %pexp.p_div_q
  br i1 %polly.loop_cond106, label %polly.loop_header99, label %polly.loop_exit101

polly.loop_header109:                             ; preds = %polly.loop_header109.preheader, %polly.loop_exit120
  %.s2a157.5 = phi double [ %.s2a157.6, %polly.loop_exit120 ], [ %.s2a157.3, %polly.loop_header109.preheader ]
  %.phiops.7 = phi double [ %.phiops.8, %polly.loop_exit120 ], [ %.phiops.5, %polly.loop_header109.preheader ]
  %polly.indvar113 = phi i64 [ %polly.indvar_next114, %polly.loop_exit120 ], [ 0, %polly.loop_header109.preheader ]
  br i1 %polly.loop_guard121, label %polly.loop_header118.preheader, label %polly.loop_exit120

polly.loop_header118.preheader:                   ; preds = %polly.loop_header109
  %79 = shl nsw i64 %polly.indvar113, 5
  %80 = sub nsw i64 %10, %79
  %81 = add nsw i64 %80, -1
  %82 = icmp sgt i64 %81, 31
  %83 = select i1 %82, i64 31, i64 %81
  %polly.loop_guard139 = icmp sgt i64 %83, -1
  %polly.adjust_ub142 = add i64 %83, -1
  br i1 %polly.loop_guard139, label %polly.loop_header118.us, label %polly.loop_exit120

polly.loop_header118.us:                          ; preds = %polly.loop_header118.preheader, %polly.loop_exit129.us
  %.s2a157.7.us = phi double [ %.s2a157.8.us, %polly.loop_exit129.us ], [ %.s2a157.5, %polly.loop_header118.preheader ]
  %.phiops.9.us = phi double [ %.phiops.10.us, %polly.loop_exit129.us ], [ %.phiops.7, %polly.loop_header118.preheader ]
  %polly.indvar122.us = phi i64 [ %polly.indvar_next123.us, %polly.loop_exit129.us ], [ 0, %polly.loop_header118.preheader ]
  %84 = add nsw i64 %polly.indvar122.us, %74
  %85 = icmp slt i64 %84, 0
  %86 = select i1 %85, i64 0, i64 %84
  %polly.loop_guard130.us = icmp sgt i64 %86, %78
  br i1 %polly.loop_guard130.us, label %polly.loop_exit129.us, label %polly.loop_header127.preheader.us

polly.loop_exit129.us:                            ; preds = %polly.loop_exit138.loopexit.us.us, %polly.loop_header118.us
  %.s2a157.8.us = phi double [ %.s2a157.7.us, %polly.loop_header118.us ], [ %p_155.us.us, %polly.loop_exit138.loopexit.us.us ]
  %.phiops.10.us = phi double [ %.phiops.9.us, %polly.loop_header118.us ], [ %p_155.us.us, %polly.loop_exit138.loopexit.us.us ]
  %polly.indvar_next123.us = add nuw nsw i64 %polly.indvar122.us, 1
  %polly.loop_cond125.us = icmp sgt i64 %polly.indvar122.us, %polly.adjust_ub124
  br i1 %polly.loop_cond125.us, label %polly.loop_exit120, label %polly.loop_header118.us

polly.loop_header127.preheader.us:                ; preds = %polly.loop_header118.us
  %87 = add nuw nsw i64 %polly.indvar122.us, %67
  %88 = shl i64 %87, 3
  br label %polly.loop_header127.us.us

polly.loop_header127.us.us:                       ; preds = %polly.loop_header127.preheader.us, %polly.loop_exit138.loopexit.us.us
  %.phiops.11.us.us = phi double [ %p_155.us.us, %polly.loop_exit138.loopexit.us.us ], [ %.phiops.9.us, %polly.loop_header127.preheader.us ]
  %polly.indvar131.us.us = phi i64 [ %polly.indvar_next132.us.us, %polly.loop_exit138.loopexit.us.us ], [ %86, %polly.loop_header127.preheader.us ]
  %89 = add nsw i64 %polly.indvar131.us.us, %73
  %90 = shl i64 %89, 3
  br label %polly.loop_header136.us.us

polly.loop_exit138.loopexit.us.us:                ; preds = %polly.loop_header136.us.us
  %polly.indvar_next132.us.us = add nuw nsw i64 %polly.indvar131.us.us, 1
  %polly.loop_cond134.us.us = icmp sgt i64 %polly.indvar131.us.us, %polly.adjust_ub133
  br i1 %polly.loop_cond134.us.us, label %polly.loop_exit129.us, label %polly.loop_header127.us.us

polly.loop_header136.us.us:                       ; preds = %polly.loop_header136.us.us, %polly.loop_header127.us.us
  %.phiops.13.us.us = phi double [ %p_155.us.us, %polly.loop_header136.us.us ], [ %.phiops.11.us.us, %polly.loop_header127.us.us ]
  %polly.indvar140.us.us = phi i64 [ %polly.indvar_next141.us.us, %polly.loop_header136.us.us ], [ 0, %polly.loop_header127.us.us ]
  %91 = add nuw nsw i64 %polly.indvar140.us.us, %79
  %92 = mul i64 %91, %8
  %93 = add i64 %92, %90
  %uglygep146.us.us = getelementptr i8, i8* %scevgep144145, i64 %93
  %uglygep146147.us.us = bitcast i8* %uglygep146.us.us to double*
  %_p_scalar_148.us.us = load double, double* %uglygep146147.us.us, align 8, !alias.scope !4, !noalias !6
  %94 = add i64 %92, %88
  %uglygep151.us.us = getelementptr i8, i8* %scevgep144145, i64 %94
  %uglygep151152.us.us = bitcast i8* %uglygep151.us.us to double*
  %_p_scalar_153.us.us = load double, double* %uglygep151152.us.us, align 8, !alias.scope !4, !noalias !6
  %p_154.us.us = fmul fast double %_p_scalar_153.us.us, %_p_scalar_148.us.us
  %p_155.us.us = fadd fast double %p_154.us.us, %.phiops.13.us.us
  %polly.indvar_next141.us.us = add nuw nsw i64 %polly.indvar140.us.us, 1
  %polly.loop_cond143.us.us = icmp sgt i64 %polly.indvar140.us.us, %polly.adjust_ub142
  br i1 %polly.loop_cond143.us.us, label %polly.loop_exit138.loopexit.us.us, label %polly.loop_header136.us.us

polly.loop_exit120:                               ; preds = %polly.loop_exit129.us, %polly.loop_header118.preheader, %polly.loop_header109
  %.s2a157.6 = phi double [ %.s2a157.5, %polly.loop_header109 ], [ %.s2a157.5, %polly.loop_header118.preheader ], [ %.s2a157.8.us, %polly.loop_exit129.us ]
  %.phiops.8 = phi double [ %.phiops.7, %polly.loop_header109 ], [ %.phiops.7, %polly.loop_header118.preheader ], [ %.phiops.10.us, %polly.loop_exit129.us ]
  %polly.indvar_next114 = add nuw nsw i64 %polly.indvar113, 1
  %polly.loop_cond116 = icmp slt i64 %polly.indvar113, %polly.fdiv_q.shr107
  br i1 %polly.loop_cond116, label %polly.loop_header109, label %polly.loop_exit111

polly.loop_header164:                             ; preds = %polly.loop_exit176, %polly.loop_preheader165
  %polly.indvar168 = phi i64 [ 0, %polly.loop_preheader165 ], [ %polly.indvar_next169, %polly.loop_exit176 ]
  %polly.loop_guard177 = icmp sgt i64 %polly.indvar168, %pexp.p_div_q172
  br i1 %polly.loop_guard177, label %polly.loop_exit176, label %polly.loop_header174.preheader

polly.loop_header174.preheader:                   ; preds = %polly.loop_header164
  %95 = shl nsw i64 %polly.indvar168, 5
  %96 = sub nsw i64 %18, %95
  %97 = add nsw i64 %96, -1
  %98 = icmp sgt i64 %97, 31
  %99 = select i1 %98, i64 31, i64 %97
  %polly.loop_guard186 = icmp sgt i64 %99, -1
  %polly.adjust_ub189 = add i64 %99, -1
  br i1 %polly.loop_guard186, label %polly.loop_header174.us, label %polly.loop_exit176

polly.loop_header174.us:                          ; preds = %polly.loop_header174.preheader, %polly.loop_exit185.loopexit.us
  %polly.indvar178.us = phi i64 [ %polly.indvar_next179.us, %polly.loop_exit185.loopexit.us ], [ %polly.indvar168, %polly.loop_header174.preheader ]
  %100 = shl nsw i64 %polly.indvar178.us, 5
  %101 = sub nsw i64 %95, %100
  %102 = sub nsw i64 %18, %100
  %103 = add nsw i64 %102, -1
  %104 = icmp sgt i64 %103, 31
  %105 = select i1 %104, i64 31, i64 %103
  %polly.adjust_ub198.us = add i64 %105, -1
  br label %polly.loop_header183.us

polly.loop_header183.us:                          ; preds = %polly.loop_header174.us, %polly.loop_exit194.us
  %polly.indvar187.us = phi i64 [ %polly.indvar_next188.us, %polly.loop_exit194.us ], [ 0, %polly.loop_header174.us ]
  %106 = add nsw i64 %polly.indvar187.us, %101
  %107 = icmp slt i64 %106, 0
  %108 = select i1 %107, i64 0, i64 %106
  %polly.loop_guard195.us = icmp sgt i64 %108, %105
  br i1 %polly.loop_guard195.us, label %polly.loop_exit194.us, label %polly.loop_header192.preheader.us

polly.loop_header192.us:                          ; preds = %polly.loop_header192.preheader.us, %polly.loop_header192.us
  %polly.indvar196.us = phi i64 [ %polly.indvar_next197.us, %polly.loop_header192.us ], [ %108, %polly.loop_header192.preheader.us ]
  %109 = add nsw i64 %polly.indvar196.us, %100
  %110 = shl i64 %109, 3
  %111 = add i64 %110, %115
  %uglygep202.us = getelementptr i8, i8* %scevgep200201, i64 %111
  %uglygep202203.us = bitcast i8* %uglygep202.us to double*
  store double %.s2a157.2, double* %uglygep202203.us, align 8, !alias.scope !8, !noalias !10
  %112 = mul i64 %109, %8
  %113 = add i64 %112, %116
  %uglygep206.us = getelementptr i8, i8* %scevgep200201, i64 %113
  %uglygep206207.us = bitcast i8* %uglygep206.us to double*
  store double %.s2a157.2, double* %uglygep206207.us, align 8, !alias.scope !8, !noalias !10
  %polly.indvar_next197.us = add nuw nsw i64 %polly.indvar196.us, 1
  %polly.loop_cond199.us = icmp sgt i64 %polly.indvar196.us, %polly.adjust_ub198.us
  br i1 %polly.loop_cond199.us, label %polly.loop_exit194.us, label %polly.loop_header192.us

polly.loop_exit194.us:                            ; preds = %polly.loop_header192.us, %polly.loop_header183.us
  %polly.indvar_next188.us = add nuw nsw i64 %polly.indvar187.us, 1
  %polly.loop_cond190.us = icmp sgt i64 %polly.indvar187.us, %polly.adjust_ub189
  br i1 %polly.loop_cond190.us, label %polly.loop_exit185.loopexit.us, label %polly.loop_header183.us

polly.loop_header192.preheader.us:                ; preds = %polly.loop_header183.us
  %114 = add nuw nsw i64 %polly.indvar187.us, %95
  %115 = mul i64 %114, %8
  %116 = shl i64 %114, 3
  br label %polly.loop_header192.us

polly.loop_exit185.loopexit.us:                   ; preds = %polly.loop_exit194.us
  %polly.indvar_next179.us = add nuw nsw i64 %polly.indvar178.us, 1
  %polly.loop_cond181.us = icmp slt i64 %polly.indvar178.us, %pexp.p_div_q172
  br i1 %polly.loop_cond181.us, label %polly.loop_header174.us, label %polly.loop_exit176

polly.loop_exit176:                               ; preds = %polly.loop_exit185.loopexit.us, %polly.loop_header174.preheader, %polly.loop_header164
  %polly.indvar_next169 = add nuw nsw i64 %polly.indvar168, 1
  %polly.loop_cond171 = icmp slt i64 %polly.indvar168, %polly.fdiv_q.shr88
  br i1 %polly.loop_cond171, label %polly.loop_header164, label %y_exit69

polly.loop_preheader165:                          ; preds = %polly.cond158
  %scevgep200201 = bitcast %u0Matrix* %39 to i8*
  %pexp.p_div_q172 = lshr i64 %66, 5
  br label %polly.loop_header164
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
!10 = !{!7, !4, !9}
