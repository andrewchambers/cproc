	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
.LB1:
.LB2:
	movq $0, %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
