	.data
	.globl x
	.balign 4
x:
	.zero 4
	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
.LB2:
	xorq %rax, %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
