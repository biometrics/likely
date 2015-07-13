; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32SX = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%f32SXY = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%i16SXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: nounwind argmemonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @covariance_tmp_thunk0({ %f32SX*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32SX*, i32 }, { %f32SX*, i32 }* %0, i64 0, i32 0
  %4 = load %f32SX*, %f32SX** %3, align 8
  %5 = getelementptr inbounds { %f32SX*, i32 }, { %f32SX*, i32 }* %0, i64 0, i32 1
  %6 = load i32, i32* %5, align 4
  %7 = getelementptr inbounds %f32SX, %f32SX* %4, i64 0, i32 6, i64 0
  %8 = ptrtoint float* %7 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  %11 = sitofp i32 %6 to float
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %12 = getelementptr %f32SX, %f32SX* %4, i64 0, i32 6, i64 %x
  store float %11, float* %12, align 4, !llvm.mem.parallel_loop_access !0
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: nounwind
define private void @covariance_tmp_thunk1({ %f32SX*, float }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32SX*, float }, { %f32SX*, float }* %0, i64 0, i32 0
  %4 = load %f32SX*, %f32SX** %3, align 8
  %5 = getelementptr inbounds { %f32SX*, float }, { %f32SX*, float }* %0, i64 0, i32 1
  %6 = load float, float* %5, align 4
  %7 = getelementptr inbounds %f32SX, %f32SX* %4, i64 0, i32 6, i64 0
  %8 = ptrtoint float* %7 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %11 = getelementptr %f32SX, %f32SX* %4, i64 0, i32 6, i64 %x
  %12 = load float, float* %11, align 4, !llvm.mem.parallel_loop_access !1
  %13 = fmul fast float %12, %6
  store float %13, float* %11, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  ret void
}

; Function Attrs: nounwind
define private void @covariance_tmp_thunk2({ %f32SXY*, %i16SXY* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32SXY*, %i16SXY* }, { %f32SXY*, %i16SXY* }* %0, i64 0, i32 0
  %4 = load %f32SXY*, %f32SXY** %3, align 8
  %5 = getelementptr inbounds { %f32SXY*, %i16SXY* }, { %f32SXY*, %i16SXY* }* %0, i64 0, i32 1
  %6 = load %i16SXY*, %i16SXY** %5, align 8
  %7 = getelementptr inbounds %f32SXY, %f32SXY* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !2
  %mat_y_step = zext i32 %columns to i64
  %8 = getelementptr inbounds %f32SXY, %f32SXY* %4, i64 0, i32 6, i64 0
  %9 = ptrtoint float* %8 to i64
  %10 = and i64 %9, 31
  %11 = icmp eq i64 %10, 0
  call void @llvm.assume(i1 %11)
  %12 = getelementptr inbounds %i16SXY, %i16SXY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %12, align 4, !range !2
  %val_y_step = zext i32 %columns1 to i64
  %13 = getelementptr inbounds %i16SXY, %i16SXY* %6, i64 0, i32 6, i64 0
  %14 = ptrtoint i16* %13 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  call void @llvm.assume(i1 %16)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %17 = mul nuw nsw i64 %y, %val_y_step
  %18 = mul nuw nsw i64 %y, %mat_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %19 = add nuw nsw i64 %x, %17
  %20 = getelementptr %i16SXY, %i16SXY* %6, i64 0, i32 6, i64 %19
  %21 = load i16, i16* %20, align 2, !llvm.mem.parallel_loop_access !3
  %22 = add nuw nsw i64 %x, %18
  %23 = getelementptr %f32SXY, %f32SXY* %4, i64 0, i32 6, i64 %22
  %24 = sitofp i16 %21 to float
  store float %24, float* %23, align 4, !llvm.mem.parallel_loop_access !3
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %mat_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

; Function Attrs: nounwind
define private void @covariance_tmp_thunk3({ %f32SXY*, %f32SX* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32SXY*, %f32SX* }, { %f32SXY*, %f32SX* }* %0, i64 0, i32 0
  %4 = load %f32SXY*, %f32SXY** %3, align 8
  %5 = getelementptr inbounds { %f32SXY*, %f32SX* }, { %f32SXY*, %f32SX* }* %0, i64 0, i32 1
  %6 = load %f32SX*, %f32SX** %5, align 8
  %7 = getelementptr inbounds %f32SXY, %f32SXY* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !2
  %mat_y_step = zext i32 %columns to i64
  %8 = getelementptr inbounds %f32SXY, %f32SXY* %4, i64 0, i32 6, i64 0
  %9 = ptrtoint float* %8 to i64
  %10 = and i64 %9, 31
  %11 = icmp eq i64 %10, 0
  call void @llvm.assume(i1 %11)
  %12 = getelementptr inbounds %f32SX, %f32SX* %6, i64 0, i32 6, i64 0
  %13 = ptrtoint float* %12 to i64
  %14 = and i64 %13, 31
  %15 = icmp eq i64 %14, 0
  call void @llvm.assume(i1 %15)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %16 = mul nuw nsw i64 %y, %mat_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %17 = add nuw nsw i64 %x, %16
  %18 = getelementptr %f32SXY, %f32SXY* %4, i64 0, i32 6, i64 %17
  %19 = load float, float* %18, align 4, !llvm.mem.parallel_loop_access !4
  %20 = getelementptr %f32SX, %f32SX* %6, i64 0, i32 6, i64 %x
  %21 = load float, float* %20, align 4, !llvm.mem.parallel_loop_access !4
  %22 = fsub fast float %19, %21
  store float %22, float* %18, align 4, !llvm.mem.parallel_loop_access !4
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %mat_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

; Function Attrs: nounwind
define private void @covariance_tmp_thunk4({ %f32SXY*, %f32SXY*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32SXY*, %f32SXY*, i32 }, { %f32SXY*, %f32SXY*, i32 }* %0, i64 0, i32 0
  %4 = load %f32SXY*, %f32SXY** %3, align 8
  %5 = getelementptr inbounds { %f32SXY*, %f32SXY*, i32 }, { %f32SXY*, %f32SXY*, i32 }* %0, i64 0, i32 1
  %6 = load %f32SXY*, %f32SXY** %5, align 8
  %7 = getelementptr inbounds { %f32SXY*, %f32SXY*, i32 }, { %f32SXY*, %f32SXY*, i32 }* %0, i64 0, i32 2
  %8 = load i32, i32* %7, align 4
  %9 = getelementptr inbounds %f32SXY, %f32SXY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %9, align 4, !range !2
  %dst_y_step = zext i32 %columns1 to i64
  %10 = getelementptr inbounds %f32SXY, %f32SXY* %4, i64 0, i32 6, i64 0
  %11 = ptrtoint float* %10 to i64
  %12 = and i64 %11, 31
  %13 = icmp eq i64 %12, 0
  call void @llvm.assume(i1 %13)
  %14 = getelementptr inbounds %f32SXY, %f32SXY* %6, i64 0, i32 6, i64 0
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
  br i1 %20, label %Flow6, label %loop.preheader

loop.preheader:                                   ; preds = %x_body
  br i1 %18, label %exit4, label %true_entry3

x_exit:                                           ; preds = %Flow6
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void

true_entry3:                                      ; preds = %loop.preheader, %true_entry3
  %21 = phi i32 [ %35, %true_entry3 ], [ 0, %loop.preheader ]
  %22 = phi double [ %34, %true_entry3 ], [ 0.000000e+00, %loop.preheader ]
  %23 = sext i32 %21 to i64
  %24 = mul nuw nsw i64 %23, %dst_y_step
  %25 = add nuw nsw i64 %24, %x
  %26 = getelementptr %f32SXY, %f32SXY* %6, i64 0, i32 6, i64 %25
  %27 = load float, float* %26, align 4, !llvm.mem.parallel_loop_access !5
  %28 = fpext float %27 to double
  %29 = add nuw nsw i64 %24, %y
  %30 = getelementptr %f32SXY, %f32SXY* %6, i64 0, i32 6, i64 %29
  %31 = load float, float* %30, align 4, !llvm.mem.parallel_loop_access !5
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

exit4:                                            ; preds = %true_entry3, %loop.preheader
  %.lcssa = phi double [ 0.000000e+00, %loop.preheader ], [ %34, %true_entry3 ]
  %37 = add nuw nsw i64 %x, %19
  %38 = getelementptr %f32SXY, %f32SXY* %4, i64 0, i32 6, i64 %37
  %39 = fptrunc double %.lcssa to float
  store float %39, float* %38, align 4, !llvm.mem.parallel_loop_access !5
  %40 = mul nuw nsw i64 %x, %dst_y_step
  %41 = add nuw nsw i64 %40, %y
  %42 = getelementptr %f32SXY, %f32SXY* %4, i64 0, i32 6, i64 %41
  store float %39, float* %42, align 4, !llvm.mem.parallel_loop_access !5
  br label %Flow6
}

define %f32SXY* @covariance(%i16SXY*) {
entry:
  %1 = getelementptr inbounds %i16SXY, %i16SXY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !2
  %2 = call %u0CXYT* @likely_new(i32 9504, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %i16SXY, %i16SXY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !2
  %4 = zext i32 %columns to i64
  %5 = alloca { %f32SX*, i32 }, align 8
  %6 = bitcast { %f32SX*, i32 }* %5 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %6, align 8
  %7 = getelementptr inbounds { %f32SX*, i32 }, { %f32SX*, i32 }* %5, i64 0, i32 1
  store i32 0, i32* %7, align 8
  %8 = bitcast { %f32SX*, i32 }* %5 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32SX*, i32 }*, i64, i64)* @covariance_tmp_thunk0 to i8*), i8* %8, i64 %4)
  %rows2 = load i32, i32* %3, align 4, !range !2
  %9 = zext i32 %rows2 to i64
  %10 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %11 = bitcast %u0CXYT* %10 to float*
  %12 = ptrtoint %u0CXYT* %10 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  call void @llvm.assume(i1 %14)
  %columns4 = load i32, i32* %1, align 4, !range !2
  %src_y_step = zext i32 %columns4 to i64
  %15 = getelementptr inbounds %i16SXY, %i16SXY* %0, i64 0, i32 6, i64 0
  %16 = ptrtoint i16* %15 to i64
  %17 = and i64 %16, 31
  %18 = icmp eq i64 %17, 0
  call void @llvm.assume(i1 %18)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %19 = mul nuw nsw i64 %y, %src_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %20 = getelementptr float, float* %11, i64 %x
  %21 = load float, float* %20, align 4
  %22 = add nuw nsw i64 %x, %19
  %23 = getelementptr %i16SXY, %i16SXY* %0, i64 0, i32 6, i64 %22
  %24 = load i16, i16* %23, align 2
  %25 = sitofp i16 %24 to float
  %26 = fadd fast float %25, %21
  store float %26, float* %20, align 4
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %src_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %9
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %27 = icmp eq i32 %rows, 1
  br i1 %27, label %exit, label %true_entry

true_entry:                                       ; preds = %y_exit
  %28 = uitofp i32 %rows to float
  %29 = fdiv fast float 1.000000e+00, %28
  %30 = alloca { %f32SX*, float }, align 8
  %31 = bitcast { %f32SX*, float }* %30 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %31, align 8
  %32 = getelementptr inbounds { %f32SX*, float }, { %f32SX*, float }* %30, i64 0, i32 1
  store float %29, float* %32, align 8
  %33 = bitcast { %f32SX*, float }* %30 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32SX*, float }*, i64, i64)* @covariance_tmp_thunk1 to i8*), i8* %33, i64 %4)
  %columns7.pre = load i32, i32* %1, align 4
  %rows8.pre = load i32, i32* %3, align 4
  br label %exit

exit:                                             ; preds = %y_exit, %true_entry
  %rows8 = phi i32 [ %rows2, %y_exit ], [ %rows8.pre, %true_entry ]
  %columns7 = phi i32 [ %columns4, %y_exit ], [ %columns7.pre, %true_entry ]
  %34 = call %u0CXYT* @likely_new(i32 25888, i32 1, i32 %columns7, i32 %rows8, i32 1, i8* null)
  %35 = zext i32 %rows8 to i64
  %36 = alloca { %f32SXY*, %i16SXY* }, align 8
  %37 = bitcast { %f32SXY*, %i16SXY* }* %36 to %u0CXYT**
  store %u0CXYT* %34, %u0CXYT** %37, align 8
  %38 = getelementptr inbounds { %f32SXY*, %i16SXY* }, { %f32SXY*, %i16SXY* }* %36, i64 0, i32 1
  store %i16SXY* %0, %i16SXY** %38, align 8
  %39 = bitcast { %f32SXY*, %i16SXY* }* %36 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32SXY*, %i16SXY* }*, i64, i64)* @covariance_tmp_thunk2 to i8*), i8* %39, i64 %35)
  %40 = alloca { %f32SXY*, %f32SX* }, align 8
  %41 = bitcast { %f32SXY*, %f32SX* }* %40 to %u0CXYT**
  store %u0CXYT* %34, %u0CXYT** %41, align 8
  %42 = getelementptr inbounds { %f32SXY*, %f32SX* }, { %f32SXY*, %f32SX* }* %40, i64 0, i32 1
  %43 = bitcast %f32SX** %42 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %43, align 8
  %44 = bitcast { %f32SXY*, %f32SX* }* %40 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32SXY*, %f32SX* }*, i64, i64)* @covariance_tmp_thunk3 to i8*), i8* %44, i64 %35)
  %45 = call %u0CXYT* @likely_new(i32 25888, i32 1, i32 %columns7, i32 %columns7, i32 1, i8* null)
  %dst = bitcast %u0CXYT* %45 to %f32SXY*
  %46 = zext i32 %columns7 to i64
  %47 = alloca { %f32SXY*, %f32SXY*, i32 }, align 8
  %48 = bitcast { %f32SXY*, %f32SXY*, i32 }* %47 to %u0CXYT**
  store %u0CXYT* %45, %u0CXYT** %48, align 8
  %49 = getelementptr inbounds { %f32SXY*, %f32SXY*, i32 }, { %f32SXY*, %f32SXY*, i32 }* %47, i64 0, i32 1
  %50 = bitcast %f32SXY** %49 to %u0CXYT**
  store %u0CXYT* %34, %u0CXYT** %50, align 8
  %51 = getelementptr inbounds { %f32SXY*, %f32SXY*, i32 }, { %f32SXY*, %f32SXY*, i32 }* %47, i64 0, i32 2
  store i32 %rows8, i32* %51, align 8
  %52 = bitcast { %f32SXY*, %f32SXY*, i32 }* %47 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32SXY*, %f32SXY*, i32 }*, i64, i64)* @covariance_tmp_thunk4 to i8*), i8* %52, i64 %46)
  %53 = bitcast %u0CXYT* %2 to i8*
  call void @likely_release_mat(i8* %53)
  %54 = bitcast %u0CXYT* %34 to i8*
  call void @likely_release_mat(i8* %54)
  ret %f32SXY* %dst
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind argmemonly }
attributes #1 = { nounwind }

!0 = distinct !{!0}
!1 = distinct !{!1}
!2 = !{i32 1, i32 -1}
!3 = distinct !{!3}
!4 = distinct !{!4}
!5 = distinct !{!5}
