#ifndef lint
static const char RCSid[] = "$Id$";
#endif
/*
 * Compute and print barycentric coordinates for triangle meshes
 */

#include <stdio.h>

#include "fvect.h"

#include "tmesh.h"

#define ABS(x)		((x) >= 0 ? (x) : -(x))


int
flat_tri(v1, v2, v3, n1, n2, n3)	/* determine if triangle is flat */
FVECT	v1, v2, v3, n1, n2, n3;
{
	double	d1, d2, d3;
	FVECT	vt1, vt2, vn;
					/* compute default normal */
	vt1[0] = v2[0] - v1[0]; vt1[1] = v2[1] - v1[1]; vt1[2] = v2[2] - v1[2];
	vt2[0] = v3[0] - v2[0]; vt2[1] = v3[1] - v2[1]; vt2[2] = v3[2] - v2[2];
	fcross(vn, vt1, vt2);
	if (normalize(vn) == 0.0)
		return(DEGEN);
					/* compare to supplied normals */
	d1 = DOT(vn, n1); d2 = DOT(vn, n2); d3 = DOT(vn, n3);
	if (d1 < 0 && d2 < 0 && d3 < 0) {
		if (d1 > -COSTOL || d2 > -COSTOL || d3 > -COSTOL)
			return(RVBENT);
		return(RVFLAT);
	}
	if (d1 < COSTOL || d2 < COSTOL || d3 < COSTOL)
		return(ISBENT);
	return(ISFLAT);
}


int
comp_baryc(bcm, v1, v2, v3)		/* compute barycentric vectors */
register BARYCCM	*bcm;
FLOAT	*v1, *v2, *v3;
{
	FLOAT	*vt;
	FVECT	va, vab, vcb;
	double	d;
	int	ax0, ax1;
	register int	i;
					/* compute major axis */
	for (i = 0; i < 3; i++) {
		vab[i] = v1[i] - v2[i];
		vcb[i] = v3[i] - v2[i];
	}
	fcross(va, vab, vcb);
	bcm->ax = ABS(va[0]) > ABS(va[1]) ? 0 : 1;
	bcm->ax = ABS(va[bcm->ax]) > ABS(va[2]) ? bcm->ax : 2;
	ax0 = (bcm->ax + 1) % 3;
	ax1 = (bcm->ax + 2) % 3;
	for (i = 0; i < 2; i++) {
		vab[0] = v1[ax0] - v2[ax0];
		vcb[0] = v3[ax0] - v2[ax0];
		vab[1] = v1[ax1] - v2[ax1];
		vcb[1] = v3[ax1] - v2[ax1];
		d = vcb[0]*vcb[0] + vcb[1]*vcb[1];
		if (d <= FTINY*FTINY)
			return(-1);
		d = (vcb[0]*vab[0]+vcb[1]*vab[1])/d;
		va[0] = vab[0] - vcb[0]*d;
		va[1] = vab[1] - vcb[1]*d;
		d = va[0]*va[0] + va[1]*va[1];
		if (d <= FTINY*FTINY)
			return(-1);
		d = 1.0/d;
		bcm->tm[i][0] = va[0] *= d;
		bcm->tm[i][1] = va[1] *= d;
		bcm->tm[i][2] = -(v2[ax0]*va[0]+v2[ax1]*va[1]);
					/* rotate vertices */
		vt = v1;
		v1 = v2;
		v2 = v3;
		v3 = vt;
	}
	return(0);
}


put_baryc(bcm, com, n)			/* put barycentric coord. vectors */
register BARYCCM	*bcm;
register FLOAT	com[][3];
int	n;
{
	double	a, b;
	register int	i, j;

	printf("%d\t%d\n", 1+3*n, bcm->ax);
	for (i = 0; i < n; i++) {
		a = com[i][0] - com[i][2];
		b = com[i][1] - com[i][2];
		printf("%14.8f %14.8f %14.8f\n",
			bcm->tm[0][0]*a + bcm->tm[1][0]*b,
			bcm->tm[0][1]*a + bcm->tm[1][1]*b,
			bcm->tm[0][2]*a + bcm->tm[1][2]*b + com[i][2]);
	}
}