; ModuleID = 'likely'
source_filename = "likely"

%u32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i32] }

; Function Attrs: norecurse nounwind
define noalias %u32Matrix* @sort(%u32Matrix* noalias nocapture) #0 {
entry:
  br label %entry.split

entry.split:                                      ; preds = %entry
  %1 = getelementptr inbounds %u32Matrix, %u32Matrix* %0, i64 0, i32 4
  %len = load i32, i32* %1, align 4, !range !0
  %2 = zext i32 %len to i64
  %3 = getelementptr inbounds %u32Matrix, %u32Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %3, align 4, !range !0
  %src_y_step = zext i32 %columns to i64
  br label %y_body

y_body:                                           ; preds = %exit, %entry.split
  %y = phi i64 [ 0, %entry.split ], [ %y_increment, %exit ]
  %4 = mul nuw nsw i64 %y, %src_y_step
  br label %true_entry

true_entry:                                       ; preds = %y_body, %loop.backedge
  %5 = phi i32 [ %10, %loop.backedge ], [ 0, %y_body ]
  %6 = zext i32 %5 to i64
  %7 = add nuw nsw i64 %6, %4
  %8 = getelementptr %u32Matrix, %u32Matrix* %0, i64 0, i32 6, i64 %7
  %9 = load i32, i32* %8, align 4, !llvm.mem.parallel_loop_access !1
  %10 = add nuw nsw i32 %5, 1
  %11 = icmp eq i32 %10, %len
  br i1 %11, label %exit5, label %true_entry4

true_entry4:                                      ; preds = %true_entry, %true_entry4
  %12 = phi i32 [ %19, %true_entry4 ], [ %10, %true_entry ]
  %13 = phi i32 [ %., %true_entry4 ], [ %5, %true_entry ]
  %14 = phi i32 [ %element., %true_entry4 ], [ %9, %true_entry ]
  %15 = sext i32 %12 to i64
  %16 = add nuw nsw i64 %15, %4
  %17 = getelementptr %u32Matrix, %u32Matrix* %0, i64 0, i32 6, i64 %16
  %element = load i32, i32* %17, align 4, !llvm.mem.parallel_loop_access !1
  %18 = icmp slt i32 %element, %14
  %element. = select i1 %18, i32 %element, i32 %14
  %. = select i1 %18, i32 %12, i32 %13
  %19 = add nuw nsw i32 %12, 1
  %20 = icmp eq i32 %19, %len
  br i1 %20, label %exit5, label %true_entry4

exit5:                                            ; preds = %true_entry4, %true_entry
  %.lcssa = phi i32 [ %5, %true_entry ], [ %., %true_entry4 ]
  %21 = icmp eq i32 %5, %.lcssa
  br i1 %21, label %loop.backedge, label %true_entry8

loop.backedge:                                    ; preds = %exit5, %true_entry8
  br i1 %11, label %exit, label %true_entry

true_entry8:                                      ; preds = %exit5
  %22 = zext i32 %.lcssa to i64
  %23 = add nuw nsw i64 %22, %4
  %24 = getelementptr %u32Matrix, %u32Matrix* %0, i64 0, i32 6, i64 %23
  %25 = load i32, i32* %24, align 4, !llvm.mem.parallel_loop_access !1
  store i32 %25, i32* %8, align 4, !llvm.mem.parallel_loop_access !1
  store i32 %9, i32* %24, align 4, !llvm.mem.parallel_loop_access !1
  br label %loop.backedge

exit:                                             ; preds = %loop.backedge
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %exit
  %26 = bitcast %u32Matrix* %0 to i8*
  %27 = call i8* @likely_retain_mat(i8* %26) #1
  %28 = bitcast i8* %27 to %u32Matrix*
  ret %u32Matrix* %28
}

declare i8* @likely_retain_mat(i8* noalias nocapture)

attributes #0 = { norecurse nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
