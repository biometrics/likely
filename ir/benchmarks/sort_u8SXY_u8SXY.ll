; ModuleID = 'likely'
source_filename = "likely"

%u8SXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: norecurse
define %u8SXY* @sort(%u8SXY*) #0 {
entry:
  %1 = getelementptr inbounds %u8SXY, %u8SXY* %0, i64 0, i32 4
  %len = load i32, i32* %1, align 4, !range !0
  %2 = zext i32 %len to i64
  %3 = getelementptr inbounds %u8SXY, %u8SXY* %0, i64 0, i32 3
  %columns = load i32, i32* %3, align 4, !range !0
  %src_y_step = zext i32 %columns to i64
  br label %y_body

y_body:                                           ; preds = %exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %exit ]
  %4 = mul nuw nsw i64 %y, %src_y_step
  br label %true_entry

true_entry:                                       ; preds = %y_body, %loop.backedge
  %5 = phi i32 [ %10, %loop.backedge ], [ 0, %y_body ]
  %6 = sext i32 %5 to i64
  %7 = add nuw nsw i64 %6, %4
  %8 = getelementptr %u8SXY, %u8SXY* %0, i64 0, i32 6, i64 %7
  %9 = load i8, i8* %8, align 1, !llvm.mem.parallel_loop_access !1
  %10 = add nuw nsw i32 %5, 1
  %11 = icmp eq i32 %10, %len
  br i1 %11, label %exit5, label %true_entry4

exit:                                             ; preds = %loop.backedge
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %exit
  %12 = bitcast %u8SXY* %0 to i8*
  %13 = call i8* @likely_retain_mat(i8* %12)
  %14 = bitcast i8* %13 to %u8SXY*
  ret %u8SXY* %14

true_entry4:                                      ; preds = %true_entry, %true_entry4
  %15 = phi i32 [ %22, %true_entry4 ], [ %10, %true_entry ]
  %16 = phi i32 [ %., %true_entry4 ], [ %5, %true_entry ]
  %17 = phi i8 [ %element., %true_entry4 ], [ %9, %true_entry ]
  %18 = sext i32 %15 to i64
  %19 = add nuw nsw i64 %18, %4
  %20 = getelementptr %u8SXY, %u8SXY* %0, i64 0, i32 6, i64 %19
  %element = load i8, i8* %20, align 1, !llvm.mem.parallel_loop_access !1
  %21 = icmp ult i8 %element, %17
  %element. = select i1 %21, i8 %element, i8 %17
  %. = select i1 %21, i32 %15, i32 %16
  %22 = add nuw nsw i32 %15, 1
  %23 = icmp eq i32 %22, %len
  br i1 %23, label %exit5, label %true_entry4

exit5:                                            ; preds = %true_entry4, %true_entry
  %.lcssa = phi i32 [ %5, %true_entry ], [ %., %true_entry4 ]
  %24 = icmp eq i32 %5, %.lcssa
  br i1 %24, label %loop.backedge, label %true_entry8

loop.backedge:                                    ; preds = %exit5, %true_entry8
  br i1 %11, label %exit, label %true_entry

true_entry8:                                      ; preds = %exit5
  %25 = sext i32 %.lcssa to i64
  %26 = add nuw nsw i64 %25, %4
  %27 = getelementptr %u8SXY, %u8SXY* %0, i64 0, i32 6, i64 %26
  %28 = load i8, i8* %27, align 1, !llvm.mem.parallel_loop_access !1
  store i8 %28, i8* %8, align 1, !llvm.mem.parallel_loop_access !1
  store i8 %9, i8* %27, align 1, !llvm.mem.parallel_loop_access !1
  br label %loop.backedge
}

declare i8* @likely_retain_mat(i8* noalias nocapture)

attributes #0 = { norecurse }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
