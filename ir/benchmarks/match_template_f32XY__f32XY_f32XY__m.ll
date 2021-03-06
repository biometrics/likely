; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: norecurse nounwind
define private void @match_template_tmp_thunk0({ %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }* %0, i64 0, i32 0
  %4 = load %f32Matrix*, %f32Matrix** %3, align 8
  %5 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }* %0, i64 0, i32 1
  %6 = load %f32Matrix*, %f32Matrix** %5, align 8
  %7 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }* %0, i64 0, i32 2
  %8 = load %f32Matrix*, %f32Matrix** %7, align 8
  %9 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }* %0, i64 0, i32 3
  %10 = load i32, i32* %9, align 4
  %11 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }* %0, i64 0, i32 4
  %12 = load i32, i32* %11, align 4
  %13 = getelementptr inbounds %f32Matrix, %f32Matrix* %4, i64 0, i32 3
  %columns = load i32, i32* %13, align 4, !range !0
  %dst_y_step = zext i32 %columns to i64
  %14 = getelementptr inbounds %f32Matrix, %f32Matrix* %6, i64 0, i32 3
  %columns1 = load i32, i32* %14, align 4, !range !0
  %src_y_step = zext i32 %columns1 to i64
  %15 = getelementptr inbounds %f32Matrix, %f32Matrix* %8, i64 0, i32 3
  %columns3 = load i32, i32* %15, align 4, !range !0
  %templ_y_step = zext i32 %columns3 to i64
  %16 = icmp eq i32 %10, 0
  %17 = icmp eq i32 %12, 0
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %18 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %exit
  %x = phi i64 [ %x_increment, %exit ], [ 0, %y_body ]
  br i1 %17, label %exit, label %loop6.preheader

loop6.preheader:                                  ; preds = %x_body, %exit8
  %19 = phi i32 [ %40, %exit8 ], [ 0, %x_body ]
  %20 = phi float [ %39, %exit8 ], [ 0.000000e+00, %x_body ]
  br i1 %16, label %exit8, label %true_entry7.lr.ph

true_entry7.lr.ph:                                ; preds = %loop6.preheader
  %21 = zext i32 %19 to i64
  %22 = add nuw nsw i64 %21, %y
  %23 = mul nuw nsw i64 %22, %src_y_step
  %24 = add i64 %23, %x
  %25 = mul nuw nsw i64 %21, %templ_y_step
  br label %true_entry7

true_entry7:                                      ; preds = %true_entry7.lr.ph, %true_entry7
  %26 = phi float [ %36, %true_entry7 ], [ %20, %true_entry7.lr.ph ]
  %27 = phi i32 [ %37, %true_entry7 ], [ 0, %true_entry7.lr.ph ]
  %28 = zext i32 %27 to i64
  %29 = add i64 %24, %28
  %30 = getelementptr %f32Matrix, %f32Matrix* %6, i64 0, i32 6, i64 %29
  %31 = load float, float* %30, align 4, !llvm.mem.parallel_loop_access !1
  %32 = add nuw nsw i64 %28, %25
  %33 = getelementptr %f32Matrix, %f32Matrix* %8, i64 0, i32 6, i64 %32
  %34 = load float, float* %33, align 4, !llvm.mem.parallel_loop_access !1
  %35 = fmul fast float %34, %31
  %36 = fadd fast float %35, %26
  %37 = add nuw nsw i32 %27, 1
  %38 = icmp eq i32 %37, %10
  br i1 %38, label %exit8, label %true_entry7

exit8:                                            ; preds = %true_entry7, %loop6.preheader
  %39 = phi float [ %20, %loop6.preheader ], [ %36, %true_entry7 ]
  %40 = add nuw nsw i32 %19, 1
  %41 = icmp eq i32 %40, %12
  br i1 %41, label %exit, label %loop6.preheader

exit:                                             ; preds = %exit8, %x_body
  %.lcssa9 = phi float [ 0.000000e+00, %x_body ], [ %39, %exit8 ]
  %42 = add nuw nsw i64 %x, %18
  %43 = getelementptr %f32Matrix, %f32Matrix* %4, i64 0, i32 6, i64 %42
  store float %.lcssa9, float* %43, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: nounwind
define noalias %f32Matrix* @match_template(%f32Matrix* noalias nocapture, %f32Matrix* noalias nocapture) #2 {
entry:
  %2 = getelementptr inbounds %f32Matrix, %f32Matrix* %1, i64 0, i32 3
  %width = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f32Matrix, %f32Matrix* %1, i64 0, i32 4
  %height = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %4, align 4, !range !0
  %width.neg = sub i32 0, %width
  %5 = add i32 %width.neg, 1
  %6 = add i32 %5, %columns
  %7 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %7, align 4, !range !0
  %8 = sub i32 %rows, %height
  %9 = add nuw nsw i32 %8, 1
  %10 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %6, i32 %9, i32 1, i8* null)
  %dst = bitcast %u0Matrix* %10 to %f32Matrix*
  %11 = zext i32 %9 to i64
  %12 = alloca { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }, align 8
  %13 = bitcast { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }* %12 to %u0Matrix**
  store %u0Matrix* %10, %u0Matrix** %13, align 8
  %14 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }* %12, i64 0, i32 1
  store %f32Matrix* %0, %f32Matrix** %14, align 8
  %15 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }* %12, i64 0, i32 2
  store %f32Matrix* %1, %f32Matrix** %15, align 8
  %16 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }* %12, i64 0, i32 3
  store i32 %width, i32* %16, align 8
  %17 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }, { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }* %12, i64 0, i32 4
  store i32 %height, i32* %17, align 4
  %18 = bitcast { %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }* %12 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, %f32Matrix*, %f32Matrix*, i32, i32 }*, i64, i64)* @match_template_tmp_thunk0 to i8*), i8* %18, i64 %11) #2
  ret %f32Matrix* %dst
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { norecurse nounwind }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
