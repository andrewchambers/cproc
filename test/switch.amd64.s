	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $48, %rsp
.LB1:
.LB2:
	leaq x(%rip), %r11
	movslq (%r11), %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	jmp .LB3
.LB5:
	movq $1, %rax
	jmp .Lret1
.LB6:
	movq $2, %rax
	jmp .Lret1
.LB7:
	movq $3, %rax
	jmp .Lret1
.LB8:
	movq $4, %rax
	jmp .Lret1
.LB9:
	movq $0, %rax
	jmp .Lret1
.LB10:
	movq $5, %rax
	jmp .Lret1
.LB3:
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	movq $52, %rcx
	cmpq %rcx, %rax
	sete %al
	movzbq %al, %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
	leaq -8(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB6
	jmp .LB11
.LB11:
	leaq -4(%rbp), %r11
	movl (%r11), %eax
	movq $52, %rcx
	cmpq %rcx, %rax
	setb %al
	movzbq %al, %rax
	leaq -12(%rbp), %r11
	movl %eax, (%r11)
	leaq -12(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB12
	jmp .LB13
.LB12:
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	movq $3, %rcx
	cmpq %rcx, %rax
	sete %al
	movzbq %al, %rax
	leaq -16(%rbp), %r11
	movl %eax, (%r11)
	leaq -16(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB5
	jmp .LB14
.LB14:
	leaq -4(%rbp), %r11
	movl (%r11), %eax
	movq $3, %rcx
	cmpq %rcx, %rax
	setb %al
	movzbq %al, %rax
	leaq -20(%rbp), %r11
	movl %eax, (%r11)
	leaq -20(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB15
	jmp .LB16
.LB15:
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	movq $0, %rcx
	cmpq %rcx, %rax
	sete %al
	movzbq %al, %rax
	leaq -24(%rbp), %r11
	movl %eax, (%r11)
	leaq -24(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB9
	jmp .LB17
.LB17:
	leaq -4(%rbp), %r11
	movl (%r11), %eax
	movq $0, %rcx
	cmpq %rcx, %rax
	setb %al
	movzbq %al, %rax
	leaq -28(%rbp), %r11
	movl %eax, (%r11)
	leaq -28(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB18
	jmp .LB19
.LB18:
	jmp .LB8
.LB19:
	jmp .LB8
.LB16:
	jmp .LB8
.LB13:
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	movq $-3, %rcx
	cmpq %rcx, %rax
	sete %al
	movzbq %al, %rax
	leaq -32(%rbp), %r11
	movl %eax, (%r11)
	leaq -32(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB7
	jmp .LB20
.LB20:
	leaq -4(%rbp), %r11
	movl (%r11), %eax
	movq $-3, %rcx
	cmpq %rcx, %rax
	setb %al
	movzbq %al, %rax
	leaq -36(%rbp), %r11
	movl %eax, (%r11)
	leaq -36(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB21
	jmp .LB22
.LB21:
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	movq $101, %rcx
	cmpq %rcx, %rax
	sete %al
	movzbq %al, %rax
	leaq -40(%rbp), %r11
	movl %eax, (%r11)
	leaq -40(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB10
	jmp .LB23
.LB23:
	leaq -4(%rbp), %r11
	movl (%r11), %eax
	movq $101, %rcx
	cmpq %rcx, %rax
	setb %al
	movzbq %al, %rax
	leaq -44(%rbp), %r11
	movl %eax, (%r11)
	leaq -44(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB24
	jmp .LB25
.LB24:
	jmp .LB8
.LB25:
	jmp .LB8
.LB22:
	jmp .LB8
.LB4:
	movq $0, %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.data
	.globl x
	.balign 4
x:
	.zero 4
	.section .note.GNU-stack,"",@progbits
