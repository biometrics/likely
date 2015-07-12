; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%f32X = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readonly
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
  %6 = getelementptr inbounds %u0CXYT, %u0CXYT* %4, i64 1
  %7 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 6, i64 0
  %8 = ptrtoint float* %7 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  %scevgep = getelementptr %u0CXYT, %u0CXYT* %4, i64 1, i32 0
  %11 = zext i32 %columns to i64
  %scevgep3 = getelementptr %f32XY, %f32XY* %0, i64 1, i32 0
  %12 = shl nuw nsw i64 %11, 2
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %13 = mul i64 %y, %11
  %scevgep1 = getelementptr i32, i32* %scevgep, i64 %13
  %scevgep12 = bitcast i32* %scevgep1 to i8*
  %scevgep4 = getelementptr i32, i32* %scevgep3, i64 %13
  %scevgep45 = bitcast i32* %scevgep4 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep12, i8* %scevgep45, i64 %12, i32 4, i1 false)
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %5
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %14 = bitcast %u0CXYT* %6 to float*
  %15 = getelementptr inbounds %f32X, %f32X* %1, i64 0, i32 6, i64 0
  %16 = ptrtoint float* %15 to i64
  %17 = and i64 %16, 31
  %18 = icmp eq i64 %17, 0
  call void @llvm.assume(i1 %18)
  br label %y_body15

y_body15:                                         ; preds = %x_exit19, %y_exit
  %y17 = phi i64 [ 0, %y_exit ], [ %y_increment23, %x_exit19 ]
  %19 = mul nuw nsw i64 %y17, %11
  br label %x_body18

x_body18:                                         ; preds = %y_body15, %x_body18
  %x20 = phi i64 [ %x_increment21, %x_body18 ], [ 0, %y_body15 ]
  %20 = add nuw nsw i64 %x20, %19
  %21 = getelementptr float, float* %14, i64 %20
  %22 = load float, float* %21, align 4, !llvm.mem.parallel_loop_access !1
  %23 = getelementptr %f32X, %f32X* %1, i64 0, i32 6, i64 %x20
  %24 = load float, float* %23, align 4, !llvm.mem.parallel_loop_access !1
  %25 = fsub fast float %22, %24
  store float %25, float* %21, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment21 = add nuw nsw i64 %x20, 1
  %x_postcondition22 = icmp eq i64 %x_increment21, %11
  br i1 %x_postcondition22, label %x_exit19, label %x_body18

x_exit19:                                         ; preds = %x_body18
  %y_increment23 = add nuw nsw i64 %y17, 1
  %y_postcondition24 = icmp eq i64 %y_increment23, %5
  br i1 %y_postcondition24, label %y_exit16, label %y_body15

y_exit16:                                         ; preds = %x_exit19
  %26 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %27 = getelementptr inbounds %u0CXYT, %u0CXYT* %26, i64 1
  %28 = bitcast %u0CXYT* %27 to float*
  %29 = ptrtoint %u0CXYT* %27 to i64
  %30 = and i64 %29, 31
  %31 = icmp eq i64 %30, 0
  call void @llvm.assume(i1 %31)
  br label %y_body33

y_body33:                                         ; preds = %x_exit37, %y_exit16
  %y35 = phi i64 [ 0, %y_exit16 ], [ %y_increment43, %x_exit37 ]
  %32 = mul nuw nsw i64 %y35, %11
  br label %x_body36

x_body36:                                         ; preds = %y_body33, %Flow
  %x38 = phi i64 [ %x_increment41, %Flow ], [ 0, %y_body33 ]
  %33 = icmp ugt i64 %y35, %x38
  br i1 %33, label %Flow, label %true_entry39

x_exit37:                                         ; preds = %Flow
  %y_increment43 = add nuw nsw i64 %y35, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %11
  br i1 %y_postcondition44, label %y_exit34, label %y_body33

y_exit34:                                         ; preds = %x_exit37
  %dst = bitcast %u0CXYT* %26 to %f32XY*
  %34 = bitcast %u0CXYT* %4 to i8*
  call void @likely_release_mat(i8* %34)
  ret %f32XY* %dst

true_entry39:                                     ; preds = %x_body36, %true_entry39
  %35 = phi i32 [ %49, %true_entry39 ], [ 0, %x_body36 ]
  %36 = phi double [ %48, %true_entry39 ], [ 0.000000e+00, %x_body36 ]
  %37 = sext i32 %35 to i64
  %38 = mul nuw nsw i64 %37, %11
  %39 = add nuw nsw i64 %38, %x38
  %40 = getelementptr float, float* %14, i64 %39
  %41 = load float, float* %40, align 4, !llvm.mem.parallel_loop_access !2
  %42 = fpext float %41 to double
  %43 = add nuw nsw i64 %38, %y35
  %44 = getelementptr float, float* %14, i64 %43
  %45 = load float, float* %44, align 4, !llvm.mem.parallel_loop_access !2
  %46 = fpext float %45 to double
  %47 = fmul fast double %46, %42
  %48 = fadd fast double %47, %36
  %49 = add nuw nsw i32 %35, 1
  %50 = icmp eq i32 %49, %rows
  br i1 %50, label %exit40, label %true_entry39

Flow:                                             ; preds = %x_body36, %exit40
  %x_increment41 = add nuw nsw i64 %x38, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %11
  br i1 %x_postcondition42, label %x_exit37, label %x_body36

exit40:                                           ; preds = %true_entry39
  %51 = add nuw nsw i64 %x38, %32
  %52 = getelementptr float, float* %28, i64 %51
  %53 = fptrunc double %48 to float
  store float %53, float* %52, align 4, !llvm.mem.parallel_loop_access !2
  %54 = mul nuw nsw i64 %x38, %11
  %55 = add nuw nsw i64 %54, %y35
  %56 = getelementptr float, float* %28, i64 %55
  store float %53, float* %56, align 4, !llvm.mem.parallel_loop_access !2
  br label %Flow
}

; Function Attrs: nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1) #1

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
