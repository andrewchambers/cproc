__asm__ __volatile (
".data\n"
".globl asm_decl_value\n"
"asm_decl_value:\n"
"	.long 1234\n"
);

extern int asm_decl_value;

int main(void)
{
	return asm_decl_value == 1234 ? 0 : 1;
}
