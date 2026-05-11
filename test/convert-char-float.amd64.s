	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
.LB2:
	movq $256, %rax
	leaq -1(%rbp), %r11
	movb %al, (%r11)
	leaq -1(%rbp), %r11
	movzbq (%r11), %rax
	cvtsi2ssq %rax, %xmm0
	leaq -8(%rbp), %r11
	movss %xmm0, (%r11)
	movq $0, %rax
	cvtsi2ssq %rax, %xmm0
	leaq -12(%rbp), %r11
	movss %xmm0, (%r11)
	movss -8(%rbp), %xmm0
	movss -12(%rbp), %xmm1
	ucomiss %xmm1, %xmm0
	setne %al
	movzbq %al, %rax
	leaq -16(%rbp), %r11
	movl %eax, (%r11)
	leaq -16(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
