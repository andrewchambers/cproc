	.data
	.globl s
	.balign 1
s:
	.byte 1
	.quad 2
	.short 3
	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $112, %rsp
.LB1:
.LB2:
	leaq s(%rip), %rax
	leaq -8(%rbp), %r11
	movq %rax, (%r11)
	leaq -8(%rbp), %r11
	movq (%r11), %rax
	movq $0, %rcx
	addq %rcx, %rax
	leaq -16(%rbp), %r11
	movq %rax, (%r11)
	leaq -16(%rbp), %r11
	movq (%r11), %r11
	movsbq (%r11), %rax
	leaq -17(%rbp), %r11
	movb %al, (%r11)
	leaq -17(%rbp), %r11
	movsbq (%r11), %rax
	leaq -32(%rbp), %r11
	movq %rax, (%r11)
	leaq s(%rip), %rax
	leaq -40(%rbp), %r11
	movq %rax, (%r11)
	leaq -40(%rbp), %r11
	movq (%r11), %rax
	movq $1, %rcx
	addq %rcx, %rax
	leaq -48(%rbp), %r11
	movq %rax, (%r11)
	leaq -48(%rbp), %r11
	movq (%r11), %r11
	movq (%r11), %rax
	leaq -56(%rbp), %r11
	movq %rax, (%r11)
	leaq -32(%rbp), %r11
	movq (%r11), %rax
	leaq -56(%rbp), %r11
	movq (%r11), %rcx
	addq %rcx, %rax
	leaq -64(%rbp), %r11
	movq %rax, (%r11)
	leaq s(%rip), %rax
	leaq -72(%rbp), %r11
	movq %rax, (%r11)
	leaq -72(%rbp), %r11
	movq (%r11), %rax
	movq $9, %rcx
	addq %rcx, %rax
	leaq -80(%rbp), %r11
	movq %rax, (%r11)
	leaq -80(%rbp), %r11
	movq (%r11), %r11
	movswq (%r11), %rax
	leaq -82(%rbp), %r11
	movw %ax, (%r11)
	leaq -82(%rbp), %r11
	movswq (%r11), %rax
	leaq -96(%rbp), %r11
	movq %rax, (%r11)
	leaq -64(%rbp), %r11
	movq (%r11), %rax
	leaq -96(%rbp), %r11
	movq (%r11), %rcx
	subq %rcx, %rax
	leaq -104(%rbp), %r11
	movq %rax, (%r11)
	leaq -104(%rbp), %r11
	movq (%r11), %rax
	leaq -108(%rbp), %r11
	movl %eax, (%r11)
	leaq -108(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
