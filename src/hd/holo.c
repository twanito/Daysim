/* Copyright (c) 1997 Silicon Graphics, Inc. */

#ifndef lint
static char SCCSid[] = "$SunId$ SGI";
#endif

/*
 * Routines for converting holodeck coordinates, etc.
 *
 *	10/22/97	GWLarson
 */

#include "holo.h"

float	hd_depthmap[DCINF-DCLIN];

static double	logstep;

static int	wg0[6] = {1,1,2,2,0,0};
static int	wg1[6] = {2,2,0,0,1,1};


hdcompgrid(hp)			/* compute derived grid vector and index */
register HOLO	*hp;
{
	FVECT	AxB;
	double	d;
	register FLOAT	*v;
	register int	i, j;
				/* initialize depth map */
	if (hd_depthmap[0] < 1.) {
		d = 1. + .5/DCLIN;
		for (i = 0; i < DCINF-DCLIN; i++) {
			hd_depthmap[i] = d;
			d *= 1. + 1./DCLIN;
		}
		logstep = log(1. + 1./DCLIN);
	}
				/* compute grid coordinate vectors */
	for (i = 0; i < 3; i++) {
		fcross(AxB, hp->xv[(i+1)%3], v=hp->xv[(i+2)%3]);
		VCOPY(hp->wn[i], AxB);
		if (normalize(hp->wn[i]) == 0.)
			error(USER, "degenerate holodeck section");
		hp->wo[i<<1] = DOT(hp->wn[i],hp->orig);
		hp->wo[i<<1|1] = hp->wo[i<<1] + DOT(hp->wn[i],hp->xv[i]);
		fcross(hp->gv[i][0], v, AxB);
		d = DOT(v,v) / DOT(hp->gv[i][0],hp->gv[i][0]) *
				hp->grid[(i+1)%3];
		for (j = 0; j < 3; j++)
			hp->gv[i][0][j] *= d;
		fcross(hp->gv[i][1], AxB, v=hp->xv[(i+1)%3]);
		d = DOT(v,v) / DOT(hp->gv[i][1],hp->gv[i][1]) *
				hp->grid[(i+2)%3];
		for (j = 0; j < 3; j++)
			hp->gv[i][1][j] *= d;
	}
				/* compute linear depth range */
	hp->tlin = VLEN(hp->xv[0]) + VLEN(hp->xv[1]) + VLEN(hp->xv[2]);
				/* compute wall super-indices from grid */
	hp->wi[0] = 1;		/**** index values begin at 1 ****/
	for (i = 1; i < 6; i++) {
		hp->wi[i] = 0;
		for (j = i; j < 6; j++)
			hp->wi[i] += hp->grid[wg0[j]] * hp->grid[wg1[j]];
		hp->wi[i] *= hp->grid[wg0[i-1]] * hp->grid[wg1[i-1]];
		hp->wi[i] += hp->wi[i-1];
	}
}


HOLO *
hdalloc(hproto)		/* allocate and set holodeck section based on grid */
HDGRID	*hproto;
{
	HOLO	hdhead;
	register HOLO	*hp;
	int	n;
				/* copy grid to temporary header */
	bcopy((char *)hproto, (char *)&hdhead, sizeof(HDGRID));
				/* compute grid vectors and sizes */
	hdcompgrid(&hdhead);
				/* allocate header with directory */
	n = sizeof(HOLO)+nbeams(&hdhead)*sizeof(BEAMI);
	if ((hp = (HOLO *)malloc(n)) == NULL)
		return(NULL);
				/* copy header information */
	copystruct(hp, &hdhead);
				/* allocate and clear beam list */
	hp->bl = (BEAM **)malloc((nbeams(hp)+1)*sizeof(BEAM *)+sizeof(BEAM));
	if (hp->bl == NULL) {
		free((char *)hp);
		return(NULL);
	}
	bzero((char *)hp->bl, (nbeams(hp)+1)*sizeof(BEAM *)+sizeof(BEAM));
	hp->bl[0] = (BEAM *)(hp->bl+nbeams(hp)+1);	/* set blglob(hp) */
	hp->fd = -1;
	hp->dirty = 0;
	hp->priv = NULL;
				/* clear beam directory */
	bzero((char *)hp->bi, (nbeams(hp)+1)*sizeof(BEAMI));
	return(hp);		/* all is well */
}


hdbcoord(gc, hp, i)		/* compute beam coordinates from index */
BCOORD	gc;		/* returned */
register HOLO	*hp;
register int	i;
{
	register int	j, n;
	int	n2, reverse;
	BCOORD	g2;
					/* check range */
	if (i < 1 | i > nbeams(hp))
		return(0);
	if (reverse = i >= hp->wi[5])
		i -= hp->wi[5] - 1;
	for (j = 0; j < 5; j++)		/* find w0 */
		if (hp->wi[j+1] > i)
			break;
	i -= hp->wi[gc[0].w=j];
					/* find w1 */
	n2 = hp->grid[wg0[j]] * hp->grid[wg1[j]];
	while (++j < 5)	{
		n = n2 * hp->grid[wg0[j]] * hp->grid[wg1[j]];
		if (n > i)
			break;
		i -= n;
	}
	gc[1].w = j;
					/* find position on w0 */
	n2 = hp->grid[wg0[j]] * hp->grid[wg1[j]];
	n = i / n2;
	gc[0].i[1] = n / hp->grid[wg0[gc[0].w]];
	gc[0].i[0] = n - gc[0].i[1]*hp->grid[wg0[gc[0].w]];
	i -= n*n2;
					/* find position on w1 */
	gc[1].i[1] = i / hp->grid[wg0[gc[1].w]];
	gc[1].i[0] = i - gc[1].i[1]*hp->grid[wg0[gc[1].w]];
	if (reverse) {
		copystruct(g2, gc+1);
		copystruct(gc+1, gc);
		copystruct(gc, g2);
	}
	return(1);			/* we're done */
}


int
hdbindex(hp, gc)		/* compute index from beam coordinates */
register HOLO	*hp;
register BCOORD	gc;
{
	BCOORD	g2;
	int	reverse;
	register int	i, j;
					/* check ordering and limits */
	if (reverse = gc[0].w > gc[1].w) {
		copystruct(g2, gc+1);
		copystruct(g2+1, gc);
		gc = g2;
	} else if (gc[0].w == gc[1].w)
		return(0);
	if (gc[0].w < 0 | gc[1].w > 5)
		return(0);
	i = 0;				/* compute index */
	for (j = gc[0].w+1; j < gc[1].w; j++)
		i += hp->grid[wg0[j]] * hp->grid[wg1[j]];
	i *= hp->grid[wg0[gc[0].w]] * hp->grid[wg1[gc[0].w]];
	i += hp->wi[gc[0].w];
	i += (hp->grid[wg0[gc[0].w]]*gc[0].i[1] + gc[0].i[0]) *
			hp->grid[wg0[gc[1].w]] * hp->grid[wg1[gc[1].w]] ;
	i += hp->grid[wg0[gc[1].w]]*gc[1].i[1] + gc[1].i[0];
	if (reverse)
		i += hp->wi[5] - 1;
	return(i);
}


hdlseg(lseg, hp, i)			/* compute line segment for beam */
register int	lseg[2][3];
register HOLO	*hp;
int	i;
{
	BCOORD	gc;
	register int	k;

	if (!hdbcoord(gc, hp, i))		/* compute grid coordinates */
		return(0);
	for (k = 0; k < 2; k++) {		/* compute end points */
		lseg[k][gc[k].w>>1] = gc[k].w&1	? hp->grid[gc[k].w>>1]-1 : 0 ;
		lseg[k][wg0[gc[k].w]] = gc[k].i[0];
		lseg[k][wg1[gc[k].w]] = gc[k].i[1];
	}
	return(1);
}


unsigned
hdcode(hp, d)			/* compute depth code for d */
HOLO	*hp;
double	d;
{
	double	tl = hp->tlin;
	register unsigned	c;

	if (d <= 0.)
		return(0);
	if (d >= .99*FHUGE)
		return(DCINF);
	if (d < tl)
		return((unsigned)(d*DCLIN/tl));
	c = (unsigned)(log(d/tl)/logstep) + DCLIN;
	return(c > DCINF ? DCINF : c);
}


double
hdray(ro, rd, hp, gc, r)	/* compute ray within a beam */
FVECT	ro, rd;		/* returned */
register HOLO	*hp;
register BCOORD	gc;
BYTE	r[2][2];
{
	FVECT	p[2];
	register int	i;
	register FLOAT	*v;
	double	d;
					/* compute entry and exit points */
	for (i = 0; i < 2; i++) {
		VCOPY(p[i], hp->orig);
		if (gc[i].w & 1) {
			v = hp->xv[gc[i].w>>1];
			p[i][0] += *v++; p[i][1] += *v++; p[i][2] += *v;
		}
		d = ( gc[i].i[0] + (1./256.)*(r[i][0]+.5) ) /
				hp->grid[wg0[gc[i].w]];
		v = hp->xv[wg0[gc[i].w]];
		p[i][0] += d * *v++; p[i][1] += d * *v++; p[i][2] += d * *v;
		d = (gc[i].i[1] + (1./256.)*(r[i][1]+.5)) /
				hp->grid[wg1[gc[i].w]];
		v = hp->xv[wg1[gc[i].w]];
		p[i][0] += d * *v++; p[i][1] += d * *v++; p[i][2] += d * *v;
	}
	VCOPY(ro, p[0]);		/* assign ray origin and direction */
	rd[0] = p[1][0] - p[0][0];
	rd[1] = p[1][1] - p[0][1];
	rd[2] = p[1][2] - p[0][2];
	return(normalize(rd));		/* return maximum inside distance */
}


double
hdinter(gc, r, hp, ro, rd)	/* compute ray intersection with section */
register BCOORD	gc;	/* returned */
BYTE	r[2][2];	/* returned */
register HOLO	*hp;
FVECT	ro, rd;		/* rd should be normalized */
{
	FVECT	p[2], vt;
	double	d, t0, t1, d0, d1;
	register FLOAT	*v;
	register int	i;
					/* first, intersect walls */
	gc[0].w = gc[1].w = -1;
	t0 = -FHUGE; t1 = FHUGE;
	for (i = 0; i < 3; i++) {		/* for each wall pair */
		d = -DOT(rd, hp->wn[i]);	/* plane distance */
		if (d <= FTINY && d >= -FTINY)	/* check for parallel */
			continue;
		d1 = DOT(ro, hp->wn[i]);	/* ray distances */
		d0 = (d1 - hp->wo[i<<1]) / d;
		d1 = (d1 - hp->wo[i<<1|1]) / d;
		if (d0 < d1) {		/* check against best */
			if (d0 > t0) {
				t0 = d0;
				gc[0].w = i<<1;
			}
			if (d1 < t1) {
				t1 = d1;
				gc[1].w = i<<1 | 1;
			}
		} else {
			if (d1 > t0) {
				t0 = d1;
				gc[0].w = i<<1 | 1;
			}
			if (d0 < t1) {
				t1 = d0;
				gc[1].w = i<<1;
			}
		}
	}
	if (gc[0].w < 0 | gc[1].w < 0)		/* paranoid check */
		return(FHUGE);
						/* compute intersections */
	for (i = 0; i < 3; i++) {
		p[0][i] = ro[i] + rd[i]*t0;
		p[1][i] = ro[i] + rd[i]*t1;
	}
					/* now, compute grid coordinates */
	for (i = 0; i < 2; i++) {
		vt[0] = p[i][0] - hp->orig[0];
		vt[1] = p[i][1] - hp->orig[1];
		vt[2] = p[i][2] - hp->orig[2];
		if (gc[i].w & 1) {
			v = hp->xv[gc[i].w>>1];
			vt[0] -= *v++; vt[1] -= *v++; vt[2] -= *v;
		}
		v = hp->gv[gc[i].w>>1][0];
		d = DOT(vt, v);
		if (d < 0. || (gc[i].i[0] = d) >= hp->grid[wg0[gc[i].w]])
			return(FHUGE);		/* outside wall */
		r[i][0] = 256. * (d - gc[i].i[0]);
		v = hp->gv[gc[i].w>>1][1];
		d = DOT(vt, v);
		if (d < 0. || (gc[i].i[1] = d) >= hp->grid[wg1[gc[i].w]])
			return(FHUGE);		/* outside wall */
		r[i][1] = 256. * (d - gc[i].i[1]);
	}
					/* return distance from entry point */
	vt[0] = ro[0] - p[0][0];
	vt[1] = ro[1] - p[0][1];
	vt[2] = ro[2] - p[0][2];
	return(DOT(vt,rd));
}