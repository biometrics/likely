; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i32] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: norecurse nounwind
define private void @transpose_tmp_thunk0({ %u32Matrix*, %u32Matrix* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  br label %entry.split

entry.split:                                      ; preds = %entry
  %3 = getelementptr inbounds { %u32Matrix*, %u32Matrix* }, { %u32Matrix*, %u32Matrix* }* %0, i64 0, i32 0
  %4 = load %u32Matrix*, %u32Matrix** %3, align 8
  %5 = getelementptr inbounds { %u32Matrix*, %u32Matrix* }, { %u32Matrix*, %u32Matrix* }* %0, i64 0, i32 1
  %6 = load %u32Matrix*, %u32Matrix** %5, align 8
  %7 = getelementptr inbounds %u32Matrix, %u32Matrix* %6, i64 0, i32 4
  %rows2 = load i32, i32* %7, align 4, !range !0
  %dst_y_step = zext i32 %rows2 to i64
  %8 = getelementptr inbounds %u32Matrix, %u32Matrix* %6, i64 0, i32 3
  %columns1 = load i32, i32* %8, align 4, !range !0
  %src_y_step = zext i32 %columns1 to i64
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry.split
  %y = phi i64 [ %1, %entry.split ], [ %y_increment, %x_exit ]
  %9 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %10 = mul nuw nsw i64 %x, %src_y_step
  %11 = add nuw nsw i64 %10, %y
  %12 = getelementptr %u32Matrix, %u32Matrix* %6, i64 0, i32 6, i64 %11
  %13 = load i32, i32* %12, align 4, !llvm.mem.parallel_loop_access !1
  %14 = add nuw nsw i64 %x, %9
  %15 = getelementptr %u32Matrix, %u32Matrix* %4, i64 0, i32 6, i64 %14
  store i32 %13, i32* %15, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: nounwind
define noalias %u32Matrix* @transpose(%u32Matrix* noalias nocapture) #2 {
entry:
  br label %entry.split

entry.split:                                      ; preds = %entry
  %1 = getelementptr inbounds %u32Matrix, %u32Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %u32Matrix, %u32Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = call %u0Matrix* @likely_new(i32 25120, i32 1, i32 %rows, i32 %columns, i32 1, i8* null)
  %dst = bitcast %u0Matrix* %3 to %u32Matrix*
  %4 = zext i32 %columns to i64
  %5 = alloca { %u32Matrix*, %u32Matrix* }, align 8
  %6 = bitcast { %u32Matrix*, %u32Matrix* }* %5 to %u0Matrix**
  store %u0Matrix* %3, %u0Matrix** %6, align 8
  %7 = getelementptr inbounds { %u32Matrix*, %u32Matrix* }, { %u32Matrix*, %u32Matrix* }* %5, i64 0, i32 1
  store %u32Matrix* %0, %u32Matrix** %7, align 8
  %8 = bitcast { %u32Matrix*, %u32Matrix* }* %5 to i8*
  call void @likely_fork(i8* bitcast (void ({ %u32Matrix*, %u32Matrix* }*, i64, i64)* @transpose_tmp_thunk0 to i8*), i8* %8, i64 %4) #2
  ret %u32Matrix* %dst
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { norecurse nounwind }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
