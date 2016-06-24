; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define noalias %f32Matrix* @match_template(%f32Matrix* noalias nocapture readonly, %f32Matrix* noalias nocapture readonly) #1 {
entry:
  %2 = getelementptr inbounds %f32Matrix, %f32Matrix* %1, i64 0, i32 3
  %width = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f32Matrix, %f32Matrix* %1, i64 0, i32 4
  %height = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %4, align 4, !range !0
  %5 = sub i32 %columns, %width
  %6 = add nuw nsw i32 %5, 1
  %7 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %7, align 4, !range !0
  %8 = sub i32 %rows, %height
  %9 = add nuw nsw i32 %8, 1
  %10 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %6, i32 %9, i32 1, i8* null)
  %11 = zext i32 %9 to i64
  %dst_y_step = zext i32 %6 to i64
  %12 = getelementptr inbounds %u0Matrix, %u0Matrix* %10, i64 1
  %13 = bitcast %u0Matrix* %12 to float*
  %src_y_step = zext i32 %columns to i64
  %templ_y_step = zext i32 %width to i64
  %14 = sext i32 %height to i64
  %15 = icmp slt i32 %height, 1
  %16 = sext i32 %columns to i64
  %17 = sext i32 %width to i64
  %18 = icmp slt i32 %columns, %width
  %19 = or i1 %15, %18
  %20 = sext i32 %rows to i64
  %21 = icmp slt i32 %rows, %height
  %22 = or i1 %19, %21
  %23 = add nsw i64 %17, 2147483647
  %24 = icmp sge i64 %16, %23
  %25 = add nsw i64 %16, 2
  %26 = icmp sge i64 %17, %25
  %27 = or i1 %24, %26
  %28 = icmp slt i32 %width, 0
  %29 = or i1 %28, %27
  %30 = icmp sge i32 %columns, %width
  %31 = icmp sge i32 %rows, %height
  %32 = icmp sgt i32 %height, -1
  %33 = and i1 %32, %30
  %34 = add nsw i64 %14, 2147483647
  %35 = icmp sge i64 %20, %34
  %36 = and i1 %33, %35
  %37 = or i1 %36, %29
  %38 = icmp sgt i32 %width, 0
  %39 = icmp sgt i32 %height, 0
  %40 = and i1 %38, %39
  %41 = and i1 %40, %30
  %42 = and i1 %41, %31
  %43 = or i1 %42, %37
  %44 = xor i1 %43, true
  %polly.rtc.result = and i1 %22, %44
  br i1 %polly.rtc.result, label %polly.merge62, label %y_body

y_body:                                           ; preds = %entry, %x_exit
  %y = phi i64 [ %y_increment, %x_exit ], [ 0, %entry ]
  %45 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %exit
  %x = phi i64 [ %x_increment, %exit ], [ 0, %y_body ]
  br label %loop9.preheader

loop9.preheader:                                  ; preds = %x_body, %exit11
  %46 = phi i32 [ %66, %exit11 ], [ 0, %x_body ]
  %47 = phi float [ %63, %exit11 ], [ 0.000000e+00, %x_body ]
  %48 = zext i32 %46 to i64
  %49 = add nuw nsw i64 %48, %y
  %50 = mul nuw nsw i64 %49, %src_y_step
  %51 = add i64 %50, %x
  %52 = mul nuw nsw i64 %48, %templ_y_step
  br label %true_entry10

true_entry10:                                     ; preds = %loop9.preheader, %true_entry10
  %53 = phi float [ %63, %true_entry10 ], [ %47, %loop9.preheader ]
  %54 = phi i32 [ %64, %true_entry10 ], [ 0, %loop9.preheader ]
  %55 = zext i32 %54 to i64
  %56 = add i64 %51, %55
  %57 = getelementptr %f32Matrix, %f32Matrix* %0, i64 0, i32 6, i64 %56
  %58 = load float, float* %57, align 4, !llvm.mem.parallel_loop_access !1
  %59 = add nuw nsw i64 %55, %52
  %60 = getelementptr %f32Matrix, %f32Matrix* %1, i64 0, i32 6, i64 %59
  %61 = load float, float* %60, align 4, !llvm.mem.parallel_loop_access !1
  %62 = fmul fast float %61, %58
  %63 = fadd fast float %62, %53
  %64 = add nuw nsw i32 %54, 1
  %65 = icmp eq i32 %64, %width
  br i1 %65, label %exit11, label %true_entry10

exit11:                                           ; preds = %true_entry10
  %66 = add nuw nsw i32 %46, 1
  %67 = icmp eq i32 %66, %height
  br i1 %67, label %exit, label %loop9.preheader

exit:                                             ; preds = %exit11
  %68 = add nuw nsw i64 %x, %45
  %69 = getelementptr float, float* %13, i64 %68
  store float %63, float* %69, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %11
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %polly.loop_exit143, %x_exit, %polly.merge62
  %dst = bitcast %u0Matrix* %10 to %f32Matrix*
  ret %f32Matrix* %dst

polly.merge62:                                    ; preds = %entry
  %70 = sub nsw i64 %20, %14
  %polly.fdiv_q.shr129 = ashr i64 %70, 5
  %polly.loop_guard134 = icmp sgt i64 %polly.fdiv_q.shr129, -1
  br i1 %polly.loop_guard134, label %polly.loop_preheader132, label %y_exit

polly.loop_header131:                             ; preds = %polly.loop_exit143, %polly.loop_preheader132
  %polly.indvar135 = phi i64 [ 0, %polly.loop_preheader132 ], [ %polly.indvar_next136, %polly.loop_exit143 ]
  br i1 %polly.loop_guard144, label %polly.loop_header141.preheader, label %polly.loop_exit143

polly.loop_header141.preheader:                   ; preds = %polly.loop_header131
  %71 = shl nsw i64 %polly.indvar135, 5
  %72 = sub nsw i64 %70, %71
  %73 = icmp sgt i64 %72, 31
  %74 = select i1 %73, i64 31, i64 %72
  %polly.loop_guard173 = icmp sgt i64 %74, -1
  %polly.adjust_ub176 = add i64 %74, -1
  br label %polly.loop_header141

polly.loop_exit143:                               ; preds = %polly.loop_exit153, %polly.loop_header131
  %polly.indvar_next136 = add nuw nsw i64 %polly.indvar135, 1
  %polly.loop_cond138 = icmp slt i64 %polly.indvar135, %polly.fdiv_q.shr129
  br i1 %polly.loop_cond138, label %polly.loop_header131, label %y_exit

polly.loop_preheader132:                          ; preds = %polly.merge62
  %75 = sub nsw i64 %16, %17
  %polly.fdiv_q.shr139 = ashr i64 %75, 5
  %polly.loop_guard144 = icmp sgt i64 %polly.fdiv_q.shr139, -1
  %76 = add nsw i64 %14, -1
  %polly.fdiv_q.shr149 = ashr i64 %76, 5
  %polly.loop_guard154 = icmp sgt i64 %polly.fdiv_q.shr149, -1
  %77 = add nsw i64 %17, -1
  %polly.fdiv_q.shr159 = ashr i64 %77, 5
  %polly.loop_guard164 = icmp sgt i64 %polly.fdiv_q.shr159, -1
  br label %polly.loop_header131

polly.loop_header141:                             ; preds = %polly.loop_header141.preheader, %polly.loop_exit153
  %polly.indvar145 = phi i64 [ %polly.indvar_next146, %polly.loop_exit153 ], [ 0, %polly.loop_header141.preheader ]
  br i1 %polly.loop_guard154, label %polly.loop_header151.preheader, label %polly.loop_exit153

polly.loop_header151.preheader:                   ; preds = %polly.loop_header141
  %78 = shl nsw i64 %polly.indvar145, 5
  %79 = sub nsw i64 %75, %78
  %80 = icmp sgt i64 %79, 31
  %81 = select i1 %80, i64 31, i64 %79
  %polly.adjust_ub185 = add i64 %81, -1
  br label %polly.loop_header151

polly.loop_exit153:                               ; preds = %polly.loop_exit163, %polly.loop_header141
  %polly.indvar_next146 = add nuw nsw i64 %polly.indvar145, 1
  %polly.loop_cond148 = icmp slt i64 %polly.indvar145, %polly.fdiv_q.shr139
  br i1 %polly.loop_cond148, label %polly.loop_header141, label %polly.loop_exit143

polly.loop_header151:                             ; preds = %polly.loop_header151.preheader, %polly.loop_exit163
  %polly.indvar155 = phi i64 [ %polly.indvar_next156, %polly.loop_exit163 ], [ 0, %polly.loop_header151.preheader ]
  br i1 %polly.loop_guard164, label %polly.loop_header161.preheader, label %polly.loop_exit163

polly.loop_header161.preheader:                   ; preds = %polly.loop_header151
  %82 = shl nsw i64 %polly.indvar155, 5
  %83 = sub nsw i64 %14, %82
  %84 = add nsw i64 %83, -1
  %85 = icmp sgt i64 %84, 31
  %86 = select i1 %85, i64 31, i64 %84
  %polly.adjust_ub194 = add i64 %86, -1
  br label %polly.loop_header161

polly.loop_exit163:                               ; preds = %polly.loop_exit172, %polly.loop_header151
  %polly.indvar_next156 = add nuw nsw i64 %polly.indvar155, 1
  %polly.loop_cond158 = icmp slt i64 %polly.indvar155, %polly.fdiv_q.shr149
  br i1 %polly.loop_cond158, label %polly.loop_header151, label %polly.loop_exit153

polly.loop_header161:                             ; preds = %polly.loop_header161.preheader, %polly.loop_exit172
  %polly.indvar165 = phi i64 [ %polly.indvar_next166, %polly.loop_exit172 ], [ 0, %polly.loop_header161.preheader ]
  br i1 %polly.loop_guard173, label %polly.loop_header170.preheader, label %polly.loop_exit172

polly.loop_header170.preheader:                   ; preds = %polly.loop_header161
  %87 = shl nsw i64 %polly.indvar165, 5
  %88 = sub nsw i64 %17, %87
  %89 = add nsw i64 %88, -1
  %90 = icmp sgt i64 %89, 31
  %91 = select i1 %90, i64 31, i64 %89
  %polly.loop_guard200 = icmp sgt i64 %91, -1
  %polly.adjust_ub203 = add i64 %91, -1
  br label %polly.loop_header170

polly.loop_exit172:                               ; preds = %polly.loop_exit181, %polly.loop_header161
  %polly.indvar_next166 = add nuw nsw i64 %polly.indvar165, 1
  %polly.loop_cond168 = icmp slt i64 %polly.indvar165, %polly.fdiv_q.shr159
  br i1 %polly.loop_cond168, label %polly.loop_header161, label %polly.loop_exit163

polly.loop_header170:                             ; preds = %polly.loop_header170.preheader, %polly.loop_exit181
  %polly.indvar174 = phi i64 [ %polly.indvar_next175, %polly.loop_exit181 ], [ 0, %polly.loop_header170.preheader ]
  %92 = or i64 %81, %86
  %93 = icmp slt i64 %92, 0
  br i1 %93, label %polly.loop_exit181, label %polly.loop_header179.us

polly.loop_header179.us:                          ; preds = %polly.loop_header170, %polly.loop_exit190.loopexit.us
  %polly.indvar183.us = phi i64 [ %polly.indvar_next184.us, %polly.loop_exit190.loopexit.us ], [ 0, %polly.loop_header170 ]
  br i1 %polly.loop_guard200, label %polly.loop_header188.us.us, label %polly.loop_exit190.loopexit.us

polly.loop_exit190.loopexit.us:                   ; preds = %polly.loop_exit199.loopexit.us.us, %polly.loop_header179.us
  %polly.indvar_next184.us = add nuw nsw i64 %polly.indvar183.us, 1
  %polly.loop_cond186.us = icmp sgt i64 %polly.indvar183.us, %polly.adjust_ub185
  br i1 %polly.loop_cond186.us, label %polly.loop_exit181, label %polly.loop_header179.us

polly.loop_header188.us.us:                       ; preds = %polly.loop_header179.us, %polly.loop_exit199.loopexit.us.us
  %polly.indvar192.us.us = phi i64 [ %polly.indvar_next193.us.us, %polly.loop_exit199.loopexit.us.us ], [ 0, %polly.loop_header179.us ]
  br label %polly.loop_header197.us.us

polly.loop_exit199.loopexit.us.us:                ; preds = %polly.loop_header197.us.us
  %polly.indvar_next193.us.us = add nuw nsw i64 %polly.indvar192.us.us, 1
  %polly.loop_cond195.us.us = icmp sgt i64 %polly.indvar192.us.us, %polly.adjust_ub194
  br i1 %polly.loop_cond195.us.us, label %polly.loop_exit190.loopexit.us, label %polly.loop_header188.us.us

polly.loop_header197.us.us:                       ; preds = %polly.loop_header197.us.us, %polly.loop_header188.us.us
  %polly.indvar201.us.us = phi i64 [ %polly.indvar_next202.us.us, %polly.loop_header197.us.us ], [ 0, %polly.loop_header188.us.us ]
  %polly.indvar_next202.us.us = add nuw nsw i64 %polly.indvar201.us.us, 1
  %polly.loop_cond204.us.us = icmp sgt i64 %polly.indvar201.us.us, %polly.adjust_ub203
  br i1 %polly.loop_cond204.us.us, label %polly.loop_exit199.loopexit.us.us, label %polly.loop_header197.us.us

polly.loop_exit181:                               ; preds = %polly.loop_exit190.loopexit.us, %polly.loop_header170
  %polly.indvar_next175 = add nuw nsw i64 %polly.indvar174, 1
  %polly.loop_cond177 = icmp sgt i64 %polly.indvar174, %polly.adjust_ub176
  br i1 %polly.loop_cond177, label %polly.loop_exit172, label %polly.loop_header170
}

; Function Attrs: nounwind readnone
declare { i64, i1 } @llvm.sadd.with.overflow.i64(i64, i64) #2

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind "polly-optimized" }
attributes #2 = { nounwind readnone }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
