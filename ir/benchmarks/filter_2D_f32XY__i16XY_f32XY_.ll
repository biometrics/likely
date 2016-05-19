; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%i16XY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

define %f32XY* @filter_2D(%i16XY*, %f32XY*) {
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
  %18 = ptrtoint %u0CXYT* %16 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  %scevgep6 = getelementptr %u0CXYT, %u0CXYT* %14, i64 1, i32 0
  %scevgep67 = bitcast i32* %scevgep6 to i8*
  %21 = add i32 %width, -1
  %22 = add i32 %21, %columns
  %23 = zext i32 %22 to i64
  %24 = shl nuw nsw i64 %23, 1
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %25 = mul i64 %y, %24
  %uglygep8 = getelementptr i8, i8* %scevgep67, i64 %25
  call void @llvm.memset.p0i8.i64(i8* %uglygep8, i8 0, i64 %24, i32 2, i1 false)
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %15
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %pad-columns = sdiv i32 %9, 2
  %pad-rows = sdiv i32 %12, 2
  %26 = zext i32 %rows to i64
  %src_y_step = zext i32 %columns to i64
  %27 = getelementptr inbounds %i16XY, %i16XY* %0, i64 0, i32 6, i64 0
  %28 = ptrtoint i16* %27 to i64
  %29 = and i64 %28, 31
  %30 = icmp eq i64 %29, 0
  call void @llvm.assume(i1 %30)
  %31 = sext i32 %pad-rows to i64
  %32 = mul nsw i64 %23, %31
  %33 = sext i32 %pad-columns to i64
  %34 = add i64 %32, %33
  %35 = mul i64 %34, 2
  %scevgep3 = getelementptr %i16XY, %i16XY* %0, i64 1, i32 0
  %scevgep34 = bitcast i32* %scevgep3 to i8*
  %36 = shl nuw nsw i64 %src_y_step, 1
  br label %y_body9

y_body9:                                          ; preds = %y_body9, %y_exit
  %y11 = phi i64 [ 0, %y_exit ], [ %y_increment17, %y_body9 ]
  %37 = mul i64 %y11, %24
  %38 = add i64 %37, %35
  %uglygep = getelementptr i8, i8* %scevgep67, i64 %38
  %39 = mul i64 %y11, %36
  %uglygep5 = getelementptr i8, i8* %scevgep34, i64 %39
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %uglygep, i8* %uglygep5, i64 %36, i32 2, i1 false)
  %y_increment17 = add nuw nsw i64 %y11, 1
  %y_postcondition18 = icmp eq i64 %y_increment17, %26
  br i1 %y_postcondition18, label %y_exit10, label %y_body9

y_exit10:                                         ; preds = %y_body9
  %40 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %41 = getelementptr inbounds %u0CXYT, %u0CXYT* %40, i64 1
  %42 = bitcast %u0CXYT* %41 to float*
  %43 = ptrtoint %u0CXYT* %41 to i64
  %44 = and i64 %43, 31
  %45 = icmp eq i64 %44, 0
  call void @llvm.assume(i1 %45)
  %kernel_y_step = zext i32 %width to i64
  %46 = getelementptr inbounds %f32XY, %f32XY* %1, i64 0, i32 6, i64 0
  %47 = ptrtoint float* %46 to i64
  %48 = and i64 %47, 31
  %49 = icmp eq i64 %48, 0
  call void @llvm.assume(i1 %49)
  br label %y_body31

y_body31:                                         ; preds = %x_exit35, %y_exit10
  %y33 = phi i64 [ 0, %y_exit10 ], [ %y_increment43, %x_exit35 ]
  %50 = mul nuw nsw i64 %y33, %src_y_step
  br label %x_body34

x_body34:                                         ; preds = %y_body31, %exit
  %x36 = phi i64 [ %x_increment41, %exit ], [ 0, %y_body31 ]
  %51 = add nuw nsw i64 %x36, %50
  br label %loop38.preheader

loop38.preheader:                                 ; preds = %x_body34, %exit40
  %52 = phi i32 [ %75, %exit40 ], [ 0, %x_body34 ]
  %53 = phi float [ %72, %exit40 ], [ 0.000000e+00, %x_body34 ]
  %54 = sext i32 %52 to i64
  %55 = add nuw nsw i64 %54, %y33
  %56 = mul nuw nsw i64 %55, %mat_y_step
  %57 = add i64 %56, %x36
  %58 = mul nuw nsw i64 %54, %kernel_y_step
  br label %true_entry39

exit:                                             ; preds = %exit40
  %59 = getelementptr float, float* %42, i64 %51
  store float %72, float* %59, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment41 = add nuw nsw i64 %x36, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %src_y_step
  br i1 %x_postcondition42, label %x_exit35, label %x_body34

x_exit35:                                         ; preds = %exit
  %y_increment43 = add nuw nsw i64 %y33, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %26
  br i1 %y_postcondition44, label %y_exit32, label %y_body31

y_exit32:                                         ; preds = %x_exit35
  %dst = bitcast %u0CXYT* %40 to %f32XY*
  %60 = bitcast %u0CXYT* %14 to i8*
  call void @likely_release_mat(i8* %60)
  ret %f32XY* %dst

true_entry39:                                     ; preds = %loop38.preheader, %true_entry39
  %61 = phi float [ %72, %true_entry39 ], [ %53, %loop38.preheader ]
  %62 = phi i32 [ %73, %true_entry39 ], [ 0, %loop38.preheader ]
  %63 = sext i32 %62 to i64
  %64 = add i64 %57, %63
  %65 = getelementptr i16, i16* %17, i64 %64
  %66 = load i16, i16* %65, align 2, !llvm.mem.parallel_loop_access !1
  %67 = add nuw nsw i64 %63, %58
  %68 = getelementptr %f32XY, %f32XY* %1, i64 0, i32 6, i64 %67
  %69 = load float, float* %68, align 4, !llvm.mem.parallel_loop_access !1
  %70 = sitofp i16 %66 to float
  %71 = fmul fast float %70, %69
  %72 = fadd fast float %71, %61
  %73 = add nuw nsw i32 %62, 1
  %74 = icmp eq i32 %73, %width
  br i1 %74, label %exit40, label %true_entry39

exit40:                                           ; preds = %true_entry39
  %75 = add nuw nsw i32 %52, 1
  %76 = icmp eq i32 %75, %height
  br i1 %76, label %exit, label %loop38.preheader
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
