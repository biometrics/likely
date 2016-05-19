; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

define %f32XY* @filter_2D(%f32XY*, %f32XY*) {
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
  %18 = ptrtoint %u0CXYT* %16 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  %scevgep7 = getelementptr %u0CXYT, %u0CXYT* %14, i64 1, i32 0
  %21 = add i32 %width, -1
  %22 = add i32 %21, %columns
  %23 = zext i32 %22 to i64
  %24 = shl nuw nsw i64 %23, 2
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %25 = mul i64 %y, %23
  %scevgep8 = getelementptr i32, i32* %scevgep7, i64 %25
  %scevgep89 = bitcast i32* %scevgep8 to i8*
  call void @llvm.memset.p0i8.i64(i8* %scevgep89, i8 0, i64 %24, i32 4, i1 false)
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %15
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %pad-columns = sdiv i32 %9, 2
  %pad-rows = sdiv i32 %12, 2
  %26 = zext i32 %rows to i64
  %src_y_step = zext i32 %columns to i64
  %27 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 6, i64 0
  %28 = ptrtoint float* %27 to i64
  %29 = and i64 %28, 31
  %30 = icmp eq i64 %29, 0
  call void @llvm.assume(i1 %30)
  %31 = sext i32 %pad-rows to i64
  %32 = sext i32 %pad-columns to i64
  %scevgep4 = getelementptr %f32XY, %f32XY* %0, i64 1, i32 0
  %33 = shl nuw nsw i64 %src_y_step, 2
  br label %y_body9

y_body9:                                          ; preds = %y_body9, %y_exit
  %y11 = phi i64 [ 0, %y_exit ], [ %y_increment17, %y_body9 ]
  %tmp = add i64 %y11, %31
  %tmp10 = mul i64 %tmp, %23
  %34 = add i64 %tmp10, %32
  %scevgep2 = getelementptr i32, i32* %scevgep7, i64 %34
  %scevgep23 = bitcast i32* %scevgep2 to i8*
  %35 = mul i64 %y11, %src_y_step
  %scevgep5 = getelementptr i32, i32* %scevgep4, i64 %35
  %scevgep56 = bitcast i32* %scevgep5 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep23, i8* %scevgep56, i64 %33, i32 4, i1 false)
  %y_increment17 = add nuw nsw i64 %y11, 1
  %y_postcondition18 = icmp eq i64 %y_increment17, %26
  br i1 %y_postcondition18, label %y_exit10, label %y_body9

y_exit10:                                         ; preds = %y_body9
  %36 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %37 = getelementptr inbounds %u0CXYT, %u0CXYT* %36, i64 1
  %38 = bitcast %u0CXYT* %37 to float*
  %39 = ptrtoint %u0CXYT* %37 to i64
  %40 = and i64 %39, 31
  %41 = icmp eq i64 %40, 0
  call void @llvm.assume(i1 %41)
  %kernel_y_step = zext i32 %width to i64
  %42 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 6, i64 0
  %43 = ptrtoint float* %42 to i64
  %44 = and i64 %43, 31
  %45 = icmp eq i64 %44, 0
  call void @llvm.assume(i1 %45)
  br label %y_body31

y_body31:                                         ; preds = %x_exit35, %y_exit10
  %y33 = phi i64 [ 0, %y_exit10 ], [ %y_increment43, %x_exit35 ]
  %46 = mul nuw nsw i64 %y33, %src_y_step
  br label %x_body34

x_body34:                                         ; preds = %y_body31, %exit
  %x36 = phi i64 [ %x_increment41, %exit ], [ 0, %y_body31 ]
  %47 = add nuw nsw i64 %x36, %46
  br label %loop38.preheader

loop38.preheader:                                 ; preds = %x_body34, %exit40
  %48 = phi i32 [ %70, %exit40 ], [ 0, %x_body34 ]
  %49 = phi float [ %67, %exit40 ], [ 0.000000e+00, %x_body34 ]
  %50 = sext i32 %48 to i64
  %51 = add nuw nsw i64 %50, %y33
  %52 = mul nuw nsw i64 %51, %mat_y_step
  %53 = add i64 %52, %x36
  %54 = mul nuw nsw i64 %50, %kernel_y_step
  br label %true_entry39

exit:                                             ; preds = %exit40
  %55 = getelementptr float, float* %38, i64 %47
  store float %67, float* %55, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment41 = add nuw nsw i64 %x36, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %src_y_step
  br i1 %x_postcondition42, label %x_exit35, label %x_body34

x_exit35:                                         ; preds = %exit
  %y_increment43 = add nuw nsw i64 %y33, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %26
  br i1 %y_postcondition44, label %y_exit32, label %y_body31

y_exit32:                                         ; preds = %x_exit35
  %dst = bitcast %u0CXYT* %36 to %f32XY*
  %56 = bitcast %u0CXYT* %14 to i8*
  call void @likely_release_mat(i8* %56)
  ret %f32XY* %dst

true_entry39:                                     ; preds = %loop38.preheader, %true_entry39
  %57 = phi float [ %67, %true_entry39 ], [ %49, %loop38.preheader ]
  %58 = phi i32 [ %68, %true_entry39 ], [ 0, %loop38.preheader ]
  %59 = sext i32 %58 to i64
  %60 = add i64 %53, %59
  %61 = getelementptr float, float* %17, i64 %60
  %62 = load float, float* %61, align 4, !llvm.mem.parallel_loop_access !1
  %63 = add nuw nsw i64 %59, %54
  %64 = getelementptr %f32XY, %f32XY* %1, i64 0, i32 6, i64 %63
  %65 = load float, float* %64, align 4, !llvm.mem.parallel_loop_access !1
  %66 = fmul fast float %65, %62
  %67 = fadd fast float %66, %57
  %68 = add nuw nsw i32 %58, 1
  %69 = icmp eq i32 %68, %width
  br i1 %69, label %exit40, label %true_entry39

exit40:                                           ; preds = %true_entry39
  %70 = add nuw nsw i32 %48, 1
  %71 = icmp eq i32 %70, %height
  br i1 %71, label %exit, label %loop38.preheader
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
