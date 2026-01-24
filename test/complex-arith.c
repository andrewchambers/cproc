typedef _Complex double cdouble;

int main(void) {
	union { cdouble z; double p[2]; } u;
	cdouble a = (cdouble){2.0, 3.0};
	cdouble b = (cdouble){4.0, -5.0};

	u.z = a + b;
	if (u.p[0] != 6.0 || u.p[1] != -2.0)
		return 1;

	u.z = a * b;
	if (u.p[0] != 23.0 || u.p[1] != 2.0)
		return 1;

	u.z = (cdouble){4.0, 2.0} / (cdouble){2.0, 0.0};
	if (u.p[0] != 2.0 || u.p[1] != 1.0)
		return 1;

	if (!(a != b))
		return 1;
	if (a == b)
		return 1;

	return 0;
}
