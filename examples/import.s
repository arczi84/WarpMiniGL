.text
.even
.globl _test_region
_test_region:
 move.l a6,-(sp)
 move.l _Warp3DBase,a6
 jsr -192(a6)
 move.l (sp)+,a6
 rts
.globl _test_create
_test_create:
 move.l a6,-(sp)
 move.l _Warp3DBase,a6
 jsr -30(a6)
 move.l (sp)+,a6
 rts
.globl _test_destroy
_test_destroy:
 move.l a6,-(sp)
 move.l _Warp3DBase,a6
 jsr -36(a6)
 move.l (sp)+,a6
 rts
.globl _test_lock
_test_lock:
 move.l a6,-(sp)
 move.l _Warp3DBase,a6
 jsr -60(a6)
 move.l (sp)+,a6
 rts
.globl _test_unlock
_test_unlock:
 move.l a6,-(sp)
 move.l _Warp3DBase,a6
 jsr -66(a6)
 move.l (sp)+,a6
 rts
.globl _test_state
_test_state:
 move.l a6,-(sp)
 move.l _Warp3DBase,a6
 jsr -48(a6)
 move.l (sp)+,a6
 rts
.globl _test_clear
_test_clear:
 move.l a6,-(sp)
 move.l _Warp3DBase,a6
 jsr -450(a6)
 move.l (sp)+,a6
 rts
.globl _test_triangle
_test_triangle:
 move.l a6,-(sp)
 move.l _Warp3DBase,a6
 jsr -162(a6)
 move.l (sp)+,a6
 rts
.globl _test_status
_test_status:
 move.l a6,-(sp)
 move.l _Warp3DBase,a6
 jsr -306(a6)
 move.l (sp)+,a6
 rts
