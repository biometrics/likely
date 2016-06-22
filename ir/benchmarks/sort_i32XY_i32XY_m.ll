; ModuleID = 'likely'
source_filename = "likely"

%u32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i32] }

; Function Attrs: norecurse nounwind
define private void @sort_tmp_thunk0({ %u32Matrix*, i32, i64, i64, i64, i64 }* noalias nocapture readonly, i64, i64) #0 {
entry:
  %3 = getelementptr inbounds { %u32Matrix*, i32, i64, i64, i64, i64 }, { %u32Matrix*, i32, i64, i64, i64, i64 }* %0, i64 0, i32 0
  %4 = load %u32Matrix*, %u32Matrix** %3, align 8
  %5 = getelementptr inbounds { %u32Matrix*, i32, i64, i64, i64, i64 }, { %u32Matrix*, i32, i64, i64, i64, i64 }* %0, i64 0, i32 1
  %6 = load i32, i32* %5, align 4
  %7 = getelementptr inbounds %u32Matrix, %u32Matrix* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !0
  %src_y_step = zext i32 %columns to i64
  %8 = icmp eq i32 %6, 0
  br label %y_body

y_body:                                           ; preds = %exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %exit ]
  br i1 %8, label %exit, label %true_entry.lr.ph

true_entry.lr.ph:                                 ; preds = %y_body
  %9 = mul nuw nsw i64 %y, %src_y_step
  br label %true_entry

true_entry:                                       ; preds = %true_entry.lr.ph, %loop.backedge
  %10 = phi i32 [ %15, %loop.backedge ], [ 0, %true_entry.lr.ph ]
  %11 = zext i32 %10 to i64
  %12 = add nuw nsw i64 %11, %9
  %13 = getelementptr %u32Matrix, %u32Matrix* %4, i64 0, i32 6, i64 %12
  %14 = load i32, i32* %13, align 4, !llvm.mem.parallel_loop_access !1
  %15 = add nuw nsw i32 %10, 1
  %16 = icmp eq i32 %15, %6
  br i1 %16, label %exit4, label %true_entry3

true_entry3:                                      ; preds = %true_entry, %true_entry3
  %17 = phi i32 [ %24, %true_entry3 ], [ %15, %true_entry ]
  %18 = phi i32 [ %., %true_entry3 ], [ %10, %true_entry ]
  %19 = phi i32 [ %element., %true_entry3 ], [ %14, %true_entry ]
  %20 = sext i32 %17 to i64
  %21 = add nuw nsw i64 %20, %9
  %22 = getelementptr %u32Matrix, %u32Matrix* %4, i64 0, i32 6, i64 %21
  %element = load i32, i32* %22, align 4, !llvm.mem.parallel_loop_access !1
  %23 = icmp slt i32 %element, %19
  %element. = select i1 %23, i32 %element, i32 %19
  %. = select i1 %23, i32 %17, i32 %18
  %24 = add nuw nsw i32 %17, 1
  %25 = icmp eq i32 %24, %6
  br i1 %25, label %exit4, label %true_entry3

exit4:                                            ; preds = %true_entry3, %true_entry
  %.lcssa = phi i32 [ %10, %true_entry ], [ %., %true_entry3 ]
  %26 = icmp eq i32 %10, %.lcssa
  br i1 %26, label %loop.backedge, label %true_entry7

loop.backedge:                                    ; preds = %exit4, %true_entry7
  br i1 %16, label %exit, label %true_entry

true_entry7:                                      ; preds = %exit4
  %27 = zext i32 %.lcssa to i64
  %28 = add nuw nsw i64 %27, %9
  %29 = getelementptr %u32Matrix, %u32Matrix* %4, i64 0, i32 6, i64 %28
  %30 = load i32, i32* %29, align 4, !llvm.mem.parallel_loop_access !1
  store i32 %30, i32* %13, align 4, !llvm.mem.parallel_loop_access !1
  store i32 %14, i32* %29, align 4, !llvm.mem.parallel_loop_access !1
  br label %loop.backedge

exit:                                             ; preds = %loop.backedge, %y_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %exit
  ret void
}

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: nounwind
define noalias %u32Matrix* @sort(%u32Matrix* noalias nocapture) #1 {
entry:
  %1 = getelementptr inbounds %u32Matrix, %u32Matrix* %0, i64 0, i32 4
  %len = load i32, i32* %1, align 4, !range !0
  %2 = zext i32 %len to i64
  %3 = alloca { %u32Matrix*, i32, i64, i64, i64, i64 }, align 8
  %4 = getelementptr inbounds { %u32Matrix*, i32, i64, i64, i64, i64 }, { %u32Matrix*, i32, i64, i64, i64, i64 }* %3, i64 0, i32 0
  store %u32Matrix* %0, %u32Matrix** %4, align 8
  %5 = getelementptr inbounds { %u32Matrix*, i32, i64, i64, i64, i64 }, { %u32Matrix*, i32, i64, i64, i64, i64 }* %3, i64 0, i32 1
  store i32 %len, i32* %5, align 8
  %6 = getelementptr inbounds { %u32Matrix*, i32, i64, i64, i64, i64 }, { %u32Matrix*, i32, i64, i64, i64, i64 }* %3, i64 0, i32 2
  store i64 1, i64* %6, align 4
  %7 = getelementptr inbounds { %u32Matrix*, i32, i64, i64, i64, i64 }, { %u32Matrix*, i32, i64, i64, i64, i64 }* %3, i64 0, i32 3
  store i64 1, i64* %7, align 4
  %8 = getelementptr inbounds { %u32Matrix*, i32, i64, i64, i64, i64 }, { %u32Matrix*, i32, i64, i64, i64, i64 }* %3, i64 0, i32 4
  store i64 %2, i64* %8, align 4
  %9 = getelementptr inbounds { %u32Matrix*, i32, i64, i64, i64, i64 }, { %u32Matrix*, i32, i64, i64, i64, i64 }* %3, i64 0, i32 5
  store i64 1, i64* %9, align 4
  %10 = bitcast { %u32Matrix*, i32, i64, i64, i64, i64 }* %3 to i8*
  call void @likely_fork(i8* bitcast (void ({ %u32Matrix*, i32, i64, i64, i64, i64 }*, i64, i64)* @sort_tmp_thunk0 to i8*), i8* %10, i64 %2) #1
  %11 = bitcast %u32Matrix* %0 to i8*
  %12 = call i8* @likely_retain_mat(i8* %11) #1
  %13 = bitcast i8* %12 to %u32Matrix*
  ret %u32Matrix* %13
}

declare i8* @likely_retain_mat(i8* noalias nocapture)

attributes #0 = { norecurse nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
