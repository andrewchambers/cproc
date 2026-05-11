	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $32, %rsp
.LB1:
	movq %rdi, %rax
	movq %rax, %r10
	leaq -24(%rbp), %rdi
	movq 0(%r10), %rax
	movq %rax, 0(%rdi)
	movq 8(%r10), %rax
	movq %rax, 8(%rdi)
	movq 16(%r10), %rax
	movq %rax, 16(%rdi)
.LB2:
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
