; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u16Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i16] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

define noalias %f32Matrix* @filter_2D(%u16Matrix* nocapture readonly, %f32Matrix* nocapture readonly) {
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
  %8 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %8, align 4, !range !0
  %9 = add i32 %width, -1
  %10 = add nuw nsw i32 %columns, %9
  %11 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %11, align 4, !range !0
  %12 = add i32 %height, -1
  %13 = add nuw nsw i32 %rows, %12
  %14 = call %u0Matrix* @likely_new(i32 25104, i32 1, i32 %10, i32 %13, i32 1, i8* null)
  %15 = zext i32 %13 to i64
  %mat_y_step = zext i32 %10 to i64
  %16 = getelementptr inbounds %u0Matrix, %u0Matrix* %14, i64 1
  %scevgep6 = getelementptr %u0Matrix, %u0Matrix* %14, i64 1, i32 0
  %scevgep67 = bitcast i32* %scevgep6 to i8*
  %17 = shl nuw nsw i64 %mat_y_step, 1
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %18 = mul i64 %y, %17
  %uglygep8 = getelementptr i8, i8* %scevgep67, i64 %18
  call void @llvm.memset.p0i8.i64(i8* %uglygep8, i8 0, i64 %17, i32 2, i1 false)
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %15
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %19 = bitcast %u0Matrix* %16 to i16*
  %pad-columns = sdiv i32 %9, 2
  %pad-rows = sdiv i32 %12, 2
  %20 = zext i32 %rows to i64
  %src_y_step = zext i32 %columns to i64
  %21 = sext i32 %pad-rows to i64
  %22 = mul nsw i64 %mat_y_step, %21
  %23 = sext i32 %pad-columns to i64
  %24 = add i64 %22, %23
  %25 = mul i64 %24, 2
  %scevgep3 = getelementptr %u16Matrix, %u16Matrix* %0, i64 1, i32 0
  %scevgep34 = bitcast i32* %scevgep3 to i8*
  %26 = shl nuw nsw i64 %src_y_step, 1
  br label %y_body9

y_body9:                                          ; preds = %y_body9, %y_exit
  %y11 = phi i64 [ 0, %y_exit ], [ %y_increment17, %y_body9 ]
  %27 = mul i64 %y11, %17
  %28 = add i64 %27, %25
  %uglygep = getelementptr i8, i8* %scevgep67, i64 %28
  %29 = mul i64 %y11, %26
  %uglygep5 = getelementptr i8, i8* %scevgep34, i64 %29
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %uglygep, i8* %uglygep5, i64 %26, i32 2, i1 false)
  %y_increment17 = add nuw nsw i64 %y11, 1
  %y_postcondition18 = icmp eq i64 %y_increment17, %20
  br i1 %y_postcondition18, label %y_exit10, label %y_body9

y_exit10:                                         ; preds = %y_body9
  %30 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %31 = getelementptr inbounds %u0Matrix, %u0Matrix* %30, i64 1
  %32 = bitcast %u0Matrix* %31 to float*
  %kernel_y_step = zext i32 %width to i64
  br label %y_body31

y_body31:                                         ; preds = %x_exit35, %y_exit10
  %y33 = phi i64 [ 0, %y_exit10 ], [ %y_increment43, %x_exit35 ]
  %33 = mul nuw nsw i64 %y33, %src_y_step
  br label %x_body34

x_body34:                                         ; preds = %y_body31, %exit
  %x36 = phi i64 [ %x_increment41, %exit ], [ 0, %y_body31 ]
  %34 = add nuw nsw i64 %x36, %33
  br label %loop38.preheader

loop38.preheader:                                 ; preds = %x_body34, %exit40
  %35 = phi i32 [ %58, %exit40 ], [ 0, %x_body34 ]
  %36 = phi float [ %55, %exit40 ], [ 0.000000e+00, %x_body34 ]
  %37 = sext i32 %35 to i64
  %38 = add nuw nsw i64 %37, %y33
  %39 = mul nuw nsw i64 %38, %mat_y_step
  %40 = add i64 %39, %x36
  %41 = mul nuw nsw i64 %37, %kernel_y_step
  br label %true_entry39

exit:                                             ; preds = %exit40
  %42 = getelementptr float, float* %32, i64 %34
  store float %55, float* %42, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment41 = add nuw nsw i64 %x36, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %src_y_step
  br i1 %x_postcondition42, label %x_exit35, label %x_body34

x_exit35:                                         ; preds = %exit
  %y_increment43 = add nuw nsw i64 %y33, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %20
  br i1 %y_postcondition44, label %y_exit32, label %y_body31

y_exit32:                                         ; preds = %x_exit35
  %dst = bitcast %u0Matrix* %30 to %f32Matrix*
  %43 = bitcast %u0Matrix* %14 to i8*
  call void @likely_release_mat(i8* %43)
  ret %f32Matrix* %dst

true_entry39:                                     ; preds = %loop38.preheader, %true_entry39
  %44 = phi float [ %55, %true_entry39 ], [ %36, %loop38.preheader ]
  %45 = phi i32 [ %56, %true_entry39 ], [ 0, %loop38.preheader ]
  %46 = sext i32 %45 to i64
  %47 = add i64 %40, %46
  %48 = getelementptr i16, i16* %19, i64 %47
  %49 = load i16, i16* %48, align 2, !llvm.mem.parallel_loop_access !1
  %50 = add nuw nsw i64 %46, %41
  %51 = getelementptr %f32Matrix, %f32Matrix* %1, i64 0, i32 6, i64 %50
  %52 = load float, float* %51, align 4, !llvm.mem.parallel_loop_access !1
  %53 = sitofp i16 %49 to float
  %54 = fmul fast float %53, %52
  %55 = fadd fast float %54, %44
  %56 = add nuw nsw i32 %45, 1
  %57 = icmp eq i32 %56, %width
  br i1 %57, label %exit40, label %true_entry39

exit40:                                           ; preds = %true_entry39
  %58 = add nuw nsw i32 %35, 1
  %59 = icmp eq i32 %58, %height
  br i1 %59, label %exit, label %loop38.preheader
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
