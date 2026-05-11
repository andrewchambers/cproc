	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $32, %rsp
.LB1:
.LB2:
	movq $1, %rax
	leaq -8(%rbp), %r11
	movq %rax, (%r11)
	leaq -8(%rbp), %r11
	movq (%r11), %rax
	movq $4, %rcx
	imulq %rcx, %rax
	leaq -16(%rbp), %r11
	movq %rax, (%r11)
	leaq x(%rip), %rax
	leaq -16(%rbp), %r11
	movq (%r11), %rcx
	addq %rcx, %rax
	leaq -24(%rbp), %r11
	movq %rax, (%r11)
	jmp .Lret1
.Lret1:
	leave
	ret
	.data
	.globl x
	.balign 4
x:
	.zero 8
	.section .note.GNU-stack,"",@progbits
