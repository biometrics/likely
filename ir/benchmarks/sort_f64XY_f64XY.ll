; ModuleID = 'likely'
source_filename = "likely"

%f64Matrix = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: norecurse nounwind
define noalias %f64Matrix* @sort(%f64Matrix* noalias nocapture) #0 {
entry:
  br label %entry.split

entry.split:                                      ; preds = %entry
  %1 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 4
  %len = load i32, i32* %1, align 4, !range !0
  %2 = zext i32 %len to i64
  %3 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 3
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
  %8 = getelementptr %f64Matrix, %f64Matrix* %0, i64 0, i32 6, i64 %7
  %9 = load double, double* %8, align 8, !llvm.mem.parallel_loop_access !1
  %10 = add nuw nsw i32 %5, 1
  %11 = icmp eq i32 %10, %len
  br i1 %11, label %exit5, label %true_entry4

true_entry4:                                      ; preds = %true_entry, %true_entry4
  %12 = phi i32 [ %21, %true_entry4 ], [ %10, %true_entry ]
  %13 = phi i32 [ %20, %true_entry4 ], [ %5, %true_entry ]
  %14 = phi double [ %19, %true_entry4 ], [ %9, %true_entry ]
  %15 = sext i32 %12 to i64
  %16 = add nuw nsw i64 %15, %4
  %17 = getelementptr %f64Matrix, %f64Matrix* %0, i64 0, i32 6, i64 %16
  %element = load double, double* %17, align 8, !llvm.mem.parallel_loop_access !1
  %18 = fcmp fast olt double %element, %14
  %19 = select i1 %18, double %element, double %14
  %20 = select i1 %18, i32 %12, i32 %13
  %21 = add nuw nsw i32 %12, 1
  %22 = icmp eq i32 %21, %len
  br i1 %22, label %exit5, label %true_entry4

exit5:                                            ; preds = %true_entry4, %true_entry
  %.lcssa = phi i32 [ %5, %true_entry ], [ %20, %true_entry4 ]
  %23 = icmp eq i32 %5, %.lcssa
  br i1 %23, label %loop.backedge, label %true_entry8

loop.backedge:                                    ; preds = %exit5, %true_entry8
  br i1 %11, label %exit, label %true_entry

true_entry8:                                      ; preds = %exit5
  %24 = zext i32 %.lcssa to i64
  %25 = add nuw nsw i64 %24, %4
  %26 = getelementptr %f64Matrix, %f64Matrix* %0, i64 0, i32 6, i64 %25
  %27 = load double, double* %26, align 8, !llvm.mem.parallel_loop_access !1
  store double %27, double* %8, align 8, !llvm.mem.parallel_loop_access !1
  store double %9, double* %26, align 8, !llvm.mem.parallel_loop_access !1
  br label %loop.backedge

exit:                                             ; preds = %loop.backedge
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %exit
  %28 = bitcast %f64Matrix* %0 to i8*
  %29 = call i8* @likely_retain_mat(i8* %28) #1
  %30 = bitcast i8* %29 to %f64Matrix*
  ret %f64Matrix* %30
}

declare i8* @likely_retain_mat(i8* noalias nocapture)

attributes #0 = { norecurse nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
