	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
.LB2:
	movq $123, %rax
	movq $123, %rcx
	cmpq %rcx, %rax
	setne %al
	movzbq %al, %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB3
	jmp .LB4
.LB3:
	movq $1, %rax
	jmp .Lret1
.LB4:
	movq $456, %rax
	movq $456, %rcx
	cmpq %rcx, %rax
	setne %al
	movzbq %al, %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
	leaq -8(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB5
	jmp .LB6
.LB5:
	movq $1, %rax
	jmp .Lret1
.LB6:
	movq $0, %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
