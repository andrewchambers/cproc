	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
.LB1:
.LB2:
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
