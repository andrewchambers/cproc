	.data
	.globl p
	.balign 8
p:
	.quad x
	.data
	.globl x
	.balign 4
x:
	.zero 4
	.section .note.GNU-stack,"",@progbits
