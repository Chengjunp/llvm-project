; RUN: opt < %s -S -mtriple=nvptx-nvidia-cuda -mcpu=sm_20 -passes=nvvm-intr-range | FileCheck %s

; When .reqntid specifies 3D dimensions, ntid.x/y/z should be replaced with
; constants and tid.x/y/z should get per-dimension ranges.
; Product 128*4*2 = 1024 is within the hardware limit.
define ptx_kernel i32 @test_reqntid_3d() "nvvm.reqntid"="128,4,2" {
; CHECK-LABEL: define ptx_kernel i32 @test_reqntid_3d(
; CHECK-NEXT:    [[TID_X:%.*]] = call range(i32 0, 128) i32 @llvm.nvvm.read.ptx.sreg.tid.x()
; CHECK-NEXT:    [[TID_Y:%.*]] = call range(i32 0, 4) i32 @llvm.nvvm.read.ptx.sreg.tid.y()
; CHECK-NEXT:    [[TID_Z:%.*]] = call range(i32 0, 2) i32 @llvm.nvvm.read.ptx.sreg.tid.z()
; CHECK-NEXT:    [[A:%.*]] = add i32 [[TID_X]], [[TID_Y]]
; CHECK-NEXT:    [[B:%.*]] = add i32 [[A]], [[TID_Z]]
; CHECK-NEXT:    [[C:%.*]] = add i32 [[B]], 128
; CHECK-NEXT:    [[D:%.*]] = add i32 [[C]], 4
; CHECK-NEXT:    [[E:%.*]] = add i32 [[D]], 2
; CHECK-NEXT:    ret i32 [[E]]
;
  %tid.x = call i32 @llvm.nvvm.read.ptx.sreg.tid.x()
  %tid.y = call i32 @llvm.nvvm.read.ptx.sreg.tid.y()
  %tid.z = call i32 @llvm.nvvm.read.ptx.sreg.tid.z()
  %ntid.x = call i32 @llvm.nvvm.read.ptx.sreg.ntid.x()
  %ntid.y = call i32 @llvm.nvvm.read.ptx.sreg.ntid.y()
  %ntid.z = call i32 @llvm.nvvm.read.ptx.sreg.ntid.z()
  %a = add i32 %tid.x, %tid.y
  %b = add i32 %a, %tid.z
  %c = add i32 %b, %ntid.x
  %d = add i32 %c, %ntid.y
  %e = add i32 %d, %ntid.z
  ret i32 %e
}

; When .reqntid specifies only 1D, y and z default to 1.
define ptx_kernel i32 @test_reqntid_1d() "nvvm.reqntid"="128" {
; CHECK-LABEL: define ptx_kernel i32 @test_reqntid_1d(
; CHECK-NEXT:    [[TID_X:%.*]] = call range(i32 0, 128) i32 @llvm.nvvm.read.ptx.sreg.tid.x()
; CHECK-NEXT:    [[TID_Y:%.*]] = call range(i32 0, 1) i32 @llvm.nvvm.read.ptx.sreg.tid.y()
; CHECK-NEXT:    [[TID_Z:%.*]] = call range(i32 0, 1) i32 @llvm.nvvm.read.ptx.sreg.tid.z()
; CHECK-NEXT:    [[A:%.*]] = add i32 [[TID_X]], [[TID_Y]]
; CHECK-NEXT:    [[B:%.*]] = add i32 [[A]], [[TID_Z]]
; CHECK-NEXT:    [[C:%.*]] = add i32 [[B]], 128
; CHECK-NEXT:    [[D:%.*]] = add i32 [[C]], 1
; CHECK-NEXT:    [[E:%.*]] = add i32 [[D]], 1
; CHECK-NEXT:    ret i32 [[E]]
;
  %tid.x = call i32 @llvm.nvvm.read.ptx.sreg.tid.x()
  %tid.y = call i32 @llvm.nvvm.read.ptx.sreg.tid.y()
  %tid.z = call i32 @llvm.nvvm.read.ptx.sreg.tid.z()
  %ntid.x = call i32 @llvm.nvvm.read.ptx.sreg.ntid.x()
  %ntid.y = call i32 @llvm.nvvm.read.ptx.sreg.ntid.y()
  %ntid.z = call i32 @llvm.nvvm.read.ptx.sreg.ntid.z()
  %a = add i32 %tid.x, %tid.y
  %b = add i32 %a, %tid.z
  %c = add i32 %b, %ntid.x
  %d = add i32 %c, %ntid.y
  %e = add i32 %d, %ntid.z
  ret i32 %e
}

; When .reqntid exceeds hardware limits, no folding — fall back to range attrs.
define ptx_kernel i32 @test_reqntid_invalid() "nvvm.reqntid"="2048" {
; CHECK-LABEL: define ptx_kernel i32 @test_reqntid_invalid(
; CHECK-NEXT:    [[TID_X:%.*]] = call range(i32 0, 1024) i32 @llvm.nvvm.read.ptx.sreg.tid.x()
; CHECK-NEXT:    [[NTID_X:%.*]] = call range(i32 1, 1025) i32 @llvm.nvvm.read.ptx.sreg.ntid.x()
; CHECK-NEXT:    [[A:%.*]] = add i32 [[TID_X]], [[NTID_X]]
; CHECK-NEXT:    ret i32 [[A]]
;
  %tid.x = call i32 @llvm.nvvm.read.ptx.sreg.tid.x()
  %ntid.x = call i32 @llvm.nvvm.read.ptx.sreg.ntid.x()
  %a = add i32 %tid.x, %ntid.x
  ret i32 %a
}

declare i32 @llvm.nvvm.read.ptx.sreg.tid.x()
declare i32 @llvm.nvvm.read.ptx.sreg.tid.y()
declare i32 @llvm.nvvm.read.ptx.sreg.tid.z()
declare i32 @llvm.nvvm.read.ptx.sreg.ntid.x()
declare i32 @llvm.nvvm.read.ptx.sreg.ntid.y()
declare i32 @llvm.nvvm.read.ptx.sreg.ntid.z()
