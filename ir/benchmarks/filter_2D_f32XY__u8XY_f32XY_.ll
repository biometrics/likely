; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
define noalias %f32Matrix* @filter_2D(%u8Matrix* noalias nocapture readonly, %f32Matrix* noalias nocapture readonly) #0 {
entry:
  %2 = getelementptr inbounds %f32Matrix, %f32Matrix* %1, i64 0, i32 3
  %width = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f32Matrix, %f32Matrix* %1, i64 0, i32 4
  %height = load i32, i32* %3, align 4, !range !0
  %4 = srem i32 %width, 2
  %5 = icmp eq i32 %4, 1
  call void @llvm.assume(i1 %5)
  %6 = srem i32 %height, 2
  %7 = icmp eq i32 %6, 1
  call void @llvm.assume(i1 %7)
  %8 = getelementptr inbounds %u8Matrix, %u8Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %8, align 4, !range !0
  %9 = add i32 %width, -1
  %10 = add nuw nsw i32 %columns, %9
  %11 = getelementptr inbounds %u8Matrix, %u8Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %11, align 4, !range !0
  %12 = add i32 %height, -1
  %13 = add nuw nsw i32 %rows, %12
  %14 = call %u0Matrix* @likely_new(i32 24584, i32 1, i32 %10, i32 %13, i32 1, i8* null)
  %15 = zext i32 %13 to i64
  %mat_y_step = zext i32 %10 to i64
  %16 = getelementptr inbounds %u0Matrix, %u0Matrix* %14, i64 1
  %scevgep6 = getelementptr %u0Matrix, %u0Matrix* %14, i64 1, i32 0
  %scevgep67 = bitcast i32* %scevgep6 to i8*
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %17 = mul i64 %y, %mat_y_step
  %uglygep8 = getelementptr i8, i8* %scevgep67, i64 %17
  call void @llvm.memset.p0i8.i64(i8* %uglygep8, i8 0, i64 %mat_y_step, i32 1, i1 false)
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %15
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %18 = bitcast %u0Matrix* %16 to i8*
  %pad-columns = sdiv i32 %9, 2
  %pad-rows = sdiv i32 %12, 2
  %19 = zext i32 %rows to i64
  %src_y_step = zext i32 %columns to i64
  %20 = sext i32 %pad-rows to i64
  %21 = sext i32 %pad-columns to i64
  %scevgep3 = getelementptr %u8Matrix, %u8Matrix* %0, i64 1, i32 0
  %scevgep34 = bitcast i32* %scevgep3 to i8*
  br label %y_body9

y_body9:                                          ; preds = %y_body9, %y_exit
  %y11 = phi i64 [ 0, %y_exit ], [ %y_increment17, %y_body9 ]
  %tmp = add i64 %y11, %20
  %tmp9 = mul i64 %tmp, %mat_y_step
  %22 = add i64 %tmp9, %21
  %uglygep = getelementptr i8, i8* %scevgep67, i64 %22
  %23 = mul i64 %y11, %src_y_step
  %uglygep5 = getelementptr i8, i8* %scevgep34, i64 %23
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %uglygep, i8* %uglygep5, i64 %src_y_step, i32 1, i1 false)
  %y_increment17 = add nuw nsw i64 %y11, 1
  %y_postcondition18 = icmp eq i64 %y_increment17, %19
  br i1 %y_postcondition18, label %y_exit10, label %y_body9

y_exit10:                                         ; preds = %y_body9
  %24 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %25 = getelementptr inbounds %u0Matrix, %u0Matrix* %24, i64 1
  %26 = bitcast %u0Matrix* %25 to float*
  %kernel_y_step = zext i32 %width to i64
  br label %y_body31

y_body31:                                         ; preds = %x_exit35, %y_exit10
  %y33 = phi i64 [ 0, %y_exit10 ], [ %y_increment43, %x_exit35 ]
  %27 = mul nuw nsw i64 %y33, %src_y_step
  br label %x_body34

x_body34:                                         ; preds = %y_body31, %exit
  %x36 = phi i64 [ %x_increment41, %exit ], [ 0, %y_body31 ]
  %28 = add nuw nsw i64 %x36, %27
  br label %loop38.preheader

loop38.preheader:                                 ; preds = %x_body34, %exit40
  %29 = phi i32 [ %50, %exit40 ], [ 0, %x_body34 ]
  %30 = phi float [ %47, %exit40 ], [ 0.000000e+00, %x_body34 ]
  %31 = zext i32 %29 to i64
  %32 = add nuw nsw i64 %31, %y33
  %33 = mul nuw nsw i64 %32, %mat_y_step
  %34 = add i64 %33, %x36
  %35 = mul nuw nsw i64 %31, %kernel_y_step
  br label %true_entry39

true_entry39:                                     ; preds = %loop38.preheader, %true_entry39
  %36 = phi float [ %47, %true_entry39 ], [ %30, %loop38.preheader ]
  %37 = phi i32 [ %48, %true_entry39 ], [ 0, %loop38.preheader ]
  %38 = zext i32 %37 to i64
  %39 = add i64 %34, %38
  %40 = getelementptr i8, i8* %18, i64 %39
  %41 = load i8, i8* %40, align 1, !llvm.mem.parallel_loop_access !1
  %42 = add nuw nsw i64 %38, %35
  %43 = getelementptr %f32Matrix, %f32Matrix* %1, i64 0, i32 6, i64 %42
  %44 = load float, float* %43, align 4, !llvm.mem.parallel_loop_access !1
  %45 = uitofp i8 %41 to float
  %46 = fmul fast float %45, %44
  %47 = fadd fast float %46, %36
  %48 = add nuw nsw i32 %37, 1
  %49 = icmp eq i32 %48, %width
  br i1 %49, label %exit40, label %true_entry39

exit40:                                           ; preds = %true_entry39
  %50 = add nuw nsw i32 %29, 1
  %51 = icmp eq i32 %50, %height
  br i1 %51, label %exit, label %loop38.preheader

exit:                                             ; preds = %exit40
  %52 = getelementptr float, float* %26, i64 %28
  store float %47, float* %52, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment41 = add nuw nsw i64 %x36, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %src_y_step
  br i1 %x_postcondition42, label %x_exit35, label %x_body34

x_exit35:                                         ; preds = %exit
  %y_increment43 = add nuw nsw i64 %y33, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %19
  br i1 %y_postcondition44, label %y_exit32, label %y_body31

y_exit32:                                         ; preds = %x_exit35
  %dst = bitcast %u0Matrix* %24 to %f32Matrix*
  %53 = bitcast %u0Matrix* %14 to i8*
  call void @likely_release_mat(i8* %53) #0
  ret %f32Matrix* %dst
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
