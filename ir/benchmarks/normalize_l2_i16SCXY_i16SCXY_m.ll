; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%i16SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: nounwind readnone
declare double @llvm.sqrt.f64(double) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
define private void @normalize_l2_tmp_thunk0({ %i16SCXY*, %i16SCXY*, double }* noalias nocapture readonly, i64, i64) #2 {
entry:
  %3 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, double }, { %i16SCXY*, %i16SCXY*, double }* %0, i64 0, i32 0
  %4 = load %i16SCXY*, %i16SCXY** %3, align 8
  %5 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, double }, { %i16SCXY*, %i16SCXY*, double }* %0, i64 0, i32 1
  %6 = load %i16SCXY*, %i16SCXY** %5, align 8
  %7 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, double }, { %i16SCXY*, %i16SCXY*, double }* %0, i64 0, i32 2
  %8 = load double, double* %7, align 8
  %9 = getelementptr inbounds %i16SCXY, %i16SCXY* %4, i64 0, i32 2
  %channels = load i32, i32* %9, align 4, !range !0
  %dst_c = zext i32 %channels to i64
  %10 = getelementptr inbounds %i16SCXY, %i16SCXY* %4, i64 0, i32 3
  %columns = load i32, i32* %10, align 4, !range !0
  %dst_x = zext i32 %columns to i64
  %11 = getelementptr inbounds %i16SCXY, %i16SCXY* %4, i64 0, i32 6, i64 0
  %12 = ptrtoint i16* %11 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  tail call void @llvm.assume(i1 %14)
  %15 = getelementptr inbounds %i16SCXY, %i16SCXY* %6, i64 0, i32 6, i64 0
  %16 = ptrtoint i16* %15 to i64
  %17 = and i64 %16, 31
  %18 = icmp eq i64 %17, 0
  tail call void @llvm.assume(i1 %18)
  %19 = mul nuw nsw i64 %dst_x, %dst_c
  %20 = mul nuw nsw i64 %19, %1
  %21 = mul nuw nsw i64 %19, %2
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ %20, %entry ], [ %y_increment, %y_body ]
  %22 = getelementptr %i16SCXY, %i16SCXY* %6, i64 0, i32 6, i64 %y
  %23 = load i16, i16* %22, align 2, !llvm.mem.parallel_loop_access !1
  %24 = sitofp i16 %23 to double
  %25 = fmul double %8, %24
  %26 = fptosi double %25 to i16
  %27 = fcmp olt double %25, -3.276800e+04
  %28 = select i1 %27, i16 -32768, i16 %26
  %29 = fcmp ogt double %25, 3.276700e+04
  %30 = select i1 %29, i16 32767, i16 %28
  %31 = getelementptr %i16SCXY, %i16SCXY* %4, i64 0, i32 6, i64 %y
  store i16 %30, i16* %31, align 2, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %21
  br i1 %y_postcondition, label %y_exit, label %y_body, !llvm.loop !1

y_exit:                                           ; preds = %y_body
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #2

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %i16SCXY* @normalize_l2(%i16SCXY*) {
entry:
  %1 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = mul nuw nsw i32 %columns, %channels
  %4 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !0
  %5 = mul nuw nsw i32 %3, %rows
  br label %then

then:                                             ; preds = %entry, %then
  %storemerge4 = phi i32 [ 0, %entry ], [ %13, %then ]
  %6 = phi double [ 0.000000e+00, %entry ], [ %12, %then ]
  %7 = sext i32 %storemerge4 to i64
  %8 = getelementptr %i16SCXY, %i16SCXY* %0, i64 0, i32 6, i64 %7
  %9 = load i16, i16* %8, align 2
  %10 = sitofp i16 %9 to double
  %11 = fmul double %10, %10
  %12 = fadd double %6, %11
  %13 = add nuw nsw i32 %storemerge4, 1
  %14 = icmp eq i32 %13, %5
  br i1 %14, label %end, label %then

end:                                              ; preds = %then
  %15 = tail call double @llvm.sqrt.f64(double %12)
  %16 = fdiv double 1.000000e+00, %15
  %17 = tail call %u0CXYT* @likely_new(i32 30224, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %18 = bitcast %u0CXYT* %17 to %i16SCXY*
  %19 = zext i32 %rows to i64
  %20 = alloca { %i16SCXY*, %i16SCXY*, double }, align 8
  %21 = bitcast { %i16SCXY*, %i16SCXY*, double }* %20 to %u0CXYT**
  store %u0CXYT* %17, %u0CXYT** %21, align 8
  %22 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, double }, { %i16SCXY*, %i16SCXY*, double }* %20, i64 0, i32 1
  store %i16SCXY* %0, %i16SCXY** %22, align 8
  %23 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, double }, { %i16SCXY*, %i16SCXY*, double }* %20, i64 0, i32 2
  store double %16, double* %23, align 8
  %24 = bitcast { %i16SCXY*, %i16SCXY*, double }* %20 to i8*
  call void @likely_fork(i8* bitcast (void ({ %i16SCXY*, %i16SCXY*, double }*, i64, i64)* @normalize_l2_tmp_thunk0 to i8*), i8* %24, i64 %19)
  ret %i16SCXY* %18
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
