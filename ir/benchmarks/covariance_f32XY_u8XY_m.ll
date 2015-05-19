; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32X = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%u8XY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @covariance_tmp_thunk0({ %f32X* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32X* }, { %f32X* }* %0, i64 0, i32 0
  %4 = load %f32X*, %f32X** %3, align 8
  %5 = getelementptr inbounds %f32X, %f32X* %4, i64 0, i32 6, i64 0
  %6 = ptrtoint float* %5 to i64
  %7 = and i64 %6, 31
  %8 = icmp eq i64 %7, 0
  call void @llvm.assume(i1 %8)
  %scevgep = getelementptr %f32X, %f32X* %4, i64 1, i32 0
  %scevgep1 = getelementptr i32, i32* %scevgep, i64 %1
  %scevgep12 = bitcast i32* %scevgep1 to i8*
  %9 = sub i64 %2, %1
  %10 = shl i64 %9, 2
  call void @llvm.memset.p0i8.i64(i8* %scevgep12, i8 0, i64 %10, i32 4, i1 false)
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: nounwind
define private void @covariance_tmp_thunk1({ %f32X*, float }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32X*, float }, { %f32X*, float }* %0, i64 0, i32 0
  %4 = load %f32X*, %f32X** %3, align 8
  %5 = getelementptr inbounds { %f32X*, float }, { %f32X*, float }* %0, i64 0, i32 1
  %6 = load float, float* %5, align 4
  %7 = getelementptr inbounds %f32X, %f32X* %4, i64 0, i32 6, i64 0
  %8 = ptrtoint float* %7 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %11 = getelementptr %f32X, %f32X* %4, i64 0, i32 6, i64 %x
  %12 = load float, float* %11, align 4, !llvm.mem.parallel_loop_access !0
  %13 = fmul fast float %12, %6
  store float %13, float* %11, align 4, !llvm.mem.parallel_loop_access !0
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  ret void
}

; Function Attrs: nounwind
define private void @covariance_tmp_thunk2({ %f32XY*, %u8XY*, %f32X* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32XY*, %u8XY*, %f32X* }, { %f32XY*, %u8XY*, %f32X* }* %0, i64 0, i32 0
  %4 = load %f32XY*, %f32XY** %3, align 8
  %5 = getelementptr inbounds { %f32XY*, %u8XY*, %f32X* }, { %f32XY*, %u8XY*, %f32X* }* %0, i64 0, i32 1
  %6 = load %u8XY*, %u8XY** %5, align 8
  %7 = getelementptr inbounds { %f32XY*, %u8XY*, %f32X* }, { %f32XY*, %u8XY*, %f32X* }* %0, i64 0, i32 2
  %8 = load %f32X*, %f32X** %7, align 8
  %9 = getelementptr inbounds %u8XY, %u8XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %9, align 4, !range !1
  %centered_y_step = zext i32 %columns1 to i64
  %10 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 6, i64 0
  %11 = ptrtoint float* %10 to i64
  %12 = and i64 %11, 31
  %13 = icmp eq i64 %12, 0
  call void @llvm.assume(i1 %13)
  %14 = getelementptr inbounds %u8XY, %u8XY* %6, i64 0, i32 6, i64 0
  %15 = ptrtoint i8* %14 to i64
  %16 = and i64 %15, 31
  %17 = icmp eq i64 %16, 0
  call void @llvm.assume(i1 %17)
  %18 = getelementptr inbounds %f32X, %f32X* %8, i64 0, i32 6, i64 0
  %19 = ptrtoint float* %18 to i64
  %20 = and i64 %19, 31
  %21 = icmp eq i64 %20, 0
  call void @llvm.assume(i1 %21)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %22 = mul nuw nsw i64 %y, %centered_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %23 = add nuw nsw i64 %x, %22
  %24 = getelementptr %u8XY, %u8XY* %6, i64 0, i32 6, i64 %23
  %25 = load i8, i8* %24, align 1, !llvm.mem.parallel_loop_access !2
  %26 = getelementptr %f32X, %f32X* %8, i64 0, i32 6, i64 %x
  %27 = load float, float* %26, align 4, !llvm.mem.parallel_loop_access !2
  %28 = uitofp i8 %25 to float
  %29 = fsub fast float %28, %27
  %30 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %23
  store float %29, float* %30, align 4, !llvm.mem.parallel_loop_access !2
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %centered_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

; Function Attrs: nounwind
define private void @covariance_tmp_thunk3({ %f32XY*, %f32XY*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %0, i64 0, i32 0
  %4 = load %f32XY*, %f32XY** %3, align 8
  %5 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %0, i64 0, i32 1
  %6 = load %f32XY*, %f32XY** %5, align 8
  %7 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %0, i64 0, i32 2
  %8 = load i32, i32* %7, align 4
  %9 = getelementptr inbounds %f32XY, %f32XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %9, align 4, !range !1
  %dst_y_step = zext i32 %columns1 to i64
  %10 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 6, i64 0
  %11 = ptrtoint float* %10 to i64
  %12 = and i64 %11, 31
  %13 = icmp eq i64 %12, 0
  call void @llvm.assume(i1 %13)
  %14 = getelementptr inbounds %f32XY, %f32XY* %6, i64 0, i32 6, i64 0
  %15 = ptrtoint float* %14 to i64
  %16 = and i64 %15, 31
  %17 = icmp eq i64 %16, 0
  call void @llvm.assume(i1 %17)
  %18 = icmp eq i32 %8, 0
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %19 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %Flow6
  %x = phi i64 [ %x_increment, %Flow6 ], [ 0, %y_body ]
  %20 = icmp ugt i64 %y, %x
  br i1 %20, label %Flow6, label %label.preheader

label.preheader:                                  ; preds = %x_body
  br i1 %18, label %exit4, label %true_entry3

x_exit:                                           ; preds = %Flow6
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void

true_entry3:                                      ; preds = %label.preheader, %true_entry3
  %21 = phi i32 [ %35, %true_entry3 ], [ 0, %label.preheader ]
  %22 = phi double [ %34, %true_entry3 ], [ 0.000000e+00, %label.preheader ]
  %23 = sext i32 %21 to i64
  %24 = mul nuw nsw i64 %23, %dst_y_step
  %25 = add nuw nsw i64 %24, %x
  %26 = getelementptr %f32XY, %f32XY* %6, i64 0, i32 6, i64 %25
  %27 = load float, float* %26, align 4, !llvm.mem.parallel_loop_access !3
  %28 = fpext float %27 to double
  %29 = add nuw nsw i64 %24, %y
  %30 = getelementptr %f32XY, %f32XY* %6, i64 0, i32 6, i64 %29
  %31 = load float, float* %30, align 4, !llvm.mem.parallel_loop_access !3
  %32 = fpext float %31 to double
  %33 = fmul fast double %32, %28
  %34 = fadd fast double %33, %22
  %35 = add nuw nsw i32 %21, 1
  %36 = icmp eq i32 %35, %8
  br i1 %36, label %exit4, label %true_entry3

Flow6:                                            ; preds = %x_body, %exit4
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

exit4:                                            ; preds = %true_entry3, %label.preheader
  %.lcssa = phi double [ 0.000000e+00, %label.preheader ], [ %34, %true_entry3 ]
  %37 = add nuw nsw i64 %x, %19
  %38 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %37
  %39 = fptrunc double %.lcssa to float
  store float %39, float* %38, align 4, !llvm.mem.parallel_loop_access !3
  %40 = mul nuw nsw i64 %x, %dst_y_step
  %41 = add nuw nsw i64 %40, %y
  %42 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %41
  store float %39, float* %42, align 4, !llvm.mem.parallel_loop_access !3
  br label %Flow6
}

define %f32XY* @covariance(%u8XY*) {
entry:
  %1 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !1
  %2 = call %u0CXYT* @likely_new(i32 8480, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = zext i32 %columns to i64
  %4 = alloca { %f32X* }, align 8
  %5 = bitcast { %f32X* }* %4 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %5, align 8
  %6 = bitcast { %f32X* }* %4 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32X* }*, i64, i64)* @covariance_tmp_thunk0 to i8*), i8* %6, i64 %3)
  %7 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 4
  %rows = load i32, i32* %7, align 4, !range !1
  %8 = zext i32 %rows to i64
  %9 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %10 = bitcast %u0CXYT* %9 to float*
  %11 = ptrtoint %u0CXYT* %9 to i64
  %12 = and i64 %11, 31
  %13 = icmp eq i64 %12, 0
  call void @llvm.assume(i1 %13)
  %columns3 = load i32, i32* %1, align 4, !range !1
  %src_y_step = zext i32 %columns3 to i64
  %14 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 6, i64 0
  %15 = ptrtoint i8* %14 to i64
  %16 = and i64 %15, 31
  %17 = icmp eq i64 %16, 0
  call void @llvm.assume(i1 %17)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %18 = mul nuw nsw i64 %y, %src_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %19 = getelementptr float, float* %10, i64 %x
  %20 = load float, float* %19, align 4
  %21 = add nuw nsw i64 %x, %18
  %22 = getelementptr %u8XY, %u8XY* %0, i64 0, i32 6, i64 %21
  %23 = load i8, i8* %22, align 1
  %24 = uitofp i8 %23 to float
  %25 = fadd fast float %24, %20
  store float %25, float* %19, align 4
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %src_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %8
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %26 = uitofp i32 %rows to float
  %27 = fdiv fast float 1.000000e+00, %26
  %28 = alloca { %f32X*, float }, align 8
  %29 = bitcast { %f32X*, float }* %28 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %29, align 8
  %30 = getelementptr inbounds { %f32X*, float }, { %f32X*, float }* %28, i64 0, i32 1
  store float %27, float* %30, align 8
  %31 = bitcast { %f32X*, float }* %28 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32X*, float }*, i64, i64)* @covariance_tmp_thunk1 to i8*), i8* %31, i64 %3)
  %columns7 = load i32, i32* %1, align 4, !range !1
  %rows8 = load i32, i32* %7, align 4, !range !1
  %32 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns7, i32 %rows8, i32 1, i8* null)
  %33 = zext i32 %rows8 to i64
  %34 = alloca { %f32XY*, %u8XY*, %f32X* }, align 8
  %35 = bitcast { %f32XY*, %u8XY*, %f32X* }* %34 to %u0CXYT**
  store %u0CXYT* %32, %u0CXYT** %35, align 8
  %36 = getelementptr inbounds { %f32XY*, %u8XY*, %f32X* }, { %f32XY*, %u8XY*, %f32X* }* %34, i64 0, i32 1
  store %u8XY* %0, %u8XY** %36, align 8
  %37 = getelementptr inbounds { %f32XY*, %u8XY*, %f32X* }, { %f32XY*, %u8XY*, %f32X* }* %34, i64 0, i32 2
  %38 = bitcast %f32X** %37 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %38, align 8
  %39 = bitcast { %f32XY*, %u8XY*, %f32X* }* %34 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32XY*, %u8XY*, %f32X* }*, i64, i64)* @covariance_tmp_thunk2 to i8*), i8* %39, i64 %33)
  %40 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns7, i32 %columns7, i32 1, i8* null)
  %41 = bitcast %u0CXYT* %40 to %f32XY*
  %42 = zext i32 %columns7 to i64
  %43 = alloca { %f32XY*, %f32XY*, i32 }, align 8
  %44 = bitcast { %f32XY*, %f32XY*, i32 }* %43 to %u0CXYT**
  store %u0CXYT* %40, %u0CXYT** %44, align 8
  %45 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %43, i64 0, i32 1
  %46 = bitcast %f32XY** %45 to %u0CXYT**
  store %u0CXYT* %32, %u0CXYT** %46, align 8
  %47 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %43, i64 0, i32 2
  store i32 %rows8, i32* %47, align 8
  %48 = bitcast { %f32XY*, %f32XY*, i32 }* %43 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32XY*, %f32XY*, i32 }*, i64, i64)* @covariance_tmp_thunk3 to i8*), i8* %48, i64 %42)
  %49 = bitcast %u0CXYT* %2 to i8*
  call void @likely_release_mat(i8* %49)
  %50 = bitcast %u0CXYT* %32 to i8*
  call void @likely_release_mat(i8* %50)
  ret %f32XY* %41
}

; Function Attrs: nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #1

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = distinct !{!0}
!1 = !{i32 1, i32 -1}
!2 = distinct !{!2}
!3 = distinct !{!3}
