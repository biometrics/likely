; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%i16SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: nounwind readnone
declare double @llvm.sqrt.f64(double) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
declare void @llvm.assume(i1) #2

; Function Attrs: nounwind
define %i16SCXY* @normalize_l2(%i16SCXY*) #2 {
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
  %18 = zext i32 %rows to i64
  %dst_c = zext i32 %channels to i64
  %dst_x = zext i32 %columns to i64
  %19 = getelementptr inbounds %u0CXYT, %u0CXYT* %17, i64 1
  %20 = bitcast %u0CXYT* %19 to i16*
  %21 = ptrtoint %u0CXYT* %19 to i64
  %22 = and i64 %21, 31
  %23 = icmp eq i64 %22, 0
  tail call void @llvm.assume(i1 %23)
  %24 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 6, i64 0
  %25 = ptrtoint i16* %24 to i64
  %26 = and i64 %25, 31
  %27 = icmp eq i64 %26, 0
  tail call void @llvm.assume(i1 %27)
  %28 = mul nuw nsw i64 %dst_x, %dst_c
  %29 = mul nuw nsw i64 %28, %18
  br label %y_body

y_body:                                           ; preds = %y_body, %end
  %y = phi i64 [ 0, %end ], [ %y_increment, %y_body ]
  %30 = getelementptr %i16SCXY, %i16SCXY* %0, i64 0, i32 6, i64 %y
  %31 = load i16, i16* %30, align 2, !llvm.mem.parallel_loop_access !1
  %32 = sitofp i16 %31 to double
  %33 = fmul double %16, %32
  %34 = fptosi double %33 to i16
  %35 = fcmp olt double %33, -3.276800e+04
  %36 = select i1 %35, i16 -32768, i16 %34
  %37 = fcmp ogt double %33, 3.276700e+04
  %38 = select i1 %37, i16 32767, i16 %36
  %39 = getelementptr i16, i16* %20, i64 %y
  store i16 %38, i16* %39, align 2, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %29
  br i1 %y_postcondition, label %y_exit, label %y_body, !llvm.loop !1

y_exit:                                           ; preds = %y_body
  %40 = bitcast %u0CXYT* %17 to %i16SCXY*
  ret %i16SCXY* %40
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
