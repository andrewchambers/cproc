	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
.LB2:
	jmp .LB3
.LB5:
	movq $1, %rax
	jmp .Lret1
.LB6:
	movq $0, %rax
	jmp .Lret1
.LB3:
	movq $1249835483136, %rax
	movq $0, %rcx
	cmpq %rcx, %rax
	sete %al
	movzbq %al, %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB5
	jmp .LB7
.LB7:
	movq $1249835483136, %rax
	movq $0, %rcx
	cmpq %rcx, %rax
	setb %al
	movzbq %al, %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
	leaq -8(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB8
	jmp .LB9
.LB8:
	jmp .LB4
.LB9:
	movq $1249835483136, %rax
	movq $1249835483136, %rcx
	cmpq %rcx, %rax
	sete %al
	movzbq %al, %rax
	leaq -12(%rbp), %r11
	movl %eax, (%r11)
	leaq -12(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB6
	jmp .LB10
.LB10:
	movq $1249835483136, %rax
	movq $1249835483136, %rcx
	cmpq %rcx, %rax
	setb %al
	movzbq %al, %rax
	leaq -16(%rbp), %r11
	movl %eax, (%r11)
	leaq -16(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB11
	jmp .LB12
.LB11:
	jmp .LB4
.LB12:
	jmp .LB4
.LB4:
	movq $2, %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
