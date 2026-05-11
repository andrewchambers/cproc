	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $64, %rsp
.LB1:
.LB2:
	movq $1, %rax
	cvtsi2sdq %rax, %xmm0
	leaq -16(%rbp), %r11
	movsd %xmm0, (%r11)
	movsd -16(%rbp), %xmm0
	leaq -8(%rbp), %r11
	movsd %xmm0, (%r11)
.LB3:
	leaq -8(%rbp), %r11
	movsd (%r11), %xmm0
	leaq -24(%rbp), %r11
	movsd %xmm0, (%r11)
	movsd -24(%rbp), %xmm0
	pxor %xmm1, %xmm1
	ucomisd %xmm1, %xmm0
	jne .LB4
	jmp .LB5
.LB4:
	leaq -8(%rbp), %r11
	movsd (%r11), %xmm0
	leaq -32(%rbp), %r11
	movsd %xmm0, (%r11)
	movq $2, %rax
	cvtsi2sdq %rax, %xmm0
	leaq -40(%rbp), %r11
	movsd %xmm0, (%r11)
	movsd -32(%rbp), %xmm0
	movsd -40(%rbp), %xmm1
	divsd %xmm1, %xmm0
	leaq -48(%rbp), %r11
	movsd %xmm0, (%r11)
	movsd -48(%rbp), %xmm0
	leaq -8(%rbp), %r11
	movsd %xmm0, (%r11)
	jmp .LB3
.LB5:
	leaq -8(%rbp), %r11
	movsd (%r11), %xmm0
	leaq -56(%rbp), %r11
	movsd %xmm0, (%r11)
	movsd -56(%rbp), %xmm0
	cvttsd2sil %xmm0, %eax
	leaq -60(%rbp), %r11
	movl %eax, (%r11)
	leaq -60(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
