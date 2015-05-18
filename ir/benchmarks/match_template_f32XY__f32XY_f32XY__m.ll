; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @match_template_tmp_thunk0({ %f32XY*, %f32XY*, %f32XY*, i32, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %0, i64 0, i32 0
  %4 = load %f32XY*, %f32XY** %3, align 8
  %5 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %0, i64 0, i32 1
  %6 = load %f32XY*, %f32XY** %5, align 8
  %7 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %0, i64 0, i32 2
  %8 = load %f32XY*, %f32XY** %7, align 8
  %9 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %0, i64 0, i32 3
  %10 = load i32, i32* %9, align 4
  %11 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %0, i64 0, i32 4
  %12 = load i32, i32* %11, align 4
  %13 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 3
  %columns = load i32, i32* %13, align 4, !range !0
  %dst_y_step = zext i32 %columns to i64
  %14 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 6, i64 0
  %15 = ptrtoint float* %14 to i64
  %16 = and i64 %15, 31
  %17 = icmp eq i64 %16, 0
  call void @llvm.assume(i1 %17)
  %18 = getelementptr inbounds %f32XY, %f32XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %18, align 4, !range !0
  %src_y_step = zext i32 %columns1 to i64
  %19 = getelementptr inbounds %f32XY, %f32XY* %6, i64 0, i32 6, i64 0
  %20 = ptrtoint float* %19 to i64
  %21 = and i64 %20, 31
  %22 = icmp eq i64 %21, 0
  call void @llvm.assume(i1 %22)
  %23 = getelementptr inbounds %f32XY, %f32XY* %8, i64 0, i32 3
  %columns3 = load i32, i32* %23, align 4, !range !0
  %templ_y_step = zext i32 %columns3 to i64
  %24 = getelementptr inbounds %f32XY, %f32XY* %8, i64 0, i32 6, i64 0
  %25 = ptrtoint float* %24 to i64
  %26 = and i64 %25, 31
  %27 = icmp eq i64 %26, 0
  call void @llvm.assume(i1 %27)
  %28 = icmp eq i32 %12, 0
  %29 = icmp eq i32 %10, 0
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %30 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %exit, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %exit ]
  br i1 %28, label %exit, label %label5.preheader

label5.preheader:                                 ; preds = %x_body, %exit7
  %31 = phi i32 [ %55, %exit7 ], [ 0, %x_body ]
  %32 = phi double [ %.lcssa, %exit7 ], [ 0.000000e+00, %x_body ]
  br i1 %29, label %exit7, label %true_entry6.lr.ph

true_entry6.lr.ph:                                ; preds = %label5.preheader
  %33 = sext i32 %31 to i64
  %34 = add nuw nsw i64 %33, %y
  %35 = mul nuw nsw i64 %34, %src_y_step
  %36 = add i64 %35, %x
  %37 = mul nuw nsw i64 %33, %templ_y_step
  br label %true_entry6

exit:                                             ; preds = %exit7, %x_body
  %.lcssa8 = phi double [ 0.000000e+00, %x_body ], [ %.lcssa, %exit7 ]
  %38 = fptrunc double %.lcssa8 to float
  %39 = add nuw nsw i64 %x, %30
  %40 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %39
  store float %38, float* %40, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !1

x_exit:                                           ; preds = %exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void

true_entry6:                                      ; preds = %true_entry6.lr.ph, %true_entry6
  %41 = phi double [ %32, %true_entry6.lr.ph ], [ %52, %true_entry6 ]
  %42 = phi i32 [ 0, %true_entry6.lr.ph ], [ %53, %true_entry6 ]
  %43 = sext i32 %42 to i64
  %44 = add i64 %36, %43
  %45 = getelementptr %f32XY, %f32XY* %6, i64 0, i32 6, i64 %44
  %46 = load float, float* %45, align 4, !llvm.mem.parallel_loop_access !1
  %47 = add nuw nsw i64 %43, %37
  %48 = getelementptr %f32XY, %f32XY* %8, i64 0, i32 6, i64 %47
  %49 = load float, float* %48, align 4, !llvm.mem.parallel_loop_access !1
  %50 = fmul fast float %49, %46
  %51 = fpext float %50 to double
  %52 = fadd fast double %51, %41
  %53 = add nuw nsw i32 %42, 1
  %54 = icmp eq i32 %53, %10
  br i1 %54, label %exit7, label %true_entry6

exit7:                                            ; preds = %true_entry6, %label5.preheader
  %.lcssa = phi double [ %32, %label5.preheader ], [ %52, %true_entry6 ]
  %55 = add nuw nsw i32 %31, 1
  %56 = icmp eq i32 %55, %12
  br i1 %56, label %exit, label %label5.preheader
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %f32XY* @match_template(%f32XY*, %f32XY*) {
entry:
  %2 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 3
  %columns1 = load i32, i32* %3, align 4, !range !0
  %4 = add i32 %columns, 1
  %columns1.neg = sub i32 0, %columns1
  %5 = add i32 %4, %columns1.neg
  %6 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 4
  %rows = load i32, i32* %6, align 4, !range !0
  %7 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 4
  %rows2 = load i32, i32* %7, align 4, !range !0
  %8 = sub i32 %rows, %rows2
  %9 = add nuw nsw i32 %8, 1
  %10 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %5, i32 %9, i32 1, i8* null)
  %11 = bitcast %u0CXYT* %10 to %f32XY*
  %12 = zext i32 %9 to i64
  %13 = alloca { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, align 8
  %14 = bitcast { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %13 to %u0CXYT**
  store %u0CXYT* %10, %u0CXYT** %14, align 8
  %15 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %13, i64 0, i32 1
  store %f32XY* %0, %f32XY** %15, align 8
  %16 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %13, i64 0, i32 2
  store %f32XY* %1, %f32XY** %16, align 8
  %17 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %13, i64 0, i32 3
  store i32 %columns1, i32* %17, align 8
  %18 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY*, i32, i32 }, { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %13, i64 0, i32 4
  store i32 %rows2, i32* %18, align 4
  %19 = bitcast { %f32XY*, %f32XY*, %f32XY*, i32, i32 }* %13 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32XY*, %f32XY*, %f32XY*, i32, i32 }*, i64, i64)* @match_template_tmp_thunk0 to i8*), i8* %19, i64 %12)
  ret %f32XY* %11
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
