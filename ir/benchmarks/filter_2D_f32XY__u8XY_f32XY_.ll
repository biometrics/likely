; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8XY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

define %f32XY* @filter_2D(%u8XY*, %f32XY*) {
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
  %8 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 3
  %columns = load i32, i32* %8, align 4, !range !0
  %9 = add i32 %width, -1
  %10 = add nuw nsw i32 %columns, %9
  %11 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 4
  %rows = load i32, i32* %11, align 4, !range !0
  %12 = add i32 %height, -1
  %13 = add nuw nsw i32 %rows, %12
  %14 = call %u0CXYT* @likely_new(i32 24584, i32 1, i32 %10, i32 %13, i32 1, i8* null)
  %15 = zext i32 %13 to i64
  %mat_y_step = zext i32 %10 to i64
  %16 = getelementptr inbounds %u0CXYT, %u0CXYT* %14, i64 1
  %17 = bitcast %u0CXYT* %16 to i8*
  %18 = ptrtoint %u0CXYT* %16 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  %scevgep6 = getelementptr %u0CXYT, %u0CXYT* %14, i64 1, i32 0
  %scevgep67 = bitcast i32* %scevgep6 to i8*
  %21 = add i32 %width, -1
  %22 = add i32 %21, %columns
  %23 = zext i32 %22 to i64
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %24 = mul i64 %y, %23
  %uglygep8 = getelementptr i8, i8* %scevgep67, i64 %24
  call void @llvm.memset.p0i8.i64(i8* %uglygep8, i8 0, i64 %23, i32 1, i1 false)
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %15
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %pad-columns = sdiv i32 %9, 2
  %pad-rows = sdiv i32 %12, 2
  %25 = zext i32 %rows to i64
  %src_y_step = zext i32 %columns to i64
  %26 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 6, i64 0
  %27 = ptrtoint i8* %26 to i64
  %28 = and i64 %27, 31
  %29 = icmp eq i64 %28, 0
  call void @llvm.assume(i1 %29)
  %30 = sext i32 %pad-rows to i64
  %31 = sext i32 %pad-columns to i64
  %scevgep3 = getelementptr %u8XY, %u8XY* %0, i64 1, i32 0
  %scevgep34 = bitcast i32* %scevgep3 to i8*
  br label %y_body9

y_body9:                                          ; preds = %y_body9, %y_exit
  %y11 = phi i64 [ 0, %y_exit ], [ %y_increment17, %y_body9 ]
  %tmp = add i64 %y11, %30
  %tmp9 = mul i64 %tmp, %23
  %32 = add i64 %tmp9, %31
  %uglygep = getelementptr i8, i8* %scevgep67, i64 %32
  %33 = mul i64 %y11, %src_y_step
  %uglygep5 = getelementptr i8, i8* %scevgep34, i64 %33
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %uglygep, i8* %uglygep5, i64 %src_y_step, i32 1, i1 false)
  %y_increment17 = add nuw nsw i64 %y11, 1
  %y_postcondition18 = icmp eq i64 %y_increment17, %25
  br i1 %y_postcondition18, label %y_exit10, label %y_body9

y_exit10:                                         ; preds = %y_body9
  %34 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %35 = getelementptr inbounds %u0CXYT, %u0CXYT* %34, i64 1
  %36 = bitcast %u0CXYT* %35 to float*
  %37 = ptrtoint %u0CXYT* %35 to i64
  %38 = and i64 %37, 31
  %39 = icmp eq i64 %38, 0
  call void @llvm.assume(i1 %39)
  %kernel_y_step = zext i32 %width to i64
  %40 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 6, i64 0
  %41 = ptrtoint float* %40 to i64
  %42 = and i64 %41, 31
  %43 = icmp eq i64 %42, 0
  call void @llvm.assume(i1 %43)
  br label %y_body31

y_body31:                                         ; preds = %x_exit35, %y_exit10
  %y33 = phi i64 [ 0, %y_exit10 ], [ %y_increment43, %x_exit35 ]
  %44 = mul nuw nsw i64 %y33, %src_y_step
  br label %x_body34

x_body34:                                         ; preds = %y_body31, %exit
  %x36 = phi i64 [ %x_increment41, %exit ], [ 0, %y_body31 ]
  %45 = add nuw nsw i64 %x36, %44
  br label %loop38.preheader

loop38.preheader:                                 ; preds = %x_body34, %exit40
  %46 = phi i32 [ %69, %exit40 ], [ 0, %x_body34 ]
  %47 = phi float [ %66, %exit40 ], [ 0.000000e+00, %x_body34 ]
  %48 = sext i32 %46 to i64
  %49 = add nuw nsw i64 %48, %y33
  %50 = mul nuw nsw i64 %49, %mat_y_step
  %51 = add i64 %50, %x36
  %52 = mul nuw nsw i64 %48, %kernel_y_step
  br label %true_entry39

exit:                                             ; preds = %exit40
  %53 = getelementptr float, float* %36, i64 %45
  store float %66, float* %53, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment41 = add nuw nsw i64 %x36, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %src_y_step
  br i1 %x_postcondition42, label %x_exit35, label %x_body34

x_exit35:                                         ; preds = %exit
  %y_increment43 = add nuw nsw i64 %y33, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %25
  br i1 %y_postcondition44, label %y_exit32, label %y_body31

y_exit32:                                         ; preds = %x_exit35
  %dst = bitcast %u0CXYT* %34 to %f32XY*
  %54 = bitcast %u0CXYT* %14 to i8*
  call void @likely_release_mat(i8* %54)
  ret %f32XY* %dst

true_entry39:                                     ; preds = %loop38.preheader, %true_entry39
  %55 = phi float [ %66, %true_entry39 ], [ %47, %loop38.preheader ]
  %56 = phi i32 [ %67, %true_entry39 ], [ 0, %loop38.preheader ]
  %57 = sext i32 %56 to i64
  %58 = add i64 %51, %57
  %59 = getelementptr i8, i8* %17, i64 %58
  %60 = load i8, i8* %59, align 1, !llvm.mem.parallel_loop_access !1
  %61 = add nuw nsw i64 %57, %52
  %62 = getelementptr %f32XY, %f32XY* %1, i64 0, i32 6, i64 %61
  %63 = load float, float* %62, align 4, !llvm.mem.parallel_loop_access !1
  %64 = uitofp i8 %60 to float
  %65 = fmul fast float %64, %63
  %66 = fadd fast float %65, %55
  %67 = add nuw nsw i32 %56, 1
  %68 = icmp eq i32 %67, %width
  br i1 %68, label %exit40, label %true_entry39

exit40:                                           ; preds = %true_entry39
  %69 = add nuw nsw i32 %46, 1
  %70 = icmp eq i32 %69, %height
  br i1 %70, label %exit, label %loop38.preheader
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
