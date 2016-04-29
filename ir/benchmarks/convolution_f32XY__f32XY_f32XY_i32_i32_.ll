; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind argmemonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f32XY* @convolution(%f32XY*, %f32XY*, i32, i32) {
entry:
  %4 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %4, align 4, !range !0
  %5 = shl i32 %3, 1
  %6 = add i32 %columns, %5
  %7 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 4
  %rows = load i32, i32* %7, align 4, !range !0
  %8 = add nuw nsw i32 %rows, %5
  %9 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %6, i32 %8, i32 1, i8* null)
  %10 = zext i32 %8 to i64
  %11 = getelementptr inbounds %u0CXYT, %u0CXYT* %9, i64 1
  %scevgep14 = getelementptr %u0CXYT, %u0CXYT* %9, i64 1, i32 0
  %12 = zext i32 %6 to i64
  %13 = shl nuw nsw i64 %12, 2
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %14 = mul i64 %y, %12
  %scevgep15 = getelementptr i32, i32* %scevgep14, i64 %14
  %scevgep1516 = bitcast i32* %scevgep15 to i8*
  call void @llvm.memset.p0i8.i64(i8* %scevgep1516, i8 0, i64 %13, i32 4, i1 false)
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %10
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %15 = bitcast %u0CXYT* %11 to float*
  %16 = zext i32 %rows to i64
  %17 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 6, i64 0
  %18 = ptrtoint float* %17 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  %21 = add nuw nsw i64 %12, 1
  %22 = sext i32 %3 to i64
  %23 = mul i64 %21, %22
  %scevgep11 = getelementptr %f32XY, %f32XY* %0, i64 1, i32 0
  %24 = zext i32 %columns to i64
  %25 = shl nuw nsw i64 %24, 2
  br label %y_body9

y_body9:                                          ; preds = %y_body9, %y_exit
  %y11 = phi i64 [ 0, %y_exit ], [ %y_increment17, %y_body9 ]
  %26 = mul i64 %y11, %12
  %27 = add i64 %26, %23
  %scevgep9 = getelementptr i32, i32* %scevgep14, i64 %27
  %scevgep910 = bitcast i32* %scevgep9 to i8*
  %28 = mul i64 %y11, %24
  %scevgep12 = getelementptr i32, i32* %scevgep11, i64 %28
  %scevgep1213 = bitcast i32* %scevgep12 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep910, i8* %scevgep1213, i64 %25, i32 4, i1 false)
  %y_increment17 = add nuw nsw i64 %y11, 1
  %y_postcondition18 = icmp eq i64 %y_increment17, %16
  br i1 %y_postcondition18, label %y_exit10, label %y_body9

y_exit10:                                         ; preds = %y_body9
  %29 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 3
  %columns20 = load i32, i32* %29, align 4, !range !0
  %30 = sub i32 %6, %columns20
  %31 = sdiv i32 %30, %2
  %32 = add nuw nsw i32 %31, 1
  %33 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 4
  %rows22 = load i32, i32* %33, align 4, !range !0
  %34 = sub i32 %8, %rows22
  %35 = sdiv i32 %34, %2
  %36 = add nuw nsw i32 %35, 1
  %37 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %32, i32 %36, i32 1, i8* null)
  %38 = zext i32 %36 to i64
  %dst_y_step = zext i32 %32 to i64
  %39 = getelementptr inbounds %u0CXYT, %u0CXYT* %37, i64 1
  %40 = bitcast %u0CXYT* %39 to float*
  %kernel_y_step = zext i32 %columns20 to i64
  %kernel_y = zext i32 %rows22 to i64
  %41 = sext i32 %2 to i64
  br label %y_body33

y_body33:                                         ; preds = %x_exit37, %y_exit10
  %y35 = phi i64 [ 0, %y_exit10 ], [ %y_increment67, %x_exit37 ]
  %42 = mul nuw nsw i64 %y35, %dst_y_step
  %src-y = mul nuw nsw i64 %y35, %41
  br label %x_body36

x_body36:                                         ; preds = %y_body33, %y_exit56
  %x38 = phi i64 [ %x_increment65, %y_exit56 ], [ 0, %y_body33 ]
  %src-x = mul nuw nsw i64 %x38, %41
  br label %y_body55

y_body55:                                         ; preds = %x_body36, %x_exit59
  %43 = phi double [ %57, %x_exit59 ], [ 0.000000e+00, %x_body36 ]
  %y57 = phi i64 [ %y_increment63, %x_exit59 ], [ 0, %x_body36 ]
  %44 = add nuw nsw i64 %y57, %src-y
  %45 = mul nuw nsw i64 %44, %12
  %46 = add i64 %45, %src-x
  %47 = mul nuw nsw i64 %y57, %kernel_y_step
  br label %x_body58

x_body58:                                         ; preds = %y_body55, %x_body58
  %48 = phi double [ %57, %x_body58 ], [ %43, %y_body55 ]
  %x60 = phi i64 [ %x_increment61, %x_body58 ], [ 0, %y_body55 ]
  %49 = add i64 %46, %x60
  %50 = getelementptr float, float* %15, i64 %49
  %51 = load float, float* %50, align 4
  %52 = add nuw nsw i64 %x60, %47
  %53 = getelementptr %f32XY, %f32XY* %1, i64 0, i32 6, i64 %52
  %54 = load float, float* %53, align 4
  %55 = fmul fast float %54, %51
  %56 = fpext float %55 to double
  %57 = fadd fast double %56, %48
  %x_increment61 = add nuw nsw i64 %x60, 1
  %x_postcondition62 = icmp eq i64 %x_increment61, %kernel_y_step
  br i1 %x_postcondition62, label %x_exit59, label %x_body58

x_exit59:                                         ; preds = %x_body58
  %y_increment63 = add nuw nsw i64 %y57, 1
  %y_postcondition64 = icmp eq i64 %y_increment63, %kernel_y
  br i1 %y_postcondition64, label %y_exit56, label %y_body55

y_exit56:                                         ; preds = %x_exit59
  %58 = add nuw nsw i64 %x38, %42
  %59 = getelementptr float, float* %40, i64 %58
  %60 = fptrunc double %57 to float
  store float %60, float* %59, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment65 = add nuw nsw i64 %x38, 1
  %x_postcondition66 = icmp eq i64 %x_increment65, %dst_y_step
  br i1 %x_postcondition66, label %x_exit37, label %x_body36

x_exit37:                                         ; preds = %y_exit56
  %y_increment67 = add nuw nsw i64 %y35, 1
  %y_postcondition68 = icmp eq i64 %y_increment67, %38
  br i1 %y_postcondition68, label %y_exit34, label %y_body33

y_exit34:                                         ; preds = %x_exit37
  %dst = bitcast %u0CXYT* %37 to %f32XY*
  %61 = bitcast %u0CXYT* %9 to i8*
  call void @likely_release_mat(i8* %61)
  ret %f32XY* %dst
}

; Function Attrs: nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1) #1

; Function Attrs: nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #1

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind argmemonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
