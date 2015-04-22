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
  %1 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 4
  %2 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 3
  %3 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 2
  br label %then

then:                                             ; preds = %entry, %end3
  %storemerge29 = phi i32 [ 0, %entry ], [ %21, %end3 ]
  %4 = phi double [ 0.000000e+00, %entry ], [ %39, %end3 ]
  %rows = load i32, i32* %1, align 4, !range !0
  %5 = sext i32 %storemerge29 to i64
  %6 = zext i32 %rows to i64
  %7 = mul nsw i64 %6, %5
  br label %then2

end:                                              ; preds = %end3
  %8 = tail call double @llvm.sqrt.f64(double %39)
  %9 = fdiv double 1.000000e+00, %8
  %channels13 = load i32, i32* %3, align 4, !range !0
  %columns14 = load i32, i32* %2, align 4, !range !0
  %rows15 = load i32, i32* %1, align 4, !range !0
  %10 = tail call %u0CXYT* @likely_new(i32 30224, i32 %channels13, i32 %columns14, i32 %rows15, i32 1, i8* null)
  %11 = bitcast %u0CXYT* %10 to %i16SCXY*
  %12 = zext i32 %rows15 to i64
  %13 = alloca { %i16SCXY*, %i16SCXY*, double }, align 8
  %14 = bitcast { %i16SCXY*, %i16SCXY*, double }* %13 to %u0CXYT**
  store %u0CXYT* %10, %u0CXYT** %14, align 8
  %15 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, double }, { %i16SCXY*, %i16SCXY*, double }* %13, i64 0, i32 1
  store %i16SCXY* %0, %i16SCXY** %15, align 8
  %16 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, double }, { %i16SCXY*, %i16SCXY*, double }* %13, i64 0, i32 2
  store double %9, double* %16, align 8
  %17 = bitcast { %i16SCXY*, %i16SCXY*, double }* %13 to i8*
  call void @likely_fork(i8* bitcast (void ({ %i16SCXY*, %i16SCXY*, double }*, i64, i64)* @normalize_l2_tmp_thunk0 to i8*), i8* %17, i64 %12)
  ret %i16SCXY* %11

then2:                                            ; preds = %then, %end6
  %storemerge128 = phi i32 [ 0, %then ], [ %28, %end6 ]
  %18 = phi double [ %4, %then ], [ %39, %end6 ]
  %columns = load i32, i32* %2, align 4, !range !0
  %19 = sext i32 %storemerge128 to i64
  %20 = zext i32 %columns to i64
  %tmp = add i64 %7, %19
  br label %then5

end3:                                             ; preds = %end6
  %21 = add nuw nsw i32 %storemerge29, 1
  %22 = icmp eq i32 %storemerge29, 0
  br i1 %22, label %end, label %then

then5:                                            ; preds = %then2, %end9
  %storemerge227 = phi i32 [ 0, %then2 ], [ %42, %end9 ]
  %23 = phi double [ %18, %then2 ], [ %39, %end9 ]
  %channels = load i32, i32* %3, align 4, !range !0
  %24 = sext i32 %storemerge227 to i64
  %25 = zext i32 %channels to i64
  %26 = mul nuw nsw i64 %20, %25
  %27 = mul nsw i64 %25, %24
  %tmp7 = mul i64 %26, %tmp
  br label %then8

end6:                                             ; preds = %end9
  %28 = add nuw nsw i32 %storemerge128, 1
  %29 = icmp eq i32 %28, %rows
  br i1 %29, label %end3, label %then2

then8:                                            ; preds = %then5, %then8
  %storemerge326 = phi i32 [ 0, %then5 ], [ %40, %then8 ]
  %30 = phi double [ %23, %then5 ], [ %39, %then8 ]
  %31 = sext i32 %storemerge326 to i64
  %32 = add i64 %27, %31
  %33 = add i64 %32, %tmp7
  %34 = getelementptr %i16SCXY, %i16SCXY* %0, i64 0, i32 6, i64 %33
  %35 = load i16, i16* %34, align 2
  %36 = sitofp i16 %35 to float
  %37 = fmul float %36, %36
  %38 = fpext float %37 to double
  %39 = fadd double %30, %38
  %40 = add nuw nsw i32 %storemerge326, 1
  %41 = icmp eq i32 %40, %channels
  br i1 %41, label %end9, label %then8

end9:                                             ; preds = %then8
  %42 = add nuw nsw i32 %storemerge227, 1
  %43 = icmp eq i32 %42, %columns
  br i1 %43, label %end6, label %then5
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
