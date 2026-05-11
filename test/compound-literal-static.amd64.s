	.data
	.balign 4
.L.1:
	.long 2
	.data
	.globl x
	.balign 8
x:
	.quad .L.1
	.section .note.GNU-stack,"",@progbits
