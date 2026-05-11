	.data
	.globl y
	.balign 4
y:
	.long 2
	.text
	.globl g
	.balign 16
g:
	pushq %rbp
	movq %rsp, %rbp
.LB1:
.LB2:
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
