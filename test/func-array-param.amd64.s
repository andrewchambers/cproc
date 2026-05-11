	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $32, %rsp
.LB1:
	movq %rdi, %rax
	leaq -8(%rbp), %r11
	movq %rax, (%r11)
	movq %rsi, %rax
	leaq -16(%rbp), %r11
	movq %rax, (%r11)
	movq %rdx, %rax
	leaq -24(%rbp), %r11
	movq %rax, (%r11)
	movq %rcx, %rax
	leaq -32(%rbp), %r11
	movq %rax, (%r11)
.LB2:
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
