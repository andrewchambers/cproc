	.data
	.globl x
	.balign 4
x:
	.long 32
	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $144, %rsp
.LB1:
	movq %rdi, %rax
	leaq -8(%rbp), %r11
	movq %rax, (%r11)
.LB2:
	leaq -8(%rbp), %r11
	movq (%r11), %rax
	leaq -16(%rbp), %r11
	movq %rax, (%r11)
	leaq -16(%rbp), %r11
	movq (%r11), %rax
	leaq -24(%rbp), %r11
	movq %rax, (%r11)
	leaq -24(%rbp), %r11
	movq (%r11), %rax
	movq $0, %rcx
	addq %rcx, %rax
	leaq -32(%rbp), %r11
	movq %rax, (%r11)
	leaq -32(%rbp), %r11
	movq (%r11), %rax
	leaq -40(%rbp), %r11
	movq %rax, (%r11)
	leaq -40(%rbp), %r11
	movq (%r11), %rax
	movq $4, %rcx
	addq %rcx, %rax
	leaq -48(%rbp), %r11
	movq %rax, (%r11)
	movq $2, %rax
	leaq -56(%rbp), %r11
	movq %rax, (%r11)
	leaq -56(%rbp), %r11
	movq (%r11), %rax
	movq $2, %rcx
	imulq %rcx, %rax
	leaq -64(%rbp), %r11
	movq %rax, (%r11)
	leaq -48(%rbp), %r11
	movq (%r11), %rax
	leaq -64(%rbp), %r11
	movq (%r11), %rcx
	addq %rcx, %rax
	leaq -72(%rbp), %r11
	movq %rax, (%r11)
	leaq -72(%rbp), %r11
	movq (%r11), %r11
	movswq (%r11), %rax
	leaq -74(%rbp), %r11
	movw %ax, (%r11)
	leaq -74(%rbp), %r11
	movswq (%r11), %rax
	leaq -80(%rbp), %r11
	movl %eax, (%r11)
	leaq -8(%rbp), %r11
	movq (%r11), %rax
	leaq -88(%rbp), %r11
	movq %rax, (%r11)
	leaq -88(%rbp), %r11
	movq (%r11), %rax
	leaq -96(%rbp), %r11
	movq %rax, (%r11)
	leaq -96(%rbp), %r11
	movq (%r11), %rax
	movq $0, %rcx
	addq %rcx, %rax
	leaq -104(%rbp), %r11
	movq %rax, (%r11)
	movq $0, %rax
	leaq -112(%rbp), %r11
	movq %rax, (%r11)
	leaq -112(%rbp), %r11
	movq (%r11), %rax
	movq $1, %rcx
	imulq %rcx, %rax
	leaq -120(%rbp), %r11
	movq %rax, (%r11)
	leaq -104(%rbp), %r11
	movq (%r11), %rax
	leaq -120(%rbp), %r11
	movq (%r11), %rcx
	addq %rcx, %rax
	leaq -128(%rbp), %r11
	movq %rax, (%r11)
	leaq -128(%rbp), %r11
	movq (%r11), %r11
	movzbq (%r11), %rax
	leaq -129(%rbp), %r11
	movb %al, (%r11)
	leaq -129(%rbp), %r11
	movzbq (%r11), %rax
	leaq -136(%rbp), %r11
	movl %eax, (%r11)
	leaq -80(%rbp), %r11
	movslq (%r11), %rax
	leaq -136(%rbp), %r11
	movslq (%r11), %rcx
	addq %rcx, %rax
	leaq -140(%rbp), %r11
	movl %eax, (%r11)
	leaq -140(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
