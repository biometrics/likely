; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

define %f64XY* @filter_2D(%f64XY*, %f64XY*) {
entry:
  %2 = getelementptr inbounds %f64XY, %f64XY* %1, i64 0, i32 3
  %width = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f64XY, %f64XY* %1, i64 0, i32 4
  %height = load i32, i32* %3, align 4, !range !0
  %4 = srem i32 %width, 2
  %5 = icmp eq i32 %4, 1
  call void @llvm.assume(i1 %5)
  %6 = srem i32 %height, 2
  %7 = icmp eq i32 %6, 1
  call void @llvm.assume(i1 %7)
  %8 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 3
  %columns = load i32, i32* %8, align 4, !range !0
  %9 = add i32 %width, -1
  %10 = add nuw nsw i32 %columns, %9
  %11 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 4
  %rows = load i32, i32* %11, align 4, !range !0
  %12 = add i32 %height, -1
  %13 = add nuw nsw i32 %rows, %12
  %14 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %10, i32 %13, i32 1, i8* null)
  %15 = zext i32 %13 to i64
  %mat_y_step = zext i32 %10 to i64
  %16 = getelementptr inbounds %u0CXYT, %u0CXYT* %14, i64 1
  %17 = bitcast %u0CXYT* %16 to double*
  %18 = ptrtoint %u0CXYT* %16 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  %scevgep7 = getelementptr %u0CXYT, %u0CXYT* %14, i64 1, i32 0
  %21 = add i32 %width, -1
  %22 = add i32 %21, %columns
  %23 = zext i32 %22 to i64
  %24 = shl nuw nsw i64 %23, 1
  %25 = shl nuw nsw i64 %23, 3
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %26 = mul i64 %y, %24
  %scevgep8 = getelementptr i32, i32* %scevgep7, i64 %26
  %scevgep89 = bitcast i32* %scevgep8 to i8*
  call void @llvm.memset.p0i8.i64(i8* %scevgep89, i8 0, i64 %25, i32 8, i1 false)
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %15
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %pad-columns = sdiv i32 %9, 2
  %pad-rows = sdiv i32 %12, 2
  %27 = zext i32 %rows to i64
  %src_y_step = zext i32 %columns to i64
  %28 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 6, i64 0
  %29 = ptrtoint double* %28 to i64
  %30 = and i64 %29, 31
  %31 = icmp eq i64 %30, 0
  call void @llvm.assume(i1 %31)
  %32 = sext i32 %pad-rows to i64
  %33 = mul nsw i64 %23, %32
  %34 = sext i32 %pad-columns to i64
  %35 = add i64 %33, %34
  %36 = mul i64 %35, 2
  %scevgep4 = getelementptr %f64XY, %f64XY* %0, i64 1, i32 0
  %37 = mul nuw nsw i64 %src_y_step, 2
  %38 = shl nuw nsw i64 %src_y_step, 3
  br label %y_body9

y_body9:                                          ; preds = %y_body9, %y_exit
  %y11 = phi i64 [ 0, %y_exit ], [ %y_increment17, %y_body9 ]
  %39 = mul i64 %y11, %24
  %40 = add i64 %39, %36
  %scevgep2 = getelementptr i32, i32* %scevgep7, i64 %40
  %scevgep23 = bitcast i32* %scevgep2 to i8*
  %41 = mul i64 %37, %y11
  %scevgep5 = getelementptr i32, i32* %scevgep4, i64 %41
  %scevgep56 = bitcast i32* %scevgep5 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep23, i8* %scevgep56, i64 %38, i32 8, i1 false)
  %y_increment17 = add nuw nsw i64 %y11, 1
  %y_postcondition18 = icmp eq i64 %y_increment17, %27
  br i1 %y_postcondition18, label %y_exit10, label %y_body9

y_exit10:                                         ; preds = %y_body9
  %42 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %43 = getelementptr inbounds %u0CXYT, %u0CXYT* %42, i64 1
  %44 = bitcast %u0CXYT* %43 to double*
  %45 = ptrtoint %u0CXYT* %43 to i64
  %46 = and i64 %45, 31
  %47 = icmp eq i64 %46, 0
  call void @llvm.assume(i1 %47)
  %kernel_y_step = zext i32 %width to i64
  %48 = getelementptr inbounds %f64XY, %f64XY* %1, i64 0, i32 6, i64 0
  %49 = ptrtoint double* %48 to i64
  %50 = and i64 %49, 31
  %51 = icmp eq i64 %50, 0
  call void @llvm.assume(i1 %51)
  br label %y_body31

y_body31:                                         ; preds = %x_exit35, %y_exit10
  %y33 = phi i64 [ 0, %y_exit10 ], [ %y_increment43, %x_exit35 ]
  %52 = mul nuw nsw i64 %y33, %src_y_step
  br label %x_body34

x_body34:                                         ; preds = %y_body31, %exit
  %x36 = phi i64 [ %x_increment41, %exit ], [ 0, %y_body31 ]
  %53 = add nuw nsw i64 %x36, %52
  br label %loop38.preheader

loop38.preheader:                                 ; preds = %x_body34, %exit40
  %54 = phi i32 [ %76, %exit40 ], [ 0, %x_body34 ]
  %55 = phi double [ %73, %exit40 ], [ 0.000000e+00, %x_body34 ]
  %56 = sext i32 %54 to i64
  %57 = add nuw nsw i64 %56, %y33
  %58 = mul nuw nsw i64 %57, %mat_y_step
  %59 = add i64 %58, %x36
  %60 = mul nuw nsw i64 %56, %kernel_y_step
  br label %true_entry39

exit:                                             ; preds = %exit40
  %61 = getelementptr double, double* %44, i64 %53
  store double %73, double* %61, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment41 = add nuw nsw i64 %x36, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %src_y_step
  br i1 %x_postcondition42, label %x_exit35, label %x_body34

x_exit35:                                         ; preds = %exit
  %y_increment43 = add nuw nsw i64 %y33, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %27
  br i1 %y_postcondition44, label %y_exit32, label %y_body31

y_exit32:                                         ; preds = %x_exit35
  %dst = bitcast %u0CXYT* %42 to %f64XY*
  %62 = bitcast %u0CXYT* %14 to i8*
  call void @likely_release_mat(i8* %62)
  ret %f64XY* %dst

true_entry39:                                     ; preds = %loop38.preheader, %true_entry39
  %63 = phi double [ %73, %true_entry39 ], [ %55, %loop38.preheader ]
  %64 = phi i32 [ %74, %true_entry39 ], [ 0, %loop38.preheader ]
  %65 = sext i32 %64 to i64
  %66 = add i64 %59, %65
  %67 = getelementptr double, double* %17, i64 %66
  %68 = load double, double* %67, align 8, !llvm.mem.parallel_loop_access !1
  %69 = add nuw nsw i64 %65, %60
  %70 = getelementptr %f64XY, %f64XY* %1, i64 0, i32 6, i64 %69
  %71 = load double, double* %70, align 8, !llvm.mem.parallel_loop_access !1
  %72 = fmul fast double %71, %68
  %73 = fadd fast double %72, %63
  %74 = add nuw nsw i32 %64, 1
  %75 = icmp eq i32 %74, %width
  br i1 %75, label %exit40, label %true_entry39

exit40:                                           ; preds = %true_entry39
  %76 = add nuw nsw i32 %54, 1
  %77 = icmp eq i32 %76, %height
  br i1 %77, label %exit, label %loop38.preheader
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1) #1

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #1

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind }
attributes #1 = { argmemonly nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
