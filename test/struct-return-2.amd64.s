	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $32, %rsp
.LB1:
.LB2:
	leaq -8(%rbp), %rdi
	movb $0, %al
	call g
	leaq -8(%rbp), %r11
	movq (%r11), %rax
	leaq -16(%rbp), %r11
	movq %rax, (%r11)
	leaq -16(%rbp), %r11
	movq (%r11), %rax
	movq $4, %rcx
	addq %rcx, %rax
	leaq -24(%rbp), %r11
	movq %rax, (%r11)
	leaq -24(%rbp), %r11
	movq (%r11), %r11
	movslq (%r11), %rax
	leaq -28(%rbp), %r11
	movl %eax, (%r11)
	leaq -28(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
