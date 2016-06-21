; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u16Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: norecurse nounwind
define private void @binary_threshold_tmp_thunk0({ %u16Matrix*, %u16Matrix*, i16, i16 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %u16Matrix*, %u16Matrix*, i16, i16 }, { %u16Matrix*, %u16Matrix*, i16, i16 }* %0, i64 0, i32 0
  %4 = load %u16Matrix*, %u16Matrix** %3, align 8
  %5 = getelementptr inbounds { %u16Matrix*, %u16Matrix*, i16, i16 }, { %u16Matrix*, %u16Matrix*, i16, i16 }* %0, i64 0, i32 1
  %6 = load %u16Matrix*, %u16Matrix** %5, align 8
  %7 = getelementptr inbounds { %u16Matrix*, %u16Matrix*, i16, i16 }, { %u16Matrix*, %u16Matrix*, i16, i16 }* %0, i64 0, i32 2
  %8 = load i16, i16* %7, align 2
  %9 = getelementptr inbounds { %u16Matrix*, %u16Matrix*, i16, i16 }, { %u16Matrix*, %u16Matrix*, i16, i16 }* %0, i64 0, i32 3
  %10 = load i16, i16* %9, align 2
  %11 = getelementptr inbounds %u16Matrix, %u16Matrix* %6, i64 0, i32 2
  %channels1 = load i32, i32* %11, align 4, !range !0
  %dst_c = zext i32 %channels1 to i64
  %12 = getelementptr inbounds %u16Matrix, %u16Matrix* %6, i64 0, i32 3
  %columns2 = load i32, i32* %12, align 4, !range !0
  %dst_x = zext i32 %columns2 to i64
  %13 = mul i64 %dst_c, %2
  %14 = mul i64 %13, %dst_x
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %y_body ]
  %15 = getelementptr %u16Matrix, %u16Matrix* %6, i64 0, i32 6, i64 %y
  %16 = load i16, i16* %15, align 2, !llvm.mem.parallel_loop_access !1
  %17 = icmp sgt i16 %16, %8
  %. = select i1 %17, i16 %10, i16 0
  %18 = getelementptr %u16Matrix, %u16Matrix* %4, i64 0, i32 6, i64 %y
  store i16 %., i16* %18, align 2, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %14
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  ret void
}

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: nounwind
define noalias %u16Matrix* @binary_threshold(%u16Matrix* noalias nocapture, i16 signext, i16 signext) #2 {
entry:
  %3 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 2
  %channels = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %4, align 4, !range !0
  %5 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = call %u0Matrix* @likely_new(i32 30224, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %dst = bitcast %u0Matrix* %6 to %u16Matrix*
  %7 = zext i32 %rows to i64
  %8 = alloca { %u16Matrix*, %u16Matrix*, i16, i16 }, align 8
  %9 = bitcast { %u16Matrix*, %u16Matrix*, i16, i16 }* %8 to %u0Matrix**
  store %u0Matrix* %6, %u0Matrix** %9, align 8
  %10 = getelementptr inbounds { %u16Matrix*, %u16Matrix*, i16, i16 }, { %u16Matrix*, %u16Matrix*, i16, i16 }* %8, i64 0, i32 1
  store %u16Matrix* %0, %u16Matrix** %10, align 8
  %11 = getelementptr inbounds { %u16Matrix*, %u16Matrix*, i16, i16 }, { %u16Matrix*, %u16Matrix*, i16, i16 }* %8, i64 0, i32 2
  store i16 %1, i16* %11, align 8
  %12 = getelementptr inbounds { %u16Matrix*, %u16Matrix*, i16, i16 }, { %u16Matrix*, %u16Matrix*, i16, i16 }* %8, i64 0, i32 3
  store i16 %2, i16* %12, align 2
  %13 = bitcast { %u16Matrix*, %u16Matrix*, i16, i16 }* %8 to i8*
  call void @likely_fork(i8* bitcast (void ({ %u16Matrix*, %u16Matrix*, i16, i16 }*, i64, i64)* @binary_threshold_tmp_thunk0 to i8*), i8* %13, i64 %7) #2
  ret %u16Matrix* %dst
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { norecurse nounwind }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
