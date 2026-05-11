	.text
	.globl f2
	.balign 16
f2:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
.LB2:
	movq $12, %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	movq $6, %rax
	movq $2, %rcx
	imulq %rcx, %rax
	leaq -12(%rbp), %r11
	movl %eax, (%r11)
	leaq -12(%rbp), %r11
	movslq (%r11), %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
