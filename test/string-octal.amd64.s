	.data
	.balign 1
.Lstring.1:
	.byte 1
	.byte 10
	.byte 83
	.byte 83
	.byte 52
	.byte 0
	.data
	.globl s
	.balign 8
s:
	.quad .Lstring.1
	.section .note.GNU-stack,"",@progbits
