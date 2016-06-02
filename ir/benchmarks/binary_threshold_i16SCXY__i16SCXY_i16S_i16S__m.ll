; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%i16SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: norecurse nounwind
define private void @binary_threshold_tmp_thunk0({ %i16SCXY*, %i16SCXY*, i16, i16 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, i16, i16 }, { %i16SCXY*, %i16SCXY*, i16, i16 }* %0, i64 0, i32 0
  %4 = load %i16SCXY*, %i16SCXY** %3, align 8
  %5 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, i16, i16 }, { %i16SCXY*, %i16SCXY*, i16, i16 }* %0, i64 0, i32 1
  %6 = load %i16SCXY*, %i16SCXY** %5, align 8
  %7 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, i16, i16 }, { %i16SCXY*, %i16SCXY*, i16, i16 }* %0, i64 0, i32 2
  %8 = load i16, i16* %7, align 2
  %9 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, i16, i16 }, { %i16SCXY*, %i16SCXY*, i16, i16 }* %0, i64 0, i32 3
  %10 = load i16, i16* %9, align 2
  %11 = getelementptr inbounds %i16SCXY, %i16SCXY* %6, i64 0, i32 2
  %channels1 = load i32, i32* %11, align 4, !range !0
  %dst_c = zext i32 %channels1 to i64
  %12 = getelementptr inbounds %i16SCXY, %i16SCXY* %6, i64 0, i32 3
  %columns2 = load i32, i32* %12, align 4, !range !0
  %dst_x = zext i32 %columns2 to i64
  %13 = mul i64 %dst_c, %2
  %14 = mul i64 %13, %dst_x
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %y_body ]
  %15 = getelementptr %i16SCXY, %i16SCXY* %6, i64 0, i32 6, i64 %y
  %16 = load i16, i16* %15, align 2, !llvm.mem.parallel_loop_access !1
  %17 = icmp sgt i16 %16, %8
  %. = select i1 %17, i16 %10, i16 0
  %18 = getelementptr %i16SCXY, %i16SCXY* %4, i64 0, i32 6, i64 %y
  store i16 %., i16* %18, align 2, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %14
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  ret void
}

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %i16SCXY* @binary_threshold(%i16SCXY*, i16, i16) {
entry:
  %3 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 2
  %channels = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 3
  %columns = load i32, i32* %4, align 4, !range !0
  %5 = getelementptr inbounds %i16SCXY, %i16SCXY* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = call %u0CXYT* @likely_new(i32 30224, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %dst = bitcast %u0CXYT* %6 to %i16SCXY*
  %7 = zext i32 %rows to i64
  %8 = alloca { %i16SCXY*, %i16SCXY*, i16, i16 }, align 8
  %9 = bitcast { %i16SCXY*, %i16SCXY*, i16, i16 }* %8 to %u0CXYT**
  store %u0CXYT* %6, %u0CXYT** %9, align 8
  %10 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, i16, i16 }, { %i16SCXY*, %i16SCXY*, i16, i16 }* %8, i64 0, i32 1
  store %i16SCXY* %0, %i16SCXY** %10, align 8
  %11 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, i16, i16 }, { %i16SCXY*, %i16SCXY*, i16, i16 }* %8, i64 0, i32 2
  store i16 %1, i16* %11, align 8
  %12 = getelementptr inbounds { %i16SCXY*, %i16SCXY*, i16, i16 }, { %i16SCXY*, %i16SCXY*, i16, i16 }* %8, i64 0, i32 3
  store i16 %2, i16* %12, align 2
  %13 = bitcast { %i16SCXY*, %i16SCXY*, i16, i16 }* %8 to i8*
  call void @likely_fork(i8* bitcast (void ({ %i16SCXY*, %i16SCXY*, i16, i16 }*, i64, i64)* @binary_threshold_tmp_thunk0 to i8*), i8* %13, i64 %7)
  ret %i16SCXY* %dst
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { norecurse nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
