; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readnone
declare double @llvm.sqrt.f64(double) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: norecurse nounwind
define private void @normalize_l2_tmp_thunk0({ %f32Matrix*, %f32Matrix*, float }* noalias nocapture readonly, i64, i64) #2 {
entry:
  br label %entry.split

entry.split:                                      ; preds = %entry
  %3 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, float }, { %f32Matrix*, %f32Matrix*, float }* %0, i64 0, i32 0
  %4 = load %f32Matrix*, %f32Matrix** %3, align 8
  %5 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, float }, { %f32Matrix*, %f32Matrix*, float }* %0, i64 0, i32 1
  %6 = load %f32Matrix*, %f32Matrix** %5, align 8
  %7 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, float }, { %f32Matrix*, %f32Matrix*, float }* %0, i64 0, i32 2
  %8 = load float, float* %7, align 4
  %9 = getelementptr inbounds %f32Matrix, %f32Matrix* %6, i64 0, i32 2
  %channels1 = load i32, i32* %9, align 4, !range !0
  %dst_c = zext i32 %channels1 to i64
  %10 = getelementptr inbounds %f32Matrix, %f32Matrix* %6, i64 0, i32 3
  %columns2 = load i32, i32* %10, align 4, !range !0
  %dst_x = zext i32 %columns2 to i64
  %11 = mul i64 %dst_c, %2
  %12 = mul i64 %11, %dst_x
  br label %y_body

y_body:                                           ; preds = %y_body, %entry.split
  %y = phi i64 [ %1, %entry.split ], [ %y_increment, %y_body ]
  %13 = getelementptr %f32Matrix, %f32Matrix* %6, i64 0, i32 6, i64 %y
  %14 = load float, float* %13, align 4, !llvm.mem.parallel_loop_access !1
  %15 = fmul fast float %14, %8
  %16 = getelementptr %f32Matrix, %f32Matrix* %4, i64 0, i32 6, i64 %y
  store float %15, float* %16, align 4, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %12
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  ret void
}

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: nounwind
define noalias %f32Matrix* @normalize_l2(%f32Matrix* noalias nocapture) #3 {
entry:
  br label %entry.split

entry.split:                                      ; preds = %entry
  %1 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = mul nuw nsw i32 %columns, %channels
  %4 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !0
  %5 = mul nuw nsw i32 %3, %rows
  br label %true_entry

true_entry:                                       ; preds = %true_entry, %entry.split
  %6 = phi i32 [ 0, %entry.split ], [ %14, %true_entry ]
  %7 = phi double [ 0.000000e+00, %entry.split ], [ %13, %true_entry ]
  %8 = zext i32 %6 to i64
  %9 = getelementptr %f32Matrix, %f32Matrix* %0, i64 0, i32 6, i64 %8
  %10 = load float, float* %9, align 4
  %11 = fpext float %10 to double
  %12 = fmul fast double %11, %11
  %13 = fadd fast double %12, %7
  %14 = add nuw nsw i32 %6, 1
  %15 = icmp eq i32 %14, %5
  br i1 %15, label %exit, label %true_entry

exit:                                             ; preds = %true_entry
  %16 = call fast double @llvm.sqrt.f64(double %13)
  %17 = fdiv fast double 1.000000e+00, %16
  %norm4 = fptrunc double %17 to float
  %18 = call %u0Matrix* @likely_new(i32 28960, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %dst = bitcast %u0Matrix* %18 to %f32Matrix*
  %19 = zext i32 %rows to i64
  %20 = alloca { %f32Matrix*, %f32Matrix*, float }, align 8
  %21 = bitcast { %f32Matrix*, %f32Matrix*, float }* %20 to %u0Matrix**
  store %u0Matrix* %18, %u0Matrix** %21, align 8
  %22 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, float }, { %f32Matrix*, %f32Matrix*, float }* %20, i64 0, i32 1
  store %f32Matrix* %0, %f32Matrix** %22, align 8
  %23 = getelementptr inbounds { %f32Matrix*, %f32Matrix*, float }, { %f32Matrix*, %f32Matrix*, float }* %20, i64 0, i32 2
  store float %norm4, float* %23, align 8
  %24 = bitcast { %f32Matrix*, %f32Matrix*, float }* %20 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32Matrix*, %f32Matrix*, float }*, i64, i64)* @normalize_l2_tmp_thunk0 to i8*), i8* %24, i64 %19) #3
  ret %f32Matrix* %dst
}

attributes #0 = { nounwind readnone }
attributes #1 = { argmemonly nounwind }
attributes #2 = { norecurse nounwind }
attributes #3 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
