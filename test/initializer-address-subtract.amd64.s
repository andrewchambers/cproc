	.data
	.globl p
	.balign 8
p:
	.quad a+4
	.data
	.globl a
	.balign 4
a:
	.zero 16
	.section .note.GNU-stack,"",@progbits
