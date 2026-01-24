typedef _Complex double cdouble;
typedef _Complex float cfloat;

int main(void) {
	union { cdouble z; double p[2]; } u;
	u.z = (cdouble){1.25, -2.5};
	if (u.p[0] != 1.25 || u.p[1] != -2.5)
		return 1;

	cdouble zr = 3.0;
	u.z = zr;
	if (u.p[0] != 3.0 || u.p[1] != 0.0)
		return 1;

	union { cfloat z; float p[2]; } uf;
	uf.z = 1.0fi;
	if (uf.p[0] != 0.0f || uf.p[1] != 1.0f)
		return 1;

	if ((double)(cdouble){4.0, 5.0} != 4.0)
		return 1;

	if (!(_Complex double){0.0, 1.0})
		return 1;
	if ((_Complex double){0.0, 0.0})
		return 1;

	return 0;
}
