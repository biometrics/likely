; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: norecurse nounwind
define private void @average_tmp_thunk0({ %f32Matrix*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32Matrix*, i32 }, { %f32Matrix*, i32 }* %0, i64 0, i32 0
  %4 = load %f32Matrix*, %f32Matrix** %3, align 8
  %5 = getelementptr inbounds { %f32Matrix*, i32 }, { %f32Matrix*, i32 }* %0, i64 0, i32 1
  %6 = load i32, i32* %5, align 4
  %7 = sitofp i32 %6 to float
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %8 = getelementptr %f32Matrix, %f32Matrix* %4, i64 0, i32 6, i64 %x
  store float %7, float* %8, align 4, !llvm.mem.parallel_loop_access !0
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  ret void
}

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: norecurse nounwind
define private void @average_tmp_thunk1({ %f32Matrix*, float }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32Matrix*, float }, { %f32Matrix*, float }* %0, i64 0, i32 0
  %4 = load %f32Matrix*, %f32Matrix** %3, align 8
  %5 = getelementptr inbounds { %f32Matrix*, float }, { %f32Matrix*, float }* %0, i64 0, i32 1
  %6 = load float, float* %5, align 4
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %7 = getelementptr %f32Matrix, %f32Matrix* %4, i64 0, i32 6, i64 %x
  %8 = load float, float* %7, align 4, !llvm.mem.parallel_loop_access !1
  %9 = fmul fast float %8, %6
  store float %9, float* %7, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  ret void
}

define %f32Matrix* @average(%f32Matrix* nocapture readonly) {
entry:
  %1 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !2
  %2 = call %u0Matrix* @likely_new(i32 8480, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !2
  %4 = zext i32 %columns to i64
  %5 = alloca { %f32Matrix*, i32 }, align 8
  %6 = bitcast { %f32Matrix*, i32 }* %5 to %u0Matrix**
  store %u0Matrix* %2, %u0Matrix** %6, align 8
  %7 = getelementptr inbounds { %f32Matrix*, i32 }, { %f32Matrix*, i32 }* %5, i64 0, i32 1
  store i32 0, i32* %7, align 8
  %8 = bitcast { %f32Matrix*, i32 }* %5 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, i32 }*, i64, i64)* @average_tmp_thunk0 to i8*), i8* %8, i64 %4)
  %rows2 = load i32, i32* %3, align 4, !range !2
  %9 = zext i32 %rows2 to i64
  %10 = getelementptr inbounds %u0Matrix, %u0Matrix* %2, i64 1
  %11 = bitcast %u0Matrix* %10 to float*
  %columns4 = load i32, i32* %1, align 4, !range !2
  %src_y_step = zext i32 %columns4 to i64
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %12 = mul nuw nsw i64 %y, %src_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %13 = getelementptr float, float* %11, i64 %x
  %14 = load float, float* %13, align 4
  %15 = add nuw nsw i64 %x, %12
  %16 = getelementptr %f32Matrix, %f32Matrix* %0, i64 0, i32 6, i64 %15
  %17 = load float, float* %16, align 4
  %18 = fadd fast float %17, %14
  store float %18, float* %13, align 4
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %src_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %9
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %19 = bitcast %u0Matrix* %2 to %f32Matrix*
  %20 = icmp eq i32 %rows, 1
  br i1 %20, label %exit, label %true_entry

true_entry:                                       ; preds = %y_exit
  %21 = uitofp i32 %rows to float
  %22 = fdiv fast float 1.000000e+00, %21
  %23 = alloca { %f32Matrix*, float }, align 8
  %24 = bitcast { %f32Matrix*, float }* %23 to %u0Matrix**
  store %u0Matrix* %2, %u0Matrix** %24, align 8
  %25 = getelementptr inbounds { %f32Matrix*, float }, { %f32Matrix*, float }* %23, i64 0, i32 1
  store float %22, float* %25, align 8
  %26 = bitcast { %f32Matrix*, float }* %23 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, float }*, i64, i64)* @average_tmp_thunk1 to i8*), i8* %26, i64 %4)
  br label %exit

exit:                                             ; preds = %y_exit, %true_entry
  ret %f32Matrix* %19
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { norecurse nounwind }

!0 = distinct !{!0}
!1 = distinct !{!1}
!2 = !{i32 1, i32 -1}
