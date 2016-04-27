; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32SX = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%i16SXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: norecurse nounwind
define private void @average_tmp_thunk0({ %f32SX*, i32 }* noalias nocapture readonly, i64, i64) #1 {
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
declare void @llvm.assume(i1) #2

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: norecurse nounwind
define private void @average_tmp_thunk1({ %f32SX*, float }* noalias nocapture readonly, i64, i64) #1 {
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

define %f32SX* @average(%i16SXY*) {
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
  call void @likely_fork(i8* bitcast (void ({ %f32SX*, i32 }*, i64, i64)* @average_tmp_thunk0 to i8*), i8* %8, i64 %4)
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
  %27 = bitcast %u0CXYT* %2 to %f32SX*
  %28 = icmp eq i32 %rows, 1
  br i1 %28, label %exit, label %true_entry

true_entry:                                       ; preds = %y_exit
  %29 = uitofp i32 %rows to float
  %30 = fdiv fast float 1.000000e+00, %29
  %31 = alloca { %f32SX*, float }, align 8
  %32 = bitcast { %f32SX*, float }* %31 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %32, align 8
  %33 = getelementptr inbounds { %f32SX*, float }, { %f32SX*, float }* %31, i64 0, i32 1
  store float %30, float* %33, align 8
  %34 = bitcast { %f32SX*, float }* %31 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32SX*, float }*, i64, i64)* @average_tmp_thunk1 to i8*), i8* %34, i64 %4)
  br label %exit

exit:                                             ; preds = %y_exit, %true_entry
  ret %f32SX* %27
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { norecurse nounwind }
attributes #2 = { nounwind }

!0 = distinct !{!0}
!1 = distinct !{!1}
!2 = !{i32 1, i32 -1}
