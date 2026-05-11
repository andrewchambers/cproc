	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
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
	jmp .LB4
.LB5:
	movq $1, %rax
	testq %rax, %rax
	jne .LB3
	jmp .LB4
.LB3:
	movq $1, %rax
	jmp .Lret1
.LB4:
	leaq f(%rip), %r11
	movss (%r11), %xmm0
	leaq -8(%rbp), %r11
	movss %xmm0, (%r11)
	movss -8(%rbp), %xmm0
	pxor %xmm1, %xmm1
	ucomiss %xmm1, %xmm0
	jne .LB8
	jmp .LB7
.LB8:
	movq $1, %rax
	testq %rax, %rax
	jne .LB6
	jmp .LB7
.LB6:
	movq $1, %rax
	jmp .Lret1
.LB7:
	leaq p(%rip), %r11
	movq (%r11), %rax
	leaq -16(%rbp), %r11
	movq %rax, (%r11)
	leaq -16(%rbp), %r11
	movq (%r11), %rax
	testq %rax, %rax
	jne .LB11
	jmp .LB10
.LB11:
	movq $1, %rax
	testq %rax, %rax
	jne .LB9
	jmp .LB10
.LB9:
	movq $1, %rax
	jmp .Lret1
.LB10:
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
