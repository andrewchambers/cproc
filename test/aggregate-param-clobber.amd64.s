	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
	movq %rdi, %rax
	movq %rax, %r10
	leaq -4(%rbp), %rdi
	movl 0(%r10), %eax
	movl %eax, 0(%rdi)
	movq %rsi, %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
.LB2:
	leaq -8(%rbp), %r11
	movslq (%r11), %rax
	leaq -12(%rbp), %r11
	movl %eax, (%r11)
	leaq -12(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
