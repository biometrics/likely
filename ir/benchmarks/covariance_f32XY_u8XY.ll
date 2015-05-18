; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%u8XY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f32XY* @covariance(%u8XY*) {
entry:
  %1 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 8480, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = zext i32 %columns to i64
  %4 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %5 = bitcast %u0CXYT* %4 to float*
  %6 = ptrtoint %u0CXYT* %4 to i64
  %7 = and i64 %6, 31
  %8 = icmp eq i64 %7, 0
  call void @llvm.assume(i1 %8)
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ 0, %entry ], [ %x_increment, %x_body ]
  %9 = getelementptr float, float* %5, i64 %x
  store float 0.000000e+00, float* %9, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %3
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !1

x_exit:                                           ; preds = %x_body
  %10 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 4
  %rows = load i32, i32* %10, align 4, !range !0
  %11 = zext i32 %rows to i64
  %12 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 6, i64 0
  %13 = ptrtoint i8* %12 to i64
  %14 = and i64 %13, 31
  %15 = icmp eq i64 %14, 0
  call void @llvm.assume(i1 %15)
  br label %y_body

y_body:                                           ; preds = %x_exit8, %x_exit
  %y = phi i64 [ 0, %x_exit ], [ %y_increment, %x_exit8 ]
  %16 = mul nuw nsw i64 %y, %3
  br label %x_body7

x_body7:                                          ; preds = %x_body7, %y_body
  %x9 = phi i64 [ 0, %y_body ], [ %x_increment10, %x_body7 ]
  %17 = getelementptr float, float* %5, i64 %x9
  %18 = load float, float* %17, align 4
  %19 = add nuw nsw i64 %x9, %16
  %20 = getelementptr %u8XY, %u8XY* %0, i64 0, i32 6, i64 %19
  %21 = load i8, i8* %20, align 1
  %22 = uitofp i8 %21 to float
  %23 = fadd fast float %22, %18
  store float %23, float* %17, align 4
  %x_increment10 = add nuw nsw i64 %x9, 1
  %x_postcondition11 = icmp eq i64 %x_increment10, %3
  br i1 %x_postcondition11, label %x_exit8, label %x_body7

x_exit8:                                          ; preds = %x_body7
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %11
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit8
  %24 = uitofp i32 %rows to float
  %25 = fdiv fast float 1.000000e+00, %24
  br label %x_body16

x_body16:                                         ; preds = %x_body16, %y_exit
  %x18 = phi i64 [ 0, %y_exit ], [ %x_increment19, %x_body16 ]
  %26 = getelementptr float, float* %5, i64 %x18
  %27 = load float, float* %26, align 4, !llvm.mem.parallel_loop_access !2
  %28 = fmul fast float %27, %25
  store float %28, float* %26, align 4, !llvm.mem.parallel_loop_access !2
  %x_increment19 = add nuw nsw i64 %x18, 1
  %x_postcondition20 = icmp eq i64 %x_increment19, %3
  br i1 %x_postcondition20, label %x_exit17, label %x_body16, !llvm.loop !2

x_exit17:                                         ; preds = %x_body16
  %29 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %30 = getelementptr inbounds %u0CXYT, %u0CXYT* %29, i64 1
  %31 = bitcast %u0CXYT* %30 to float*
  %32 = ptrtoint %u0CXYT* %30 to i64
  %33 = and i64 %32, 31
  %34 = icmp eq i64 %33, 0
  call void @llvm.assume(i1 %34)
  br label %y_body32

y_body32:                                         ; preds = %x_exit36, %x_exit17
  %y34 = phi i64 [ 0, %x_exit17 ], [ %y_increment40, %x_exit36 ]
  %35 = mul nuw nsw i64 %y34, %3
  br label %x_body35

x_body35:                                         ; preds = %x_body35, %y_body32
  %x37 = phi i64 [ 0, %y_body32 ], [ %x_increment38, %x_body35 ]
  %36 = add nuw nsw i64 %x37, %35
  %37 = getelementptr %u8XY, %u8XY* %0, i64 0, i32 6, i64 %36
  %38 = load i8, i8* %37, align 1, !llvm.mem.parallel_loop_access !3
  %39 = getelementptr float, float* %5, i64 %x37
  %40 = load float, float* %39, align 4, !llvm.mem.parallel_loop_access !3
  %41 = uitofp i8 %38 to float
  %42 = fsub fast float %41, %40
  %43 = getelementptr float, float* %31, i64 %36
  store float %42, float* %43, align 4, !llvm.mem.parallel_loop_access !3
  %x_increment38 = add nuw nsw i64 %x37, 1
  %x_postcondition39 = icmp eq i64 %x_increment38, %3
  br i1 %x_postcondition39, label %x_exit36, label %x_body35, !llvm.loop !3

x_exit36:                                         ; preds = %x_body35
  %y_increment40 = add nuw nsw i64 %y34, 1
  %y_postcondition41 = icmp eq i64 %y_increment40, %11
  br i1 %y_postcondition41, label %y_exit33, label %y_body32

y_exit33:                                         ; preds = %x_exit36
  %44 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %45 = getelementptr inbounds %u0CXYT, %u0CXYT* %44, i64 1
  %46 = bitcast %u0CXYT* %45 to float*
  %47 = ptrtoint %u0CXYT* %45 to i64
  %48 = and i64 %47, 31
  %49 = icmp eq i64 %48, 0
  call void @llvm.assume(i1 %49)
  br label %y_body53

y_body53:                                         ; preds = %x_exit57, %y_exit33
  %y55 = phi i64 [ 0, %y_exit33 ], [ %y_increment63, %x_exit57 ]
  %50 = mul nuw nsw i64 %y55, %3
  br label %x_body56

x_body56:                                         ; preds = %exit, %y_body53
  %x58 = phi i64 [ 0, %y_body53 ], [ %x_increment61, %exit ]
  %51 = icmp ugt i64 %y55, %x58
  br i1 %51, label %exit, label %true_enry59

exit:                                             ; preds = %x_body56, %exit60
  %x_increment61 = add nuw nsw i64 %x58, 1
  %x_postcondition62 = icmp eq i64 %x_increment61, %3
  br i1 %x_postcondition62, label %x_exit57, label %x_body56, !llvm.loop !4

x_exit57:                                         ; preds = %exit
  %y_increment63 = add nuw nsw i64 %y55, 1
  %y_postcondition64 = icmp eq i64 %y_increment63, %3
  br i1 %y_postcondition64, label %y_exit54, label %y_body53

y_exit54:                                         ; preds = %x_exit57
  %52 = bitcast %u0CXYT* %44 to %f32XY*
  %53 = bitcast %u0CXYT* %2 to i8*
  call void @likely_release_mat(i8* %53)
  %54 = bitcast %u0CXYT* %29 to i8*
  call void @likely_release_mat(i8* %54)
  ret %f32XY* %52

true_enry59:                                      ; preds = %x_body56, %true_enry59
  %55 = phi i32 [ %69, %true_enry59 ], [ 0, %x_body56 ]
  %56 = phi double [ %68, %true_enry59 ], [ 0.000000e+00, %x_body56 ]
  %57 = sext i32 %55 to i64
  %58 = mul nuw nsw i64 %57, %3
  %59 = add nuw nsw i64 %58, %x58
  %60 = getelementptr float, float* %31, i64 %59
  %61 = load float, float* %60, align 4, !llvm.mem.parallel_loop_access !4
  %62 = fpext float %61 to double
  %63 = add nuw nsw i64 %58, %y55
  %64 = getelementptr float, float* %31, i64 %63
  %65 = load float, float* %64, align 4, !llvm.mem.parallel_loop_access !4
  %66 = fpext float %65 to double
  %67 = fmul fast double %66, %62
  %68 = fadd fast double %67, %56
  %69 = add nuw nsw i32 %55, 1
  %70 = icmp eq i32 %69, %rows
  br i1 %70, label %exit60, label %true_enry59

exit60:                                           ; preds = %true_enry59
  %71 = fptrunc double %68 to float
  %72 = add nuw nsw i64 %x58, %50
  %73 = getelementptr float, float* %46, i64 %72
  store float %71, float* %73, align 4, !llvm.mem.parallel_loop_access !4
  %74 = mul nuw nsw i64 %x58, %3
  %75 = add nuw nsw i64 %74, %y55
  %76 = getelementptr float, float* %46, i64 %75
  store float %71, float* %76, align 4, !llvm.mem.parallel_loop_access !4
  br label %exit
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
!4 = distinct !{!4}
