; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%f32X = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f32XY* @multiply_transposed(%f32XY*, %f32X*) {
entry:
  %2 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %5 = zext i32 %rows to i64
  %mat_y_step = zext i32 %columns to i64
  %6 = getelementptr inbounds %u0CXYT, %u0CXYT* %4, i64 1
  %7 = bitcast %u0CXYT* %6 to float*
  %8 = ptrtoint %u0CXYT* %6 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  %11 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 6, i64 0
  %12 = ptrtoint float* %11 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  call void @llvm.assume(i1 %14)
  %scevgep = getelementptr %u0CXYT, %u0CXYT* %4, i64 1, i32 0
  %scevgep3 = getelementptr %f32XY, %f32XY* %0, i64 1, i32 0
  %15 = shl nuw nsw i64 %mat_y_step, 2
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %16 = mul i64 %y, %mat_y_step
  %scevgep1 = getelementptr i32, i32* %scevgep, i64 %16
  %scevgep12 = bitcast i32* %scevgep1 to i8*
  %scevgep4 = getelementptr i32, i32* %scevgep3, i64 %16
  %scevgep45 = bitcast i32* %scevgep4 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep12, i8* %scevgep45, i64 %15, i32 4, i1 false)
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %5
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %17 = getelementptr inbounds %f32X, %f32X* %1, i64 0, i32 6, i64 0
  %18 = ptrtoint float* %17 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  br label %y_body15

y_body15:                                         ; preds = %x_exit19, %y_exit
  %y17 = phi i64 [ 0, %y_exit ], [ %y_increment23, %x_exit19 ]
  %21 = mul nuw nsw i64 %y17, %mat_y_step
  br label %x_body18

x_body18:                                         ; preds = %y_body15, %x_body18
  %x20 = phi i64 [ %x_increment21, %x_body18 ], [ 0, %y_body15 ]
  %22 = add nuw nsw i64 %x20, %21
  %23 = getelementptr float, float* %7, i64 %22
  %24 = load float, float* %23, align 4, !llvm.mem.parallel_loop_access !1
  %25 = getelementptr %f32X, %f32X* %1, i64 0, i32 6, i64 %x20
  %26 = load float, float* %25, align 4, !llvm.mem.parallel_loop_access !1
  %27 = fsub fast float %24, %26
  store float %27, float* %23, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment21 = add nuw nsw i64 %x20, 1
  %x_postcondition22 = icmp eq i64 %x_increment21, %mat_y_step
  br i1 %x_postcondition22, label %x_exit19, label %x_body18

x_exit19:                                         ; preds = %x_body18
  %y_increment23 = add nuw nsw i64 %y17, 1
  %y_postcondition24 = icmp eq i64 %y_increment23, %5
  br i1 %y_postcondition24, label %y_exit16, label %y_body15

y_exit16:                                         ; preds = %x_exit19
  %28 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %29 = getelementptr inbounds %u0CXYT, %u0CXYT* %28, i64 1
  %30 = bitcast %u0CXYT* %29 to float*
  %31 = ptrtoint %u0CXYT* %29 to i64
  %32 = and i64 %31, 31
  %33 = icmp eq i64 %32, 0
  call void @llvm.assume(i1 %33)
  br label %y_body33

y_body33:                                         ; preds = %x_exit37, %y_exit16
  %y35 = phi i64 [ 0, %y_exit16 ], [ %y_increment43, %x_exit37 ]
  %34 = mul nuw nsw i64 %y35, %mat_y_step
  br label %x_body36

x_body36:                                         ; preds = %y_body33, %Flow
  %x38 = phi i64 [ %x_increment41, %Flow ], [ 0, %y_body33 ]
  %35 = icmp ugt i64 %y35, %x38
  br i1 %35, label %Flow, label %true_entry39

x_exit37:                                         ; preds = %Flow
  %y_increment43 = add nuw nsw i64 %y35, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %mat_y_step
  br i1 %y_postcondition44, label %y_exit34, label %y_body33

y_exit34:                                         ; preds = %x_exit37
  %dst = bitcast %u0CXYT* %28 to %f32XY*
  %36 = bitcast %u0CXYT* %4 to i8*
  call void @likely_release_mat(i8* %36)
  ret %f32XY* %dst

true_entry39:                                     ; preds = %x_body36, %true_entry39
  %37 = phi i32 [ %51, %true_entry39 ], [ 0, %x_body36 ]
  %38 = phi double [ %50, %true_entry39 ], [ 0.000000e+00, %x_body36 ]
  %39 = sext i32 %37 to i64
  %40 = mul nuw nsw i64 %39, %mat_y_step
  %41 = add nuw nsw i64 %40, %x38
  %42 = getelementptr float, float* %7, i64 %41
  %43 = load float, float* %42, align 4, !llvm.mem.parallel_loop_access !2
  %44 = fpext float %43 to double
  %45 = add nuw nsw i64 %40, %y35
  %46 = getelementptr float, float* %7, i64 %45
  %47 = load float, float* %46, align 4, !llvm.mem.parallel_loop_access !2
  %48 = fpext float %47 to double
  %49 = fmul fast double %48, %44
  %50 = fadd fast double %49, %38
  %51 = add nuw nsw i32 %37, 1
  %52 = icmp eq i32 %51, %rows
  br i1 %52, label %exit40, label %true_entry39

Flow:                                             ; preds = %x_body36, %exit40
  %x_increment41 = add nuw nsw i64 %x38, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %mat_y_step
  br i1 %x_postcondition42, label %x_exit37, label %x_body36

exit40:                                           ; preds = %true_entry39
  %53 = add nuw nsw i64 %x38, %34
  %54 = getelementptr float, float* %30, i64 %53
  %55 = fptrunc double %50 to float
  store float %55, float* %54, align 4, !llvm.mem.parallel_loop_access !2
  %56 = mul nuw nsw i64 %x38, %mat_y_step
  %57 = add nuw nsw i64 %56, %y35
  %58 = getelementptr float, float* %30, i64 %57
  store float %55, float* %58, align 4, !llvm.mem.parallel_loop_access !2
  br label %Flow
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1) #0

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
