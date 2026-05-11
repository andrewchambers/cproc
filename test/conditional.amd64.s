	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $32, %rsp
.LB1:
.LB2:
	leaq i(%rip), %r11
	movslq (%r11), %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB5
	jmp .LB6
.LB5:
	movq $1, %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
	jmp .LB7
.LB6:
	movq $0, %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
	jmp .LB7
.LB7:
	leaq -8(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB3
	jmp .LB4
.LB3:
	movq $1, %rax
	jmp .Lret1
.LB4:
	leaq f(%rip), %r11
	movss (%r11), %xmm0
	leaq -12(%rbp), %r11
	movss %xmm0, (%r11)
	movss -12(%rbp), %xmm0
	pxor %xmm1, %xmm1
	ucomiss %xmm1, %xmm0
	jne .LB10
	jmp .LB11
.LB10:
	movq $1, %rax
	leaq -16(%rbp), %r11
	movl %eax, (%r11)
	jmp .LB12
.LB11:
	movq $0, %rax
	leaq -16(%rbp), %r11
	movl %eax, (%r11)
	jmp .LB12
.LB12:
	leaq -16(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB8
	jmp .LB9
.LB8:
	movq $1, %rax
	jmp .Lret1
.LB9:
	leaq p(%rip), %r11
	movq (%r11), %rax
	leaq -24(%rbp), %r11
	movq %rax, (%r11)
	leaq -24(%rbp), %r11
	movq (%r11), %rax
	testq %rax, %rax
	jne .LB15
	jmp .LB16
.LB15:
	movq $1, %rax
	leaq -28(%rbp), %r11
	movl %eax, (%r11)
	jmp .LB17
.LB16:
	movq $0, %rax
	leaq -28(%rbp), %r11
	movl %eax, (%r11)
	jmp .LB17
.LB17:
	leaq -28(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB13
	jmp .LB14
.LB13:
	movq $1, %rax
	jmp .Lret1
.LB14:
	movq $0, %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.data
	.globl i
	.balign 4
i:
	.zero 4
	.data
	.globl f
	.balign 4
f:
	.zero 4
	.data
	.globl p
	.balign 8
p:
	.zero 8
	.section .note.GNU-stack,"",@progbits
