	.data
	.balign 1
.Lc.2:
	.byte 0
	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $32, %rsp
.LB1:
.LB2:
	leaq .Lc.2(%rip), %r11
	movzbq (%r11), %rax
	leaq -1(%rbp), %r11
	movb %al, (%r11)
	leaq -1(%rbp), %r11
	movzbq (%r11), %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
	leaq -8(%rbp), %r11
	movslq (%r11), %rax
	movq %rax, %rdi
	movb $0, %al
	call g
	leaq .Lc.2(%rip), %r11
	movzbq (%r11), %rax
	leaq -9(%rbp), %r11
	movb %al, (%r11)
	leaq -9(%rbp), %r11
	movzbq (%r11), %rax
	leaq -16(%rbp), %r11
	movl %eax, (%r11)
	leaq -16(%rbp), %r11
	movslq (%r11), %rax
	movq $-1, %rcx
	xorq %rcx, %rax
	leaq -20(%rbp), %r11
	movl %eax, (%r11)
	leaq -20(%rbp), %r11
	movslq (%r11), %rax
	movq %rax, %rdi
	movb $0, %al
	call g
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
