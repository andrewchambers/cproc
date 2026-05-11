	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
	movq %rdi, %rax
	leaq -8(%rbp), %r11
	movq %rax, (%r11)
.LB2:
	movq $2, %rax
	leaq -12(%rbp), %r11
	movl %eax, (%r11)
	leaq -8(%rbp), %r11
	movq (%r11), %rdi
	leaq -12(%rbp), %rsi
	movl 0(%rsi), %eax
	movl %eax, 0(%rdi)
	leaq -8(%rbp), %r11
	movq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
