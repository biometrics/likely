; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f32XY* @covariance(%f32XY*) {
entry:
  %1 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 8480, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %6 = bitcast %u0CXYT* %5 to float*
  %7 = ptrtoint %u0CXYT* %5 to i64
  %8 = and i64 %7, 31
  %9 = icmp eq i64 %8, 0
  call void @llvm.assume(i1 %9)
  %scevgep67 = bitcast %u0CXYT* %5 to i8*
  %10 = shl nuw nsw i64 %4, 2
  call void @llvm.memset.p0i8.i64(i8* %scevgep67, i8 0, i64 %10, i32 4, i1 false)
  %11 = zext i32 %rows to i64
  %12 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 6, i64 0
  %13 = ptrtoint float* %12 to i64
  %14 = and i64 %13, 31
  %15 = icmp eq i64 %14, 0
  call void @llvm.assume(i1 %15)
  br label %y_body

y_body:                                           ; preds = %x_exit8, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit8 ]
  %16 = mul nuw nsw i64 %y, %4
  br label %x_body7

x_body7:                                          ; preds = %y_body, %x_body7
  %x9 = phi i64 [ %x_increment10, %x_body7 ], [ 0, %y_body ]
  %17 = getelementptr float, float* %6, i64 %x9
  %18 = load float, float* %17, align 4
  %19 = add nuw nsw i64 %x9, %16
  %20 = getelementptr %f32XY, %f32XY* %0, i64 0, i32 6, i64 %19
  %21 = load float, float* %20, align 4
  %22 = fadd fast float %21, %18
  store float %22, float* %17, align 4
  %x_increment10 = add nuw nsw i64 %x9, 1
  %x_postcondition11 = icmp eq i64 %x_increment10, %4
  br i1 %x_postcondition11, label %x_exit8, label %x_body7

x_exit8:                                          ; preds = %x_body7
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %11
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit8
  %23 = icmp eq i32 %rows, 1
  br i1 %23, label %Flow9, label %true_entry

true_entry:                                       ; preds = %y_exit
  %24 = uitofp i32 %rows to float
  %25 = fdiv fast float 1.000000e+00, %24
  br label %x_body15

Flow9:                                            ; preds = %x_body15, %y_exit
  %26 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %27 = getelementptr inbounds %u0CXYT, %u0CXYT* %26, i64 1
  %28 = ptrtoint %u0CXYT* %27 to i64
  %29 = and i64 %28, 31
  %30 = icmp eq i64 %29, 0
  call void @llvm.assume(i1 %30)
  %scevgep = getelementptr %u0CXYT, %u0CXYT* %26, i64 1, i32 0
  %scevgep3 = getelementptr %f32XY, %f32XY* %0, i64 1, i32 0
  br label %y_body30

x_body15:                                         ; preds = %true_entry, %x_body15
  %x17 = phi i64 [ %x_increment18, %x_body15 ], [ 0, %true_entry ]
  %31 = getelementptr float, float* %6, i64 %x17
  %32 = load float, float* %31, align 4, !llvm.mem.parallel_loop_access !1
  %33 = fmul fast float %32, %25
  store float %33, float* %31, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment18 = add nuw nsw i64 %x17, 1
  %x_postcondition19 = icmp eq i64 %x_increment18, %4
  br i1 %x_postcondition19, label %Flow9, label %x_body15

y_body30:                                         ; preds = %y_body30, %Flow9
  %y32 = phi i64 [ 0, %Flow9 ], [ %y_increment38, %y_body30 ]
  %34 = mul i64 %y32, %4
  %scevgep1 = getelementptr i32, i32* %scevgep, i64 %34
  %scevgep12 = bitcast i32* %scevgep1 to i8*
  %scevgep4 = getelementptr i32, i32* %scevgep3, i64 %34
  %scevgep45 = bitcast i32* %scevgep4 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep12, i8* %scevgep45, i64 %10, i32 4, i1 false)
  %y_increment38 = add nuw nsw i64 %y32, 1
  %y_postcondition39 = icmp eq i64 %y_increment38, %11
  br i1 %y_postcondition39, label %y_body45.preheader, label %y_body30

y_body45.preheader:                               ; preds = %y_body30
  %35 = bitcast %u0CXYT* %27 to float*
  br label %y_body45

y_body45:                                         ; preds = %x_exit49, %y_body45.preheader
  %y47 = phi i64 [ 0, %y_body45.preheader ], [ %y_increment53, %x_exit49 ]
  %36 = mul nuw nsw i64 %y47, %4
  br label %x_body48

x_body48:                                         ; preds = %y_body45, %x_body48
  %x50 = phi i64 [ %x_increment51, %x_body48 ], [ 0, %y_body45 ]
  %37 = add nuw nsw i64 %x50, %36
  %38 = getelementptr float, float* %35, i64 %37
  %39 = load float, float* %38, align 4, !llvm.mem.parallel_loop_access !2
  %40 = getelementptr float, float* %6, i64 %x50
  %41 = load float, float* %40, align 4, !llvm.mem.parallel_loop_access !2
  %42 = fsub fast float %39, %41
  store float %42, float* %38, align 4, !llvm.mem.parallel_loop_access !2
  %x_increment51 = add nuw nsw i64 %x50, 1
  %x_postcondition52 = icmp eq i64 %x_increment51, %4
  br i1 %x_postcondition52, label %x_exit49, label %x_body48

x_exit49:                                         ; preds = %x_body48
  %y_increment53 = add nuw nsw i64 %y47, 1
  %y_postcondition54 = icmp eq i64 %y_increment53, %11
  br i1 %y_postcondition54, label %y_exit46, label %y_body45

y_exit46:                                         ; preds = %x_exit49
  %43 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %44 = getelementptr inbounds %u0CXYT, %u0CXYT* %43, i64 1
  %45 = bitcast %u0CXYT* %44 to float*
  %46 = ptrtoint %u0CXYT* %44 to i64
  %47 = and i64 %46, 31
  %48 = icmp eq i64 %47, 0
  call void @llvm.assume(i1 %48)
  br label %y_body66

y_body66:                                         ; preds = %x_exit70, %y_exit46
  %y68 = phi i64 [ 0, %y_exit46 ], [ %y_increment78, %x_exit70 ]
  %49 = mul nuw nsw i64 %y68, %4
  br label %x_body69

x_body69:                                         ; preds = %y_body66, %Flow
  %x71 = phi i64 [ %x_increment76, %Flow ], [ 0, %y_body66 ]
  %50 = icmp ugt i64 %y68, %x71
  br i1 %50, label %Flow, label %true_entry74

x_exit70:                                         ; preds = %Flow
  %y_increment78 = add nuw nsw i64 %y68, 1
  %y_postcondition79 = icmp eq i64 %y_increment78, %4
  br i1 %y_postcondition79, label %y_exit67, label %y_body66

y_exit67:                                         ; preds = %x_exit70
  %dst = bitcast %u0CXYT* %43 to %f32XY*
  %51 = bitcast %u0CXYT* %2 to i8*
  call void @likely_release_mat(i8* %51)
  %52 = bitcast %u0CXYT* %26 to i8*
  call void @likely_release_mat(i8* %52)
  ret %f32XY* %dst

true_entry74:                                     ; preds = %x_body69, %true_entry74
  %53 = phi i32 [ %67, %true_entry74 ], [ 0, %x_body69 ]
  %54 = phi double [ %66, %true_entry74 ], [ 0.000000e+00, %x_body69 ]
  %55 = sext i32 %53 to i64
  %56 = mul nuw nsw i64 %55, %4
  %57 = add nuw nsw i64 %56, %x71
  %58 = getelementptr float, float* %35, i64 %57
  %59 = load float, float* %58, align 4, !llvm.mem.parallel_loop_access !3
  %60 = fpext float %59 to double
  %61 = add nuw nsw i64 %56, %y68
  %62 = getelementptr float, float* %35, i64 %61
  %63 = load float, float* %62, align 4, !llvm.mem.parallel_loop_access !3
  %64 = fpext float %63 to double
  %65 = fmul fast double %64, %60
  %66 = fadd fast double %65, %54
  %67 = add nuw nsw i32 %53, 1
  %68 = icmp eq i32 %67, %rows
  br i1 %68, label %exit75, label %true_entry74

Flow:                                             ; preds = %x_body69, %exit75
  %x_increment76 = add nuw nsw i64 %x71, 1
  %x_postcondition77 = icmp eq i64 %x_increment76, %4
  br i1 %x_postcondition77, label %x_exit70, label %x_body69

exit75:                                           ; preds = %true_entry74
  %69 = add nuw nsw i64 %x71, %49
  %70 = getelementptr float, float* %45, i64 %69
  %71 = fptrunc double %66 to float
  store float %71, float* %70, align 4, !llvm.mem.parallel_loop_access !3
  %72 = mul nuw nsw i64 %x71, %4
  %73 = add nuw nsw i64 %72, %y68
  %74 = getelementptr float, float* %45, i64 %73
  store float %71, float* %74, align 4, !llvm.mem.parallel_loop_access !3
  br label %Flow
}

; Function Attrs: nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1) #1

; Function Attrs: nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #1

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
