	.data
	.globl s
	.balign 1
s:
	.byte 97
	.byte 206
	.byte 177
	.byte 226
	.byte 130
	.byte 172
	.byte 240
	.byte 159
	.byte 152
	.byte 128
	.byte 170
	.byte 187
	.byte 204
	.byte 0
	.data
	.globl u8
	.balign 1
u8:
	.byte 97
	.byte 206
	.byte 177
	.byte 226
	.byte 130
	.byte 172
	.byte 240
	.byte 159
	.byte 152
	.byte 128
	.byte 170
	.byte 187
	.byte 204
	.byte 0
	.data
	.globl u
	.balign 2
u:
	.short 97
	.short 945
	.short 8364
	.short 55357
	.short 56832
	.short 170
	.short 48059
	.short 52428
	.short 0
	.data
	.globl U
	.balign 4
U:
	.long 97
	.long 945
	.long 8364
	.long 128512
	.long 170
	.long 48059
	.long 3435973836
	.long 0
	.data
	.globl L
	.balign 4
L:
	.long 97
	.long 945
	.long 8364
	.long 128512
	.long 170
	.long 48059
	.long 3435973836
	.long 0
	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $112, %rsp
.LB1:
.LB2:
	movq $97, %rax
	leaq -14(%rbp), %r11
	movb %al, (%r11)
	movq $206, %rax
	leaq -14(%rbp), %r11
	leaq 1(%r11), %r11
	movb %al, (%r11)
	movq $177, %rax
	leaq -14(%rbp), %r11
	leaq 2(%r11), %r11
	movb %al, (%r11)
	movq $226, %rax
	leaq -14(%rbp), %r11
	leaq 3(%r11), %r11
	movb %al, (%r11)
	movq $130, %rax
	leaq -14(%rbp), %r11
	leaq 4(%r11), %r11
	movb %al, (%r11)
	movq $172, %rax
	leaq -14(%rbp), %r11
	leaq 5(%r11), %r11
	movb %al, (%r11)
	movq $240, %rax
	leaq -14(%rbp), %r11
	leaq 6(%r11), %r11
	movb %al, (%r11)
	movq $159, %rax
	leaq -14(%rbp), %r11
	leaq 7(%r11), %r11
	movb %al, (%r11)
	movq $152, %rax
	leaq -14(%rbp), %r11
	leaq 8(%r11), %r11
	movb %al, (%r11)
	movq $128, %rax
	leaq -14(%rbp), %r11
	leaq 9(%r11), %r11
	movb %al, (%r11)
	movq $170, %rax
	leaq -14(%rbp), %r11
	leaq 10(%r11), %r11
	movb %al, (%r11)
	movq $187, %rax
	leaq -14(%rbp), %r11
	leaq 11(%r11), %r11
	movb %al, (%r11)
	movq $204, %rax
	leaq -14(%rbp), %r11
	leaq 12(%r11), %r11
	movb %al, (%r11)
	movq $0, %rax
	leaq -14(%rbp), %r11
	leaq 13(%r11), %r11
	movb %al, (%r11)
	movq $97, %rax
	leaq -28(%rbp), %r11
	movb %al, (%r11)
	movq $206, %rax
	leaq -28(%rbp), %r11
	leaq 1(%r11), %r11
	movb %al, (%r11)
	movq $177, %rax
	leaq -28(%rbp), %r11
	leaq 2(%r11), %r11
	movb %al, (%r11)
	movq $226, %rax
	leaq -28(%rbp), %r11
	leaq 3(%r11), %r11
	movb %al, (%r11)
	movq $130, %rax
	leaq -28(%rbp), %r11
	leaq 4(%r11), %r11
	movb %al, (%r11)
	movq $172, %rax
	leaq -28(%rbp), %r11
	leaq 5(%r11), %r11
	movb %al, (%r11)
	movq $240, %rax
	leaq -28(%rbp), %r11
	leaq 6(%r11), %r11
	movb %al, (%r11)
	movq $159, %rax
	leaq -28(%rbp), %r11
	leaq 7(%r11), %r11
	movb %al, (%r11)
	movq $152, %rax
	leaq -28(%rbp), %r11
	leaq 8(%r11), %r11
	movb %al, (%r11)
	movq $128, %rax
	leaq -28(%rbp), %r11
	leaq 9(%r11), %r11
	movb %al, (%r11)
	movq $170, %rax
	leaq -28(%rbp), %r11
	leaq 10(%r11), %r11
	movb %al, (%r11)
	movq $187, %rax
	leaq -28(%rbp), %r11
	leaq 11(%r11), %r11
	movb %al, (%r11)
	movq $204, %rax
	leaq -28(%rbp), %r11
	leaq 12(%r11), %r11
	movb %al, (%r11)
	movq $0, %rax
	leaq -28(%rbp), %r11
	leaq 13(%r11), %r11
	movb %al, (%r11)
	movq $97, %rax
	leaq -46(%rbp), %r11
	movw %ax, (%r11)
	movq $945, %rax
	leaq -46(%rbp), %r11
	leaq 2(%r11), %r11
	movw %ax, (%r11)
	movq $8364, %rax
	leaq -46(%rbp), %r11
	leaq 4(%r11), %r11
	movw %ax, (%r11)
	movq $55357, %rax
	leaq -46(%rbp), %r11
	leaq 6(%r11), %r11
	movw %ax, (%r11)
	movq $56832, %rax
	leaq -46(%rbp), %r11
	leaq 8(%r11), %r11
	movw %ax, (%r11)
	movq $170, %rax
	leaq -46(%rbp), %r11
	leaq 10(%r11), %r11
	movw %ax, (%r11)
	movq $48059, %rax
	leaq -46(%rbp), %r11
	leaq 12(%r11), %r11
	movw %ax, (%r11)
	movq $52428, %rax
	leaq -46(%rbp), %r11
	leaq 14(%r11), %r11
	movw %ax, (%r11)
	movq $0, %rax
	leaq -46(%rbp), %r11
	leaq 16(%r11), %r11
	movw %ax, (%r11)
	movq $97, %rax
	leaq -80(%rbp), %r11
	movl %eax, (%r11)
	movq $945, %rax
	leaq -80(%rbp), %r11
	leaq 4(%r11), %r11
	movl %eax, (%r11)
	movq $8364, %rax
	leaq -80(%rbp), %r11
	leaq 8(%r11), %r11
	movl %eax, (%r11)
	movq $128512, %rax
	leaq -80(%rbp), %r11
	leaq 12(%r11), %r11
	movl %eax, (%r11)
	movq $170, %rax
	leaq -80(%rbp), %r11
	leaq 16(%r11), %r11
	movl %eax, (%r11)
	movq $48059, %rax
	leaq -80(%rbp), %r11
	leaq 20(%r11), %r11
	movl %eax, (%r11)
	movq $3435973836, %rax
	leaq -80(%rbp), %r11
	leaq 24(%r11), %r11
	movl %eax, (%r11)
	movq $0, %rax
	leaq -80(%rbp), %r11
	leaq 28(%r11), %r11
	movl %eax, (%r11)
	movq $97, %rax
	leaq -112(%rbp), %r11
	movl %eax, (%r11)
	movq $945, %rax
	leaq -112(%rbp), %r11
	leaq 4(%r11), %r11
	movl %eax, (%r11)
	movq $8364, %rax
	leaq -112(%rbp), %r11
	leaq 8(%r11), %r11
	movl %eax, (%r11)
	movq $128512, %rax
	leaq -112(%rbp), %r11
	leaq 12(%r11), %r11
	movl %eax, (%r11)
	movq $170, %rax
	leaq -112(%rbp), %r11
	leaq 16(%r11), %r11
	movl %eax, (%r11)
	movq $48059, %rax
	leaq -112(%rbp), %r11
	leaq 20(%r11), %r11
	movl %eax, (%r11)
	movq $3435973836, %rax
	leaq -112(%rbp), %r11
	leaq 24(%r11), %r11
	movl %eax, (%r11)
	movq $0, %rax
	leaq -112(%rbp), %r11
	leaq 28(%r11), %r11
	movl %eax, (%r11)
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
