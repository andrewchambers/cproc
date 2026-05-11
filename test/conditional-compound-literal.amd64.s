	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $64, %rsp
.LB1:
.LB2:
	movq $0, %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	leaq -20(%rbp), %r11
	movl %eax, (%r11)
	leaq -20(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB3
	jmp .LB4
.LB3:
	movq $0, %rax
	leaq -40(%rbp), %r11
	movq %rax, (%r11)
	leaq -40(%rbp), %r11
	movq (%r11), %rax
	leaq -32(%rbp), %r11
	movq %rax, (%r11)
	jmp .LB5
.LB4:
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	leaq -48(%rbp), %r11
	movl %eax, (%r11)
	leaq -48(%rbp), %r11
	movslq (%r11), %rax
	leaq -44(%rbp), %r11
	movl %eax, (%r11)
	leaq -44(%rbp), %rax
	leaq -32(%rbp), %r11
	movq %rax, (%r11)
	jmp .LB5
.LB5:
	leaq -32(%rbp), %r11
	movq (%r11), %rax
	leaq -16(%rbp), %r11
	movq %rax, (%r11)
	leaq -16(%rbp), %r11
	movq (%r11), %rax
	leaq -56(%rbp), %r11
	movq %rax, (%r11)
	leaq -56(%rbp), %r11
	movq (%r11), %r11
	movslq (%r11), %rax
	leaq -60(%rbp), %r11
	movl %eax, (%r11)
	leaq -60(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
