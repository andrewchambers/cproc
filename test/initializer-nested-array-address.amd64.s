	.data
	.globl y
	.balign 8
y:
	.quad x+20
	.data
	.globl x
	.balign 4
x:
	.zero 24
	.section .note.GNU-stack,"",@progbits
