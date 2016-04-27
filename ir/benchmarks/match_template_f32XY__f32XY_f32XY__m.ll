; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: norecurse nounwind
define private void @match_template_tmp_thunk0({ %f32XY*, %f32XY*, %f32XY* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY* }, { %f32XY*, %f32XY*, %f32XY* }* %0, i64 0, i32 0
  %4 = load %f32XY*, %f32XY** %3, align 8
  %5 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY* }, { %f32XY*, %f32XY*, %f32XY* }* %0, i64 0, i32 1
  %6 = load %f32XY*, %f32XY** %5, align 8
  %7 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY* }, { %f32XY*, %f32XY*, %f32XY* }* %0, i64 0, i32 2
  %8 = load %f32XY*, %f32XY** %7, align 8
  %9 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 3
  %columns = load i32, i32* %9, align 4, !range !0
  %dst_y_step = zext i32 %columns to i64
  %10 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 6, i64 0
  %11 = ptrtoint float* %10 to i64
  %12 = and i64 %11, 31
  %13 = icmp eq i64 %12, 0
  call void @llvm.assume(i1 %13)
  %14 = getelementptr inbounds %f32XY, %f32XY* %6, i64 0, i32 3
  %15 = getelementptr inbounds %f32XY, %f32XY* %6, i64 0, i32 6, i64 0
  %16 = ptrtoint float* %15 to i64
  %17 = and i64 %16, 31
  %18 = icmp eq i64 %17, 0
  call void @llvm.assume(i1 %18)
  %19 = getelementptr inbounds %f32XY, %f32XY* %8, i64 0, i32 3
  %20 = getelementptr inbounds %f32XY, %f32XY* %8, i64 0, i32 4
  %21 = getelementptr inbounds %f32XY, %f32XY* %8, i64 0, i32 6, i64 0
  %22 = ptrtoint float* %21 to i64
  %23 = and i64 %22, 31
  %24 = icmp eq i64 %23, 0
  call void @llvm.assume(i1 %24)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %outer-y = phi i64 [ %1, %entry ], [ %y_increment27, %x_exit ]
  %25 = mul nuw nsw i64 %outer-y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %y_exit22
  %outer-x = phi i64 [ %x_increment25, %y_exit22 ], [ 0, %y_body ]
  %rows5 = load i32, i32* %20, align 4, !range !0
  %26 = zext i32 %rows5 to i64
  %columns11 = load i32, i32* %19, align 4, !range !0
  %templ_y_step14 = zext i32 %columns11 to i64
  %columns16 = load i32, i32* %14, align 4, !range !0
  %src_y_step19 = zext i32 %columns16 to i64
  br label %y_body21

y_body21:                                         ; preds = %x_body, %x_exit24
  %27 = phi double [ %41, %x_exit24 ], [ 0.000000e+00, %x_body ]
  %y = phi i64 [ %y_increment, %x_exit24 ], [ 0, %x_body ]
  %28 = add nuw nsw i64 %y, %outer-y
  %29 = mul nuw nsw i64 %28, %src_y_step19
  %30 = add i64 %29, %outer-x
  %31 = mul nuw nsw i64 %y, %templ_y_step14
  br label %x_body23

x_body23:                                         ; preds = %y_body21, %x_body23
  %32 = phi double [ %41, %x_body23 ], [ %27, %y_body21 ]
  %x = phi i64 [ %x_increment, %x_body23 ], [ 0, %y_body21 ]
  %33 = add i64 %30, %x
  %34 = getelementptr %f32XY, %f32XY* %6, i64 0, i32 6, i64 %33
  %35 = load float, float* %34, align 4
  %36 = add nuw nsw i64 %x, %31
  %37 = getelementptr %f32XY, %f32XY* %8, i64 0, i32 6, i64 %36
  %38 = load float, float* %37, align 4
  %39 = fmul fast float %38, %35
  %40 = fpext float %39 to double
  %41 = fadd fast double %40, %32
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %templ_y_step14
  br i1 %x_postcondition, label %x_exit24, label %x_body23

x_exit24:                                         ; preds = %x_body23
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %26
  br i1 %y_postcondition, label %y_exit22, label %y_body21

y_exit22:                                         ; preds = %x_exit24
  %42 = add nuw nsw i64 %outer-x, %25
  %43 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %42
  %44 = fptrunc double %41 to float
  store float %44, float* %43, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment25 = add nuw nsw i64 %outer-x, 1
  %x_postcondition26 = icmp eq i64 %x_increment25, %dst_y_step
  br i1 %x_postcondition26, label %x_exit, label %x_body

x_exit:                                           ; preds = %y_exit22
  %y_increment27 = add nuw nsw i64 %outer-y, 1
  %y_postcondition28 = icmp eq i64 %y_increment27, %2
  br i1 %y_postcondition28, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #2

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
  %dst = bitcast %u0CXYT* %10 to %f32XY*
  %11 = zext i32 %9 to i64
  %12 = alloca { %f32XY*, %f32XY*, %f32XY* }, align 8
  %13 = bitcast { %f32XY*, %f32XY*, %f32XY* }* %12 to %u0CXYT**
  store %u0CXYT* %10, %u0CXYT** %13, align 8
  %14 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY* }, { %f32XY*, %f32XY*, %f32XY* }* %12, i64 0, i32 1
  store %f32XY* %0, %f32XY** %14, align 8
  %15 = getelementptr inbounds { %f32XY*, %f32XY*, %f32XY* }, { %f32XY*, %f32XY*, %f32XY* }* %12, i64 0, i32 2
  store %f32XY* %1, %f32XY** %15, align 8
  %16 = bitcast { %f32XY*, %f32XY*, %f32XY* }* %12 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32XY*, %f32XY*, %f32XY* }*, i64, i64)* @match_template_tmp_thunk0 to i8*), i8* %16, i64 %11)
  ret %f32XY* %dst
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { norecurse nounwind }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
