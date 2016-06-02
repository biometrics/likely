; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

define noalias %f32XY* @filter_2D(%f32XY* nocapture readonly, %f32XY* nocapture readonly) {
entry:
  %2 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 3
  %width = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 4
  %height = load i32, i32* %3, align 4, !range !0
  %4 = srem i32 %width, 2
  %5 = icmp eq i32 %4, 1
  call void @llvm.assume(i1 %5)
  %6 = srem i32 %height, 2
  %7 = icmp eq i32 %6, 1
  call void @llvm.assume(i1 %7)
  %8 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %8, align 4, !range !0
  %9 = add i32 %width, -1
  %10 = add nuw nsw i32 %columns, %9
  %11 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 4
  %rows = load i32, i32* %11, align 4, !range !0
  %12 = add i32 %height, -1
  %13 = add nuw nsw i32 %rows, %12
  %14 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %10, i32 %13, i32 1, i8* null)
  %15 = zext i32 %13 to i64
  %mat_y_step = zext i32 %10 to i64
  %16 = getelementptr inbounds %u0CXYT, %u0CXYT* %14, i64 1
  %17 = bitcast %u0CXYT* %16 to float*
  %scevgep7 = getelementptr %u0CXYT, %u0CXYT* %14, i64 1, i32 0
  %18 = add i32 %width, -1
  %19 = add i32 %18, %columns
  %20 = zext i32 %19 to i64
  %21 = shl nuw nsw i64 %20, 2
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %22 = mul i64 %y, %20
  %scevgep8 = getelementptr i32, i32* %scevgep7, i64 %22
  %scevgep89 = bitcast i32* %scevgep8 to i8*
  call void @llvm.memset.p0i8.i64(i8* %scevgep89, i8 0, i64 %21, i32 4, i1 false)
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %15
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %pad-columns = sdiv i32 %9, 2
  %pad-rows = sdiv i32 %12, 2
  %23 = zext i32 %rows to i64
  %src_y_step = zext i32 %columns to i64
  %24 = sext i32 %pad-rows to i64
  %25 = sext i32 %pad-columns to i64
  %scevgep4 = getelementptr %f32XY, %f32XY* %0, i64 1, i32 0
  %26 = shl nuw nsw i64 %src_y_step, 2
  br label %y_body9

y_body9:                                          ; preds = %y_body9, %y_exit
  %y11 = phi i64 [ 0, %y_exit ], [ %y_increment17, %y_body9 ]
  %tmp = add i64 %y11, %24
  %tmp10 = mul i64 %tmp, %20
  %27 = add i64 %tmp10, %25
  %scevgep2 = getelementptr i32, i32* %scevgep7, i64 %27
  %scevgep23 = bitcast i32* %scevgep2 to i8*
  %28 = mul i64 %y11, %src_y_step
  %scevgep5 = getelementptr i32, i32* %scevgep4, i64 %28
  %scevgep56 = bitcast i32* %scevgep5 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep23, i8* %scevgep56, i64 %26, i32 4, i1 false)
  %y_increment17 = add nuw nsw i64 %y11, 1
  %y_postcondition18 = icmp eq i64 %y_increment17, %23
  br i1 %y_postcondition18, label %y_exit10, label %y_body9

y_exit10:                                         ; preds = %y_body9
  %29 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %30 = getelementptr inbounds %u0CXYT, %u0CXYT* %29, i64 1
  %31 = bitcast %u0CXYT* %30 to float*
  %kernel_y_step = zext i32 %width to i64
  br label %y_body31

y_body31:                                         ; preds = %x_exit35, %y_exit10
  %y33 = phi i64 [ 0, %y_exit10 ], [ %y_increment43, %x_exit35 ]
  %32 = mul nuw nsw i64 %y33, %src_y_step
  br label %x_body34

x_body34:                                         ; preds = %y_body31, %exit
  %x36 = phi i64 [ %x_increment41, %exit ], [ 0, %y_body31 ]
  %33 = add nuw nsw i64 %x36, %32
  br label %loop38.preheader

loop38.preheader:                                 ; preds = %x_body34, %exit40
  %34 = phi i32 [ %56, %exit40 ], [ 0, %x_body34 ]
  %35 = phi float [ %53, %exit40 ], [ 0.000000e+00, %x_body34 ]
  %36 = sext i32 %34 to i64
  %37 = add nuw nsw i64 %36, %y33
  %38 = mul nuw nsw i64 %37, %mat_y_step
  %39 = add i64 %38, %x36
  %40 = mul nuw nsw i64 %36, %kernel_y_step
  br label %true_entry39

exit:                                             ; preds = %exit40
  %41 = getelementptr float, float* %31, i64 %33
  store float %53, float* %41, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment41 = add nuw nsw i64 %x36, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %src_y_step
  br i1 %x_postcondition42, label %x_exit35, label %x_body34

x_exit35:                                         ; preds = %exit
  %y_increment43 = add nuw nsw i64 %y33, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %23
  br i1 %y_postcondition44, label %y_exit32, label %y_body31

y_exit32:                                         ; preds = %x_exit35
  %dst = bitcast %u0CXYT* %29 to %f32XY*
  %42 = bitcast %u0CXYT* %14 to i8*
  call void @likely_release_mat(i8* %42)
  ret %f32XY* %dst

true_entry39:                                     ; preds = %loop38.preheader, %true_entry39
  %43 = phi float [ %53, %true_entry39 ], [ %35, %loop38.preheader ]
  %44 = phi i32 [ %54, %true_entry39 ], [ 0, %loop38.preheader ]
  %45 = sext i32 %44 to i64
  %46 = add i64 %39, %45
  %47 = getelementptr float, float* %17, i64 %46
  %48 = load float, float* %47, align 4, !llvm.mem.parallel_loop_access !1
  %49 = add nuw nsw i64 %45, %40
  %50 = getelementptr %f32XY, %f32XY* %1, i64 0, i32 6, i64 %49
  %51 = load float, float* %50, align 4, !llvm.mem.parallel_loop_access !1
  %52 = fmul fast float %51, %48
  %53 = fadd fast float %52, %43
  %54 = add nuw nsw i32 %44, 1
  %55 = icmp eq i32 %54, %width
  br i1 %55, label %exit40, label %true_entry39

exit40:                                           ; preds = %true_entry39
  %56 = add nuw nsw i32 %34, 1
  %57 = icmp eq i32 %56, %height
  br i1 %57, label %exit, label %loop38.preheader
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1) #1

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #1

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind }
attributes #1 = { argmemonly nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
