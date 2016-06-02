; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%i16XY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

define noalias %f32XY* @filter_2D(%i16XY* nocapture readonly, %f32XY* nocapture readonly) {
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
  %8 = getelementptr inbounds %i16XY, %i16XY* %0, i64 0, i32 3
  %columns = load i32, i32* %8, align 4, !range !0
  %9 = add i32 %width, -1
  %10 = add nuw nsw i32 %columns, %9
  %11 = getelementptr inbounds %i16XY, %i16XY* %0, i64 0, i32 4
  %rows = load i32, i32* %11, align 4, !range !0
  %12 = add i32 %height, -1
  %13 = add nuw nsw i32 %rows, %12
  %14 = call %u0CXYT* @likely_new(i32 25104, i32 1, i32 %10, i32 %13, i32 1, i8* null)
  %15 = zext i32 %13 to i64
  %mat_y_step = zext i32 %10 to i64
  %16 = getelementptr inbounds %u0CXYT, %u0CXYT* %14, i64 1
  %17 = bitcast %u0CXYT* %16 to i16*
  %scevgep6 = getelementptr %u0CXYT, %u0CXYT* %14, i64 1, i32 0
  %scevgep67 = bitcast i32* %scevgep6 to i8*
  %18 = add i32 %width, -1
  %19 = add i32 %18, %columns
  %20 = zext i32 %19 to i64
  %21 = shl nuw nsw i64 %20, 1
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %22 = mul i64 %y, %21
  %uglygep8 = getelementptr i8, i8* %scevgep67, i64 %22
  call void @llvm.memset.p0i8.i64(i8* %uglygep8, i8 0, i64 %21, i32 2, i1 false)
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %15
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %pad-columns = sdiv i32 %9, 2
  %pad-rows = sdiv i32 %12, 2
  %23 = zext i32 %rows to i64
  %src_y_step = zext i32 %columns to i64
  %24 = sext i32 %pad-rows to i64
  %25 = mul nsw i64 %20, %24
  %26 = sext i32 %pad-columns to i64
  %27 = add i64 %25, %26
  %28 = mul i64 %27, 2
  %scevgep3 = getelementptr %i16XY, %i16XY* %0, i64 1, i32 0
  %scevgep34 = bitcast i32* %scevgep3 to i8*
  %29 = shl nuw nsw i64 %src_y_step, 1
  br label %y_body9

y_body9:                                          ; preds = %y_body9, %y_exit
  %y11 = phi i64 [ 0, %y_exit ], [ %y_increment17, %y_body9 ]
  %30 = mul i64 %y11, %21
  %31 = add i64 %30, %28
  %uglygep = getelementptr i8, i8* %scevgep67, i64 %31
  %32 = mul i64 %y11, %29
  %uglygep5 = getelementptr i8, i8* %scevgep34, i64 %32
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %uglygep, i8* %uglygep5, i64 %29, i32 2, i1 false)
  %y_increment17 = add nuw nsw i64 %y11, 1
  %y_postcondition18 = icmp eq i64 %y_increment17, %23
  br i1 %y_postcondition18, label %y_exit10, label %y_body9

y_exit10:                                         ; preds = %y_body9
  %33 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %34 = getelementptr inbounds %u0CXYT, %u0CXYT* %33, i64 1
  %35 = bitcast %u0CXYT* %34 to float*
  %kernel_y_step = zext i32 %width to i64
  br label %y_body31

y_body31:                                         ; preds = %x_exit35, %y_exit10
  %y33 = phi i64 [ 0, %y_exit10 ], [ %y_increment43, %x_exit35 ]
  %36 = mul nuw nsw i64 %y33, %src_y_step
  br label %x_body34

x_body34:                                         ; preds = %y_body31, %exit
  %x36 = phi i64 [ %x_increment41, %exit ], [ 0, %y_body31 ]
  %37 = add nuw nsw i64 %x36, %36
  br label %loop38.preheader

loop38.preheader:                                 ; preds = %x_body34, %exit40
  %38 = phi i32 [ %61, %exit40 ], [ 0, %x_body34 ]
  %39 = phi float [ %58, %exit40 ], [ 0.000000e+00, %x_body34 ]
  %40 = sext i32 %38 to i64
  %41 = add nuw nsw i64 %40, %y33
  %42 = mul nuw nsw i64 %41, %mat_y_step
  %43 = add i64 %42, %x36
  %44 = mul nuw nsw i64 %40, %kernel_y_step
  br label %true_entry39

exit:                                             ; preds = %exit40
  %45 = getelementptr float, float* %35, i64 %37
  store float %58, float* %45, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment41 = add nuw nsw i64 %x36, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %src_y_step
  br i1 %x_postcondition42, label %x_exit35, label %x_body34

x_exit35:                                         ; preds = %exit
  %y_increment43 = add nuw nsw i64 %y33, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %23
  br i1 %y_postcondition44, label %y_exit32, label %y_body31

y_exit32:                                         ; preds = %x_exit35
  %dst = bitcast %u0CXYT* %33 to %f32XY*
  %46 = bitcast %u0CXYT* %14 to i8*
  call void @likely_release_mat(i8* %46)
  ret %f32XY* %dst

true_entry39:                                     ; preds = %loop38.preheader, %true_entry39
  %47 = phi float [ %58, %true_entry39 ], [ %39, %loop38.preheader ]
  %48 = phi i32 [ %59, %true_entry39 ], [ 0, %loop38.preheader ]
  %49 = sext i32 %48 to i64
  %50 = add i64 %43, %49
  %51 = getelementptr i16, i16* %17, i64 %50
  %52 = load i16, i16* %51, align 2, !llvm.mem.parallel_loop_access !1
  %53 = add nuw nsw i64 %49, %44
  %54 = getelementptr %f32XY, %f32XY* %1, i64 0, i32 6, i64 %53
  %55 = load float, float* %54, align 4, !llvm.mem.parallel_loop_access !1
  %56 = sitofp i16 %52 to float
  %57 = fmul fast float %56, %55
  %58 = fadd fast float %57, %47
  %59 = add nuw nsw i32 %48, 1
  %60 = icmp eq i32 %59, %width
  br i1 %60, label %exit40, label %true_entry39

exit40:                                           ; preds = %true_entry39
  %61 = add nuw nsw i32 %38, 1
  %62 = icmp eq i32 %61, %height
  br i1 %62, label %exit, label %loop38.preheader
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
