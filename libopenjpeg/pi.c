/*
 * Copyright (c) 2002-2007, Communications and Remote Sensing Laboratory, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2007, Professor Benoit Macq
 * Copyright (c) 2001-2003, David Janssens
 * Copyright (c) 2002-2003, Yannick Verschueren
 * Copyright (c) 2003-2007, Francois-Olivier Devaux and Antonin Descampe
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
 * Copyright (c) 2006-2007, Parvatha Elangovan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opj_includes.h"

/** @defgroup PI PI - Implementation of a packet iterator */
/*@{*/

/** @name Local static functions */
/*@{*/

/**
Get next packet in layer-resolution-component-precinct order.
@param pi packet iterator to modify
@return returns false if pi pointed to the last packet or else returns true 
*/
static opj_bool pi_next_lrcp(opj_pi_iterator_t * pi);
/**
Get next packet in resolution-layer-component-precinct order.
@param pi packet iterator to modify
@return returns false if pi pointed to the last packet or else returns true 
*/
static opj_bool pi_next_rlcp(opj_pi_iterator_t * pi);
/**
Get next packet in resolution-precinct-component-layer order.
@param pi packet iterator to modify
@return returns false if pi pointed to the last packet or else returns true 
*/
static opj_bool pi_next_rpcl(opj_pi_iterator_t * pi);
/**
Get next packet in precinct-component-resolution-layer order.
@param pi packet iterator to modify
@return returns false if pi pointed to the last packet or else returns true 
*/
static opj_bool pi_next_pcrl(opj_pi_iterator_t * pi);
/**
Get next packet in component-precinct-resolution-layer order.
@param pi packet iterator to modify
@return returns false if pi pointed to the last packet or else returns true 
*/
static opj_bool pi_next_cprl(opj_pi_iterator_t * pi);



/**
 * Gets the encoding parameters needed to update the coding parameters and all the pocs.
 * The precinct widths, heights, dx and dy for each component at each resolution will be stored as well.
 * the last parameter of the function should be an array of pointers of size nb components, each pointer leading
 * to an area of size 4 * max_res. The data is stored inside this area with the following pattern :
 * dx_compi_res0 , dy_compi_res0 , w_compi_res0, h_compi_res0 , dx_compi_res1 , dy_compi_res1 , w_compi_res1, h_compi_res1 , ...
 *
 * @param	p_image			the image being encoded.
 * @param	p_cp			the coding parameters.
 * @param	tileno			the tile index of the tile being encoded.
 * @param	p_tx0			pointer that will hold the X0 parameter for the tile
 * @param	p_tx1			pointer that will hold the X1 parameter for the tile
 * @param	p_ty0			pointer that will hold the Y0 parameter for the tile
 * @param	p_ty1			pointer that will hold the Y1 parameter for the tile
 * @param	p_max_prec		pointer that will hold the the maximum precision for all the bands of the tile
 * @param	p_max_res		pointer that will hold the the maximum number of resolutions for all the poc inside the tile.
 * @param	p_dx_min		pointer that will hold the the minimum dx of all the components of all the resolutions for the tile.
 * @param	p_dy_min		pointer that will hold the the minimum dy of all the components of all the resolutions for the tile.
 * @param	p_resolutions	pointer to an area corresponding to the one described above.
 */
void get_all_encoding_parameters(
								const opj_image_t *p_image,
								const opj_cp_v2_t *p_cp,
								OPJ_UINT32 tileno,
								OPJ_INT32 * p_tx0,
								OPJ_INT32 * p_tx1,
								OPJ_INT32 * p_ty0,
								OPJ_INT32 * p_ty1,
								OPJ_UINT32 * p_dx_min,
								OPJ_UINT32 * p_dy_min,
								OPJ_UINT32 * p_max_prec,
								OPJ_UINT32 * p_max_res,
								OPJ_UINT32 ** p_resolutions
							);


/**
 * Allocates memory for a packet iterator. Data and data sizes are set by this operation.
 * No other data is set. The include section of the packet  iterator is not allocated.
 *
 * @param	image		the image used to initialize the packet iterator (only the number of components is relevant).
 * @param	cp		the coding parameters.
 * @param	tileno		the index of the tile from which creating the packet iterator.
 */
opj_pi_iterator_t * pi_create(	const opj_image_t *image,
								const opj_cp_v2_t *cp,
								OPJ_UINT32 tileno );

void pi_update_decode_not_poc (opj_pi_iterator_t * p_pi,opj_tcp_v2_t * p_tcp,OPJ_UINT32 p_max_precision,OPJ_UINT32 p_max_res);
void pi_update_decode_poc (opj_pi_iterator_t * p_pi,opj_tcp_v2_t * p_tcp,OPJ_UINT32 p_max_precision,OPJ_UINT32 p_max_res);


/*@}*/

/*@}*/

/* 
==========================================================
   local functions
==========================================================
*/

static opj_bool pi_next_lrcp(opj_pi_iterator_t * pi) {
	opj_pi_comp_t *comp = NULL;
	opj_pi_resolution_t *res = NULL;
	long index = 0;
	
	if (!pi->first) {
		comp = &pi->comps[pi->compno];
		res = &comp->resolutions[pi->resno];
		goto LABEL_SKIP;
	} else {
		pi->first = 0;
	}

	for (pi->layno = pi->poc.layno0; pi->layno < pi->poc.layno1; pi->layno++) {
		for (pi->resno = pi->poc.resno0; pi->resno < pi->poc.resno1;
		pi->resno++) {
			for (pi->compno = pi->poc.compno0; pi->compno < pi->poc.compno1; pi->compno++) {
				comp = &pi->comps[pi->compno];
				if (pi->resno >= comp->numresolutions) {
					continue;
				}
				res = &comp->resolutions[pi->resno];
				if (!pi->tp_on){
					pi->poc.precno1 = res->pw * res->ph;
				}
				for (pi->precno = pi->poc.precno0; pi->precno < pi->poc.precno1; pi->precno++) {
					index = pi->layno * pi->step_l + pi->resno * pi->step_r + pi->compno * pi->step_c + pi->precno * pi->step_p;
					if (!pi->include[index]) {
						pi->include[index] = 1;
						return OPJ_TRUE;
					}
LABEL_SKIP:;
				}
			}
		}
	}
	
	return OPJ_FALSE;
}

static opj_bool pi_next_rlcp(opj_pi_iterator_t * pi) {
	opj_pi_comp_t *comp = NULL;
	opj_pi_resolution_t *res = NULL;
	long index = 0;

	if (!pi->first) {
		comp = &pi->comps[pi->compno];
		res = &comp->resolutions[pi->resno];
		goto LABEL_SKIP;
	} else {
		pi->first = 0;
	}

	for (pi->resno = pi->poc.resno0; pi->resno < pi->poc.resno1; pi->resno++) {
		for (pi->layno = pi->poc.layno0; pi->layno < pi->poc.layno1; pi->layno++) {
			for (pi->compno = pi->poc.compno0; pi->compno < pi->poc.compno1; pi->compno++) {
				comp = &pi->comps[pi->compno];
				if (pi->resno >= comp->numresolutions) {
					continue;
				}
				res = &comp->resolutions[pi->resno];
				if(!pi->tp_on){
					pi->poc.precno1 = res->pw * res->ph;
				}
				for (pi->precno = pi->poc.precno0; pi->precno < pi->poc.precno1; pi->precno++) {
					index = pi->layno * pi->step_l + pi->resno * pi->step_r + pi->compno * pi->step_c + pi->precno * pi->step_p;
					if (!pi->include[index]) {
						pi->include[index] = 1;
						return OPJ_TRUE;
					}
LABEL_SKIP:;
				}
			}
		}
	}
	
	return OPJ_FALSE;
}

static opj_bool pi_next_rpcl(opj_pi_iterator_t * pi) {
	opj_pi_comp_t *comp = NULL;
	opj_pi_resolution_t *res = NULL;
	long index = 0;

	if (!pi->first) {
		goto LABEL_SKIP;
	} else {
		int compno, resno;
		pi->first = 0;
		pi->dx = 0;
		pi->dy = 0;
		for (compno = 0; compno < pi->numcomps; compno++) {
			comp = &pi->comps[compno];
			for (resno = 0; resno < comp->numresolutions; resno++) {
				int dx, dy;
				res = &comp->resolutions[resno];
				dx = comp->dx * (1 << (res->pdx + comp->numresolutions - 1 - resno));
				dy = comp->dy * (1 << (res->pdy + comp->numresolutions - 1 - resno));
				pi->dx = !pi->dx ? dx : int_min(pi->dx, dx);
				pi->dy = !pi->dy ? dy : int_min(pi->dy, dy);
			}
		}
	}
if (!pi->tp_on){
			pi->poc.ty0 = pi->ty0;
			pi->poc.tx0 = pi->tx0;
			pi->poc.ty1 = pi->ty1;
			pi->poc.tx1 = pi->tx1;
		}
	for (pi->resno = pi->poc.resno0; pi->resno < pi->poc.resno1; pi->resno++) {
		for (pi->y = pi->poc.ty0; pi->y < pi->poc.ty1; pi->y += pi->dy - (pi->y % pi->dy)) {
			for (pi->x = pi->poc.tx0; pi->x < pi->poc.tx1; pi->x += pi->dx - (pi->x % pi->dx)) {
				for (pi->compno = pi->poc.compno0; pi->compno < pi->poc.compno1; pi->compno++) {
					int levelno;
					int trx0, try0;
					int trx1, try1;
					int rpx, rpy;
					int prci, prcj;
					comp = &pi->comps[pi->compno];
					if (pi->resno >= comp->numresolutions) {
						continue;
					}
					res = &comp->resolutions[pi->resno];
					levelno = comp->numresolutions - 1 - pi->resno;
					trx0 = int_ceildiv(pi->tx0, comp->dx << levelno);
					try0 = int_ceildiv(pi->ty0, comp->dy << levelno);
					trx1 = int_ceildiv(pi->tx1, comp->dx << levelno);
					try1 = int_ceildiv(pi->ty1, comp->dy << levelno);
					rpx = res->pdx + levelno;
					rpy = res->pdy + levelno;
					if (!((pi->y % (comp->dy << rpy) == 0) || ((pi->y == pi->ty0) && ((try0 << levelno) % (1 << rpy))))){
						continue;	
					}
					if (!((pi->x % (comp->dx << rpx) == 0) || ((pi->x == pi->tx0) && ((trx0 << levelno) % (1 << rpx))))){
						continue; 
					}
					
					if ((res->pw==0)||(res->ph==0)) continue;
					
					if ((trx0==trx1)||(try0==try1)) continue;
					
					prci = int_floordivpow2(int_ceildiv(pi->x, comp->dx << levelno), res->pdx) 
						 - int_floordivpow2(trx0, res->pdx);
					prcj = int_floordivpow2(int_ceildiv(pi->y, comp->dy << levelno), res->pdy) 
						 - int_floordivpow2(try0, res->pdy);
					pi->precno = prci + prcj * res->pw;
					for (pi->layno = pi->poc.layno0; pi->layno < pi->poc.layno1; pi->layno++) {
						index = pi->layno * pi->step_l + pi->resno * pi->step_r + pi->compno * pi->step_c + pi->precno * pi->step_p;
						if (!pi->include[index]) {
							pi->include[index] = 1;
							return OPJ_TRUE;
						}
LABEL_SKIP:;
					}
				}
			}
		}
	}
	
	return OPJ_FALSE;
}

static opj_bool pi_next_pcrl(opj_pi_iterator_t * pi) {
	opj_pi_comp_t *comp = NULL;
	opj_pi_resolution_t *res = NULL;
	long index = 0;

	if (!pi->first) {
		comp = &pi->comps[pi->compno];
		goto LABEL_SKIP;
	} else {
		int compno, resno;
		pi->first = 0;
		pi->dx = 0;
		pi->dy = 0;
		for (compno = 0; compno < pi->numcomps; compno++) {
			comp = &pi->comps[compno];
			for (resno = 0; resno < comp->numresolutions; resno++) {
				int dx, dy;
				res = &comp->resolutions[resno];
				dx = comp->dx * (1 << (res->pdx + comp->numresolutions - 1 - resno));
				dy = comp->dy * (1 << (res->pdy + comp->numresolutions - 1 - resno));
				pi->dx = !pi->dx ? dx : int_min(pi->dx, dx);
				pi->dy = !pi->dy ? dy : int_min(pi->dy, dy);
			}
		}
	}
	if (!pi->tp_on){
			pi->poc.ty0 = pi->ty0;
			pi->poc.tx0 = pi->tx0;
			pi->poc.ty1 = pi->ty1;
			pi->poc.tx1 = pi->tx1;
		}
	for (pi->y = pi->poc.ty0; pi->y < pi->poc.ty1; pi->y += pi->dy - (pi->y % pi->dy)) {
		for (pi->x = pi->poc.tx0; pi->x < pi->poc.tx1; pi->x += pi->dx - (pi->x % pi->dx)) {
			for (pi->compno = pi->poc.compno0; pi->compno < pi->poc.compno1; pi->compno++) {
				comp = &pi->comps[pi->compno];
				for (pi->resno = pi->poc.resno0; pi->resno < int_min(pi->poc.resno1, comp->numresolutions); pi->resno++) {
					int levelno;
					int trx0, try0;
					int trx1, try1;
					int rpx, rpy;
					int prci, prcj;
					res = &comp->resolutions[pi->resno];
					levelno = comp->numresolutions - 1 - pi->resno;
					trx0 = int_ceildiv(pi->tx0, comp->dx << levelno);
					try0 = int_ceildiv(pi->ty0, comp->dy << levelno);
					trx1 = int_ceildiv(pi->tx1, comp->dx << levelno);
					try1 = int_ceildiv(pi->ty1, comp->dy << levelno);
					rpx = res->pdx + levelno;
					rpy = res->pdy + levelno;
					if (!((pi->y % (comp->dy << rpy) == 0) || ((pi->y == pi->ty0) && ((try0 << levelno) % (1 << rpy))))){
						continue;	
					}
					if (!((pi->x % (comp->dx << rpx) == 0) || ((pi->x == pi->tx0) && ((trx0 << levelno) % (1 << rpx))))){
						continue; 
					}
					
					if ((res->pw==0)||(res->ph==0)) continue;
					
					if ((trx0==trx1)||(try0==try1)) continue;
					
					prci = int_floordivpow2(int_ceildiv(pi->x, comp->dx << levelno), res->pdx) 
						 - int_floordivpow2(trx0, res->pdx);
					prcj = int_floordivpow2(int_ceildiv(pi->y, comp->dy << levelno), res->pdy) 
						 - int_floordivpow2(try0, res->pdy);
					pi->precno = prci + prcj * res->pw;
					for (pi->layno = pi->poc.layno0; pi->layno < pi->poc.layno1; pi->layno++) {
						index = pi->layno * pi->step_l + pi->resno * pi->step_r + pi->compno * pi->step_c + pi->precno * pi->step_p;
						if (!pi->include[index]) {
							pi->include[index] = 1;
							return OPJ_TRUE;
						}	
LABEL_SKIP:;
					}
				}
			}
		}
	}
	
	return OPJ_FALSE;
}

static opj_bool pi_next_cprl(opj_pi_iterator_t * pi) {
	opj_pi_comp_t *comp = NULL;
	opj_pi_resolution_t *res = NULL;
	long index = 0;

	if (!pi->first) {
		comp = &pi->comps[pi->compno];
		goto LABEL_SKIP;
	} else {
		pi->first = 0;
	}

	for (pi->compno = pi->poc.compno0; pi->compno < pi->poc.compno1; pi->compno++) {
		int resno;
		comp = &pi->comps[pi->compno];
		pi->dx = 0;
		pi->dy = 0;
		for (resno = 0; resno < comp->numresolutions; resno++) {
			int dx, dy;
			res = &comp->resolutions[resno];
			dx = comp->dx * (1 << (res->pdx + comp->numresolutions - 1 - resno));
			dy = comp->dy * (1 << (res->pdy + comp->numresolutions - 1 - resno));
			pi->dx = !pi->dx ? dx : int_min(pi->dx, dx);
			pi->dy = !pi->dy ? dy : int_min(pi->dy, dy);
		}
		if (!pi->tp_on){
			pi->poc.ty0 = pi->ty0;
			pi->poc.tx0 = pi->tx0;
			pi->poc.ty1 = pi->ty1;
			pi->poc.tx1 = pi->tx1;
		}
		for (pi->y = pi->poc.ty0; pi->y < pi->poc.ty1; pi->y += pi->dy - (pi->y % pi->dy)) {
			for (pi->x = pi->poc.tx0; pi->x < pi->poc.tx1; pi->x += pi->dx - (pi->x % pi->dx)) {
				for (pi->resno = pi->poc.resno0; pi->resno < int_min(pi->poc.resno1, comp->numresolutions); pi->resno++) {
					int levelno;
					int trx0, try0;
					int trx1, try1;
					int rpx, rpy;
					int prci, prcj;
					res = &comp->resolutions[pi->resno];
					levelno = comp->numresolutions - 1 - pi->resno;
					trx0 = int_ceildiv(pi->tx0, comp->dx << levelno);
					try0 = int_ceildiv(pi->ty0, comp->dy << levelno);
					trx1 = int_ceildiv(pi->tx1, comp->dx << levelno);
					try1 = int_ceildiv(pi->ty1, comp->dy << levelno);
					rpx = res->pdx + levelno;
					rpy = res->pdy + levelno;
					if (!((pi->y % (comp->dy << rpy) == 0) || ((pi->y == pi->ty0) && ((try0 << levelno) % (1 << rpy))))){
						continue;	
					}
					if (!((pi->x % (comp->dx << rpx) == 0) || ((pi->x == pi->tx0) && ((trx0 << levelno) % (1 << rpx))))){
						continue; 
					}
					
					if ((res->pw==0)||(res->ph==0)) continue;
					
					if ((trx0==trx1)||(try0==try1)) continue;
					
					prci = int_floordivpow2(int_ceildiv(pi->x, comp->dx << levelno), res->pdx) 
						 - int_floordivpow2(trx0, res->pdx);
					prcj = int_floordivpow2(int_ceildiv(pi->y, comp->dy << levelno), res->pdy) 
						 - int_floordivpow2(try0, res->pdy);
					pi->precno = prci + prcj * res->pw;
					for (pi->layno = pi->poc.layno0; pi->layno < pi->poc.layno1; pi->layno++) {
						index = pi->layno * pi->step_l + pi->resno * pi->step_r + pi->compno * pi->step_c + pi->precno * pi->step_p;
						if (!pi->include[index]) {
							pi->include[index] = 1;
							return OPJ_TRUE;
						}
LABEL_SKIP:;
					}
				}
			}
		}
	}
	
	return OPJ_FALSE;
}

/* 
==========================================================
   Packet iterator interface
==========================================================
*/

opj_pi_iterator_t *pi_create_decode(opj_image_t *image, opj_cp_t *cp, int tileno) {
	int p, q;
	int compno, resno, pino;
	opj_pi_iterator_t *pi = NULL;
	opj_tcp_t *tcp = NULL;
	opj_tccp_t *tccp = NULL;

	tcp = &cp->tcps[tileno];

	pi = (opj_pi_iterator_t*) opj_calloc((tcp->numpocs + 1), sizeof(opj_pi_iterator_t));
	if(!pi) {
		/* TODO: throw an error */
		return NULL;
	}

	for (pino = 0; pino < tcp->numpocs + 1; pino++) {	/* change */
		int maxres = 0;
		int maxprec = 0;
		p = tileno % cp->tw;
		q = tileno / cp->tw;

		pi[pino].tx0 = int_max(cp->tx0 + p * cp->tdx, image->x0);
		pi[pino].ty0 = int_max(cp->ty0 + q * cp->tdy, image->y0);
		pi[pino].tx1 = int_min(cp->tx0 + (p + 1) * cp->tdx, image->x1);
		pi[pino].ty1 = int_min(cp->ty0 + (q + 1) * cp->tdy, image->y1);
		pi[pino].numcomps = image->numcomps;

		pi[pino].comps = (opj_pi_comp_t*) opj_calloc(image->numcomps, sizeof(opj_pi_comp_t));
		if(!pi[pino].comps) {
			/* TODO: throw an error */
			pi_destroy(pi, cp, tileno);
			return NULL;
		}
		
		for (compno = 0; compno < pi->numcomps; compno++) {
			int tcx0, tcy0, tcx1, tcy1;
			opj_pi_comp_t *comp = &pi[pino].comps[compno];
			tccp = &tcp->tccps[compno];
			comp->dx = image->comps[compno].dx;
			comp->dy = image->comps[compno].dy;
			comp->numresolutions = tccp->numresolutions;

			comp->resolutions = (opj_pi_resolution_t*) opj_calloc(comp->numresolutions, sizeof(opj_pi_resolution_t));
			if(!comp->resolutions) {
				/* TODO: throw an error */
				pi_destroy(pi, cp, tileno);
				return NULL;
			}

			tcx0 = int_ceildiv(pi->tx0, comp->dx);
			tcy0 = int_ceildiv(pi->ty0, comp->dy);
			tcx1 = int_ceildiv(pi->tx1, comp->dx);
			tcy1 = int_ceildiv(pi->ty1, comp->dy);
			if (comp->numresolutions > maxres) {
				maxres = comp->numresolutions;
			}

			for (resno = 0; resno < comp->numresolutions; resno++) {
				int levelno;
				int rx0, ry0, rx1, ry1;
				int px0, py0, px1, py1;
				opj_pi_resolution_t *res = &comp->resolutions[resno];
				if (tccp->csty & J2K_CCP_CSTY_PRT) {
					res->pdx = tccp->prcw[resno];
					res->pdy = tccp->prch[resno];
				} else {
					res->pdx = 15;
					res->pdy = 15;
				}
				levelno = comp->numresolutions - 1 - resno;
				rx0 = int_ceildivpow2(tcx0, levelno);
				ry0 = int_ceildivpow2(tcy0, levelno);
				rx1 = int_ceildivpow2(tcx1, levelno);
				ry1 = int_ceildivpow2(tcy1, levelno);
				px0 = int_floordivpow2(rx0, res->pdx) << res->pdx;
				py0 = int_floordivpow2(ry0, res->pdy) << res->pdy;
				px1 = int_ceildivpow2(rx1, res->pdx) << res->pdx;
				py1 = int_ceildivpow2(ry1, res->pdy) << res->pdy;
				res->pw = (rx0==rx1)?0:((px1 - px0) >> res->pdx);
				res->ph = (ry0==ry1)?0:((py1 - py0) >> res->pdy);
				
				if (res->pw*res->ph > maxprec) {
					maxprec = res->pw*res->ph;
				}
				
			}
		}
		
		tccp = &tcp->tccps[0];
		pi[pino].step_p = 1;
		pi[pino].step_c = maxprec * pi[pino].step_p;
		pi[pino].step_r = image->numcomps * pi[pino].step_c;
		pi[pino].step_l = maxres * pi[pino].step_r;
		
		if (pino == 0) {
			pi[pino].include = (short int*) opj_calloc(image->numcomps * maxres * tcp->numlayers * maxprec, sizeof(short int));
			if(!pi[pino].include) {
				/* TODO: throw an error */
				pi_destroy(pi, cp, tileno);
				return NULL;
			}
		}
		else {
			pi[pino].include = pi[pino - 1].include;
		}
		
		if (tcp->POC == 0) {
			pi[pino].first = 1;
			pi[pino].poc.resno0 = 0;
			pi[pino].poc.compno0 = 0;
			pi[pino].poc.layno1 = tcp->numlayers;
			pi[pino].poc.resno1 = maxres;
			pi[pino].poc.compno1 = image->numcomps;
			pi[pino].poc.prg = tcp->prg;
		} else {
			pi[pino].first = 1;
			pi[pino].poc.resno0 = tcp->pocs[pino].resno0;
			pi[pino].poc.compno0 = tcp->pocs[pino].compno0;
			pi[pino].poc.layno1 = tcp->pocs[pino].layno1;
			pi[pino].poc.resno1 = tcp->pocs[pino].resno1;
			pi[pino].poc.compno1 = tcp->pocs[pino].compno1;
			pi[pino].poc.prg = tcp->pocs[pino].prg;
		}
		pi[pino].poc.layno0  = 0;
		pi[pino].poc.precno0 = 0; 
		pi[pino].poc.precno1 = maxprec;
			
	}
	
	return pi;
}


opj_pi_iterator_t *pi_create_decode_v2(	opj_image_t *p_image,
										opj_cp_v2_t *p_cp,
										OPJ_UINT32 p_tile_no
										)
{
	// loop
	OPJ_UINT32 pino;
	OPJ_UINT32 compno, resno;

	// to store w, h, dx and dy fro all components and resolutions
	OPJ_UINT32 * l_tmp_data;
	OPJ_UINT32 ** l_tmp_ptr;

	// encoding prameters to set
	OPJ_UINT32 l_max_res;
	OPJ_UINT32 l_max_prec;
	OPJ_INT32 l_tx0,l_tx1,l_ty0,l_ty1;
	OPJ_UINT32 l_dx_min,l_dy_min;
	OPJ_UINT32 l_bound;
	OPJ_UINT32 l_step_p , l_step_c , l_step_r , l_step_l ;
	OPJ_UINT32 l_data_stride;

	// pointers
	opj_pi_iterator_t *l_pi = 00;
	opj_tcp_v2_t *l_tcp = 00;
	const opj_tccp_t *l_tccp = 00;
	opj_pi_comp_t *l_current_comp = 00;
	opj_image_comp_t * l_img_comp = 00;
	opj_pi_iterator_t * l_current_pi = 00;
	OPJ_UINT32 * l_encoding_value_ptr = 00;

	// preconditions in debug
	assert(p_cp != 00);
	assert(p_image != 00);
	assert(p_tile_no < p_cp->tw * p_cp->th);

	// initializations
	l_tcp = &p_cp->tcps[p_tile_no];
	l_bound = l_tcp->numpocs+1;

	l_data_stride = 4 * J2K_MAXRLVLS;
	l_tmp_data = (OPJ_UINT32*)opj_malloc(
		l_data_stride * p_image->numcomps * sizeof(OPJ_UINT32));
	if
		(! l_tmp_data)
	{
		return 00;
	}
	l_tmp_ptr = (OPJ_UINT32**)opj_malloc(
		p_image->numcomps * sizeof(OPJ_UINT32 *));
	if
		(! l_tmp_ptr)
	{
		opj_free(l_tmp_data);
		return 00;
	}

	// memory allocation for pi
	l_pi = pi_create(p_image, p_cp, p_tile_no);
	if (!l_pi) {
		opj_free(l_tmp_data);
		opj_free(l_tmp_ptr);
		return 00;
	}

	l_encoding_value_ptr = l_tmp_data;
	// update pointer array
	for
		(compno = 0; compno < p_image->numcomps; ++compno)
	{
		l_tmp_ptr[compno] = l_encoding_value_ptr;
		l_encoding_value_ptr += l_data_stride;
	}
	// get encoding parameters
	get_all_encoding_parameters(p_image,p_cp,p_tile_no,&l_tx0,&l_tx1,&l_ty0,&l_ty1,&l_dx_min,&l_dy_min,&l_max_prec,&l_max_res,l_tmp_ptr);

	// step calculations
	l_step_p = 1;
	l_step_c = l_max_prec * l_step_p;
	l_step_r = p_image->numcomps * l_step_c;
	l_step_l = l_max_res * l_step_r;

	// set values for first packet iterator
	l_current_pi = l_pi;

	// memory allocation for include
	l_current_pi->include = (OPJ_INT16*) opj_calloc((l_tcp->numlayers +1) * l_step_l, sizeof(OPJ_INT16));
	if
		(!l_current_pi->include)
	{
		opj_free(l_tmp_data);
		opj_free(l_tmp_ptr);
		pi_destroy_v2(l_pi, l_bound);
		return 00;
	}
	memset(l_current_pi->include,0, (l_tcp->numlayers + 1) * l_step_l* sizeof(OPJ_INT16));

	// special treatment for the first packet iterator
	l_current_comp = l_current_pi->comps;
	l_img_comp = p_image->comps;
	l_tccp = l_tcp->tccps;

	l_current_pi->tx0 = l_tx0;
	l_current_pi->ty0 = l_ty0;
	l_current_pi->tx1 = l_tx1;
	l_current_pi->ty1 = l_ty1;

	//l_current_pi->dx = l_img_comp->dx;
	//l_current_pi->dy = l_img_comp->dy;

	l_current_pi->step_p = l_step_p;
	l_current_pi->step_c = l_step_c;
	l_current_pi->step_r = l_step_r;
	l_current_pi->step_l = l_step_l;

	/* allocation for components and number of components has already been calculated by pi_create */
	for
		(compno = 0; compno < l_current_pi->numcomps; ++compno)
	{
		opj_pi_resolution_t *l_res = l_current_comp->resolutions;
		l_encoding_value_ptr = l_tmp_ptr[compno];

		l_current_comp->dx = l_img_comp->dx;
		l_current_comp->dy = l_img_comp->dy;
		/* resolutions have already been initialized */
		for
			(resno = 0; resno < l_current_comp->numresolutions; resno++)
		{
			l_res->pdx = *(l_encoding_value_ptr++);
			l_res->pdy = *(l_encoding_value_ptr++);
			l_res->pw =  *(l_encoding_value_ptr++);
			l_res->ph =  *(l_encoding_value_ptr++);
			++l_res;
		}
		++l_current_comp;
		++l_img_comp;
		++l_tccp;
	}
	++l_current_pi;

	for
		(pino = 1 ; pino<l_bound ; ++pino )
	{
		opj_pi_comp_t *l_current_comp = l_current_pi->comps;
		opj_image_comp_t * l_img_comp = p_image->comps;
		l_tccp = l_tcp->tccps;

		l_current_pi->tx0 = l_tx0;
		l_current_pi->ty0 = l_ty0;
		l_current_pi->tx1 = l_tx1;
		l_current_pi->ty1 = l_ty1;
		//l_current_pi->dx = l_dx_min;
		//l_current_pi->dy = l_dy_min;
		l_current_pi->step_p = l_step_p;
		l_current_pi->step_c = l_step_c;
		l_current_pi->step_r = l_step_r;
		l_current_pi->step_l = l_step_l;

		/* allocation for components and number of components has already been calculated by pi_create */
		for
			(compno = 0; compno < l_current_pi->numcomps; ++compno)
		{
			opj_pi_resolution_t *l_res = l_current_comp->resolutions;
			l_encoding_value_ptr = l_tmp_ptr[compno];

			l_current_comp->dx = l_img_comp->dx;
			l_current_comp->dy = l_img_comp->dy;
			/* resolutions have already been initialized */
			for
				(resno = 0; resno < l_current_comp->numresolutions; resno++)
			{
				l_res->pdx = *(l_encoding_value_ptr++);
				l_res->pdy = *(l_encoding_value_ptr++);
				l_res->pw =  *(l_encoding_value_ptr++);
				l_res->ph =  *(l_encoding_value_ptr++);
				++l_res;
			}
			++l_current_comp;
			++l_img_comp;
			++l_tccp;
		}
		// special treatment
		l_current_pi->include = (l_current_pi-1)->include;
		++l_current_pi;
	}
	opj_free(l_tmp_data);
	l_tmp_data = 00;
	opj_free(l_tmp_ptr);
	l_tmp_ptr = 00;
	if
		(l_tcp->POC)
	{
		pi_update_decode_poc (l_pi,l_tcp,l_max_prec,l_max_res);
	}
	else
	{
		pi_update_decode_not_poc(l_pi,l_tcp,l_max_prec,l_max_res);
	}
	return l_pi;
}

opj_pi_iterator_t *pi_initialise_encode(opj_image_t *image, opj_cp_t *cp, int tileno, J2K_T2_MODE t2_mode){ 
	int p, q, pino;
	int compno, resno;
	int maxres = 0;
	int maxprec = 0;
	opj_pi_iterator_t *pi = NULL;
	opj_tcp_t *tcp = NULL;
	opj_tccp_t *tccp = NULL;
	
	tcp = &cp->tcps[tileno];

	pi = (opj_pi_iterator_t*) opj_calloc((tcp->numpocs + 1), sizeof(opj_pi_iterator_t));
	if(!pi) {	return NULL;}
	pi->tp_on = cp->tp_on;

	for(pino = 0;pino < tcp->numpocs+1 ; pino ++){
		p = tileno % cp->tw;
		q = tileno / cp->tw;

		pi[pino].tx0 = int_max(cp->tx0 + p * cp->tdx, image->x0);
		pi[pino].ty0 = int_max(cp->ty0 + q * cp->tdy, image->y0);
		pi[pino].tx1 = int_min(cp->tx0 + (p + 1) * cp->tdx, image->x1);
		pi[pino].ty1 = int_min(cp->ty0 + (q + 1) * cp->tdy, image->y1);
		pi[pino].numcomps = image->numcomps;
		
		pi[pino].comps = (opj_pi_comp_t*) opj_calloc(image->numcomps, sizeof(opj_pi_comp_t));
		if(!pi[pino].comps) {
			pi_destroy(pi, cp, tileno);
			return NULL;
		}
		
		for (compno = 0; compno < pi[pino].numcomps; compno++) {
			int tcx0, tcy0, tcx1, tcy1;
			opj_pi_comp_t *comp = &pi[pino].comps[compno];
			tccp = &tcp->tccps[compno];
			comp->dx = image->comps[compno].dx;
			comp->dy = image->comps[compno].dy;
			comp->numresolutions = tccp->numresolutions;

			comp->resolutions = (opj_pi_resolution_t*) opj_malloc(comp->numresolutions * sizeof(opj_pi_resolution_t));
			if(!comp->resolutions) {
				pi_destroy(pi, cp, tileno);
				return NULL;
			}

			tcx0 = int_ceildiv(pi[pino].tx0, comp->dx);
			tcy0 = int_ceildiv(pi[pino].ty0, comp->dy);
			tcx1 = int_ceildiv(pi[pino].tx1, comp->dx);
			tcy1 = int_ceildiv(pi[pino].ty1, comp->dy);
			if (comp->numresolutions > maxres) {
				maxres = comp->numresolutions;
			}

			for (resno = 0; resno < comp->numresolutions; resno++) {
				int levelno;
				int rx0, ry0, rx1, ry1;
				int px0, py0, px1, py1;
				opj_pi_resolution_t *res = &comp->resolutions[resno];
				if (tccp->csty & J2K_CCP_CSTY_PRT) {
					res->pdx = tccp->prcw[resno];
					res->pdy = tccp->prch[resno];
				} else {
					res->pdx = 15;
					res->pdy = 15;
				}
				levelno = comp->numresolutions - 1 - resno;
				rx0 = int_ceildivpow2(tcx0, levelno);
				ry0 = int_ceildivpow2(tcy0, levelno);
				rx1 = int_ceildivpow2(tcx1, levelno);
				ry1 = int_ceildivpow2(tcy1, levelno);
				px0 = int_floordivpow2(rx0, res->pdx) << res->pdx;
				py0 = int_floordivpow2(ry0, res->pdy) << res->pdy;
				px1 = int_ceildivpow2(rx1, res->pdx) << res->pdx;
				py1 = int_ceildivpow2(ry1, res->pdy) << res->pdy;
				res->pw = (rx0==rx1)?0:((px1 - px0) >> res->pdx);
				res->ph = (ry0==ry1)?0:((py1 - py0) >> res->pdy);

				if (res->pw*res->ph > maxprec) {
					maxprec = res->pw * res->ph;
				}
			}
		}
		
		tccp = &tcp->tccps[0];
		pi[pino].step_p = 1;
		pi[pino].step_c = maxprec * pi[pino].step_p;
		pi[pino].step_r = image->numcomps * pi[pino].step_c;
		pi[pino].step_l = maxres * pi[pino].step_r;
		
		for (compno = 0; compno < pi->numcomps; compno++) {
			opj_pi_comp_t *comp = &pi->comps[compno];
			for (resno = 0; resno < comp->numresolutions; resno++) {
				int dx, dy;
				opj_pi_resolution_t *res = &comp->resolutions[resno];
				dx = comp->dx * (1 << (res->pdx + comp->numresolutions - 1 - resno));
				dy = comp->dy * (1 << (res->pdy + comp->numresolutions - 1 - resno));
				pi[pino].dx = !pi->dx ? dx : int_min(pi->dx, dx);
				pi[pino].dy = !pi->dy ? dy : int_min(pi->dy, dy);
			}
		}

		if (pino == 0) {
			pi[pino].include = (short int*) opj_calloc(tcp->numlayers * pi[pino].step_l, sizeof(short int));
			if(!pi[pino].include) {
				pi_destroy(pi, cp, tileno);
				return NULL;
			}
		}
		else {
			pi[pino].include = pi[pino - 1].include;
		}
		
		/* Generation of boundaries for each prog flag*/
			if(tcp->POC && ( cp->cinema || ((!cp->cinema) && (t2_mode == FINAL_PASS)))){
				tcp->pocs[pino].compS= tcp->pocs[pino].compno0;
				tcp->pocs[pino].compE= tcp->pocs[pino].compno1;
				tcp->pocs[pino].resS = tcp->pocs[pino].resno0;
				tcp->pocs[pino].resE = tcp->pocs[pino].resno1;
				tcp->pocs[pino].layE = tcp->pocs[pino].layno1;
				tcp->pocs[pino].prg  = tcp->pocs[pino].prg1;
				if (pino > 0)
					tcp->pocs[pino].layS = (tcp->pocs[pino].layE > tcp->pocs[pino - 1].layE) ? tcp->pocs[pino - 1].layE : 0;
			}else {
				tcp->pocs[pino].compS= 0;
				tcp->pocs[pino].compE= image->numcomps;
				tcp->pocs[pino].resS = 0;
				tcp->pocs[pino].resE = maxres;
				tcp->pocs[pino].layS = 0;
				tcp->pocs[pino].layE = tcp->numlayers;
				tcp->pocs[pino].prg  = tcp->prg;
			}
			tcp->pocs[pino].prcS = 0;
			tcp->pocs[pino].prcE = maxprec;;
			tcp->pocs[pino].txS = pi[pino].tx0;
			tcp->pocs[pino].txE = pi[pino].tx1;
			tcp->pocs[pino].tyS = pi[pino].ty0;
			tcp->pocs[pino].tyE = pi[pino].ty1;
			tcp->pocs[pino].dx = pi[pino].dx;
			tcp->pocs[pino].dy = pi[pino].dy;
		}
			return pi;
	}



void pi_destroy(opj_pi_iterator_t *pi, opj_cp_t *cp, int tileno) {
	int compno, pino;
	opj_tcp_t *tcp = &cp->tcps[tileno];
	if(pi) {
		for (pino = 0; pino < tcp->numpocs + 1; pino++) {	
			if(pi[pino].comps) {
				for (compno = 0; compno < pi->numcomps; compno++) {
					opj_pi_comp_t *comp = &pi[pino].comps[compno];
					if(comp->resolutions) {
						opj_free(comp->resolutions);
					}
				}
				opj_free(pi[pino].comps);
			}
		}
		if(pi->include) {
			opj_free(pi->include);
		}
		opj_free(pi);
	}
}

opj_bool pi_next(opj_pi_iterator_t * pi) {
	switch (pi->poc.prg) {
		case LRCP:
			return pi_next_lrcp(pi);
		case RLCP:
			return pi_next_rlcp(pi);
		case RPCL:
			return pi_next_rpcl(pi);
		case PCRL:
			return pi_next_pcrl(pi);
		case CPRL:
			return pi_next_cprl(pi);
		case PROG_UNKNOWN:
			return OPJ_FALSE;
	}
	
	return OPJ_FALSE;
}

opj_bool pi_create_encode( opj_pi_iterator_t *pi, opj_cp_t *cp,int tileno, int pino,int tpnum, int tppos, J2K_T2_MODE t2_mode,int cur_totnum_tp){
	char prog[4];
	int i;
	int incr_top=1,resetX=0;
	opj_tcp_t *tcps =&cp->tcps[tileno];
	opj_poc_t *tcp= &tcps->pocs[pino];

	pi[pino].first = 1;
	pi[pino].poc.prg = tcp->prg;

	switch(tcp->prg){
		case CPRL: strncpy(prog, "CPRL",4);
			break;
		case LRCP: strncpy(prog, "LRCP",4);
			break;
		case PCRL: strncpy(prog, "PCRL",4);
			break;
		case RLCP: strncpy(prog, "RLCP",4);
			break;
		case RPCL: strncpy(prog, "RPCL",4);
			break;
		case PROG_UNKNOWN: 
			return OPJ_TRUE;
	}

	if(!(cp->tp_on && ((!cp->cinema && (t2_mode == FINAL_PASS)) || cp->cinema))){
		pi[pino].poc.resno0 = tcp->resS;
		pi[pino].poc.resno1 = tcp->resE;
		pi[pino].poc.compno0 = tcp->compS;
		pi[pino].poc.compno1 = tcp->compE;
		pi[pino].poc.layno0 = tcp->layS;
		pi[pino].poc.layno1 = tcp->layE;
		pi[pino].poc.precno0 = tcp->prcS;
		pi[pino].poc.precno1 = tcp->prcE;
		pi[pino].poc.tx0 = tcp->txS;
		pi[pino].poc.ty0 = tcp->tyS;
		pi[pino].poc.tx1 = tcp->txE;
		pi[pino].poc.ty1 = tcp->tyE;
	}else {
		if( tpnum < cur_totnum_tp){
			for(i=3;i>=0;i--){
				switch(prog[i]){
				case 'C':
					if (i > tppos){
						pi[pino].poc.compno0 = tcp->compS;
						pi[pino].poc.compno1 = tcp->compE;
					}else{
						if (tpnum == 0){
							tcp->comp_t = tcp->compS;
							pi[pino].poc.compno0 = tcp->comp_t;
							pi[pino].poc.compno1 = tcp->comp_t+1;
							tcp->comp_t+=1;
						}else{
							if (incr_top == 1){
								if(tcp->comp_t ==tcp->compE){
									tcp->comp_t = tcp->compS;
									pi[pino].poc.compno0 = tcp->comp_t;
									pi[pino].poc.compno1 = tcp->comp_t+1;
									tcp->comp_t+=1;
									incr_top=1;
								}else{
									pi[pino].poc.compno0 = tcp->comp_t;
									pi[pino].poc.compno1 = tcp->comp_t+1;
									tcp->comp_t+=1;
									incr_top=0;
								}
							}else{
								pi[pino].poc.compno0 = tcp->comp_t-1;
								pi[pino].poc.compno1 = tcp->comp_t;
							}
						}
					}
					break;

				case 'R':
					if (i > tppos){
						pi[pino].poc.resno0 = tcp->resS;
						pi[pino].poc.resno1 = tcp->resE;
					}else{
						if (tpnum == 0){
							tcp->res_t = tcp->resS;
							pi[pino].poc.resno0 = tcp->res_t;
							pi[pino].poc.resno1 = tcp->res_t+1;
							tcp->res_t+=1;
						}else{
							if (incr_top == 1){
								if(tcp->res_t==tcp->resE){
									tcp->res_t = tcp->resS;
									pi[pino].poc.resno0 = tcp->res_t;
									pi[pino].poc.resno1 = tcp->res_t+1;
									tcp->res_t+=1;
									incr_top=1;
								}else{
									pi[pino].poc.resno0 = tcp->res_t;
									pi[pino].poc.resno1 = tcp->res_t+1;
									tcp->res_t+=1;
									incr_top=0;
								}
							}else{
								pi[pino].poc.resno0 = tcp->res_t - 1;
								pi[pino].poc.resno1 = tcp->res_t;
							}
						}
					}
					break;

				case 'L':
					if (i > tppos){
						pi[pino].poc.layno0 = tcp->layS;
						pi[pino].poc.layno1 = tcp->layE;
					}else{
						if (tpnum == 0){
							tcp->lay_t = tcp->layS;
							pi[pino].poc.layno0 = tcp->lay_t;
							pi[pino].poc.layno1 = tcp->lay_t+1;
							tcp->lay_t+=1;
						}else{
							if (incr_top == 1){
								if(tcp->lay_t == tcp->layE){
									tcp->lay_t = tcp->layS;
									pi[pino].poc.layno0 = tcp->lay_t;
									pi[pino].poc.layno1 = tcp->lay_t+1;
									tcp->lay_t+=1;
									incr_top=1;
								}else{
									pi[pino].poc.layno0 = tcp->lay_t;
									pi[pino].poc.layno1 = tcp->lay_t+1;
									tcp->lay_t+=1;
									incr_top=0;
								}
							}else{
								pi[pino].poc.layno0 = tcp->lay_t - 1;
								pi[pino].poc.layno1 = tcp->lay_t;
							}
						}
					}
					break;

				case 'P':
					switch(tcp->prg){
						case LRCP:
						case RLCP:
							if (i > tppos){
								pi[pino].poc.precno0 = tcp->prcS;
								pi[pino].poc.precno1 = tcp->prcE;
							}else{
								if (tpnum == 0){
									tcp->prc_t = tcp->prcS;
									pi[pino].poc.precno0 = tcp->prc_t;
									pi[pino].poc.precno1 = tcp->prc_t+1;
									tcp->prc_t+=1; 
								}else{
									if (incr_top == 1){
										if(tcp->prc_t == tcp->prcE){
											tcp->prc_t = tcp->prcS;
											pi[pino].poc.precno0 = tcp->prc_t;
											pi[pino].poc.precno1 = tcp->prc_t+1;
											tcp->prc_t+=1;
											incr_top=1;
										}else{
											pi[pino].poc.precno0 = tcp->prc_t;
											pi[pino].poc.precno1 = tcp->prc_t+1;
											tcp->prc_t+=1;
											incr_top=0;
										}
									}else{
										pi[pino].poc.precno0 = tcp->prc_t - 1;
										pi[pino].poc.precno1 = tcp->prc_t;
									}
								}
							}
						break;
						default:
							if (i > tppos){
								pi[pino].poc.tx0 = tcp->txS;
								pi[pino].poc.ty0 = tcp->tyS;
								pi[pino].poc.tx1 = tcp->txE;
								pi[pino].poc.ty1 = tcp->tyE;
							}else{
								if (tpnum == 0){
									tcp->tx0_t = tcp->txS;
									tcp->ty0_t = tcp->tyS;
									pi[pino].poc.tx0 = tcp->tx0_t;
									pi[pino].poc.tx1 = tcp->tx0_t + tcp->dx - (tcp->tx0_t % tcp->dx);
									pi[pino].poc.ty0 = tcp->ty0_t;
									pi[pino].poc.ty1 = tcp->ty0_t + tcp->dy - (tcp->ty0_t % tcp->dy);
									tcp->tx0_t = pi[pino].poc.tx1;
									tcp->ty0_t = pi[pino].poc.ty1;
								}else{
									if (incr_top == 1){
										if(tcp->tx0_t >= tcp->txE){
											if(tcp->ty0_t >= tcp->tyE){
												tcp->ty0_t = tcp->tyS;
												pi[pino].poc.ty0 = tcp->ty0_t;
												pi[pino].poc.ty1 = tcp->ty0_t + tcp->dy - (tcp->ty0_t % tcp->dy);
												tcp->ty0_t = pi[pino].poc.ty1;
												incr_top=1;resetX=1;
											}else{
												pi[pino].poc.ty0 = tcp->ty0_t;
												pi[pino].poc.ty1 = tcp->ty0_t + tcp->dy - (tcp->ty0_t % tcp->dy);
												tcp->ty0_t = pi[pino].poc.ty1;
												incr_top=0;resetX=1;
											}
											if(resetX==1){
												tcp->tx0_t = tcp->txS;
												pi[pino].poc.tx0 = tcp->tx0_t;
												pi[pino].poc.tx1 = tcp->tx0_t + tcp->dx- (tcp->tx0_t % tcp->dx);
												tcp->tx0_t = pi[pino].poc.tx1;
											}
										}else{
											pi[pino].poc.tx0 = tcp->tx0_t;
											pi[pino].poc.tx1 = tcp->tx0_t + tcp->dx- (tcp->tx0_t % tcp->dx);
											tcp->tx0_t = pi[pino].poc.tx1;
											pi[pino].poc.ty0 = tcp->ty0_t - tcp->dy - (tcp->ty0_t % tcp->dy);
											pi[pino].poc.ty1 = tcp->ty0_t ;
											incr_top=0;
										}
									}else{
										pi[pino].poc.tx0 = tcp->tx0_t - tcp->dx - (tcp->tx0_t % tcp->dx);
										pi[pino].poc.tx1 = tcp->tx0_t ;
										pi[pino].poc.ty0 = tcp->ty0_t - tcp->dy - (tcp->ty0_t % tcp->dy);
										pi[pino].poc.ty1 = tcp->ty0_t ;
									}
								}
							}
						break;
						}
						break;
				}		
			} 
		}
	}	
	return OPJ_FALSE;
}



/**
 * Gets the encoding parameters needed to update the coding parameters and all the pocs.
 * The precinct widths, heights, dx and dy for each component at each resolution will be stored as well.
 * the last parameter of the function should be an array of pointers of size nb components, each pointer leading
 * to an area of size 4 * max_res. The data is stored inside this area with the following pattern :
 * dx_compi_res0 , dy_compi_res0 , w_compi_res0, h_compi_res0 , dx_compi_res1 , dy_compi_res1 , w_compi_res1, h_compi_res1 , ...
 *
 * @param	p_image			the image being encoded.
 * @param	p_cp			the coding parameters.
 * @param	tileno			the tile index of the tile being encoded.
 * @param	p_tx0			pointer that will hold the X0 parameter for the tile
 * @param	p_tx1			pointer that will hold the X1 parameter for the tile
 * @param	p_ty0			pointer that will hold the Y0 parameter for the tile
 * @param	p_ty1			pointer that will hold the Y1 parameter for the tile
 * @param	p_max_prec		pointer that will hold the the maximum precision for all the bands of the tile
 * @param	p_max_res		pointer that will hold the the maximum number of resolutions for all the poc inside the tile.
 * @param	p_dx_min		pointer that will hold the the minimum dx of all the components of all the resolutions for the tile.
 * @param	p_dy_min		pointer that will hold the the minimum dy of all the components of all the resolutions for the tile.
 * @param	p_resolutions	pointer to an area corresponding to the one described above.
 */
void get_all_encoding_parameters(
								const opj_image_t *p_image,
								const opj_cp_v2_t *p_cp,
								OPJ_UINT32 tileno,
								OPJ_INT32 * p_tx0,
								OPJ_INT32 * p_tx1,
								OPJ_INT32 * p_ty0,
								OPJ_INT32 * p_ty1,
								OPJ_UINT32 * p_dx_min,
								OPJ_UINT32 * p_dy_min,
								OPJ_UINT32 * p_max_prec,
								OPJ_UINT32 * p_max_res,
								OPJ_UINT32 ** p_resolutions
							)
{
	// loop
	OPJ_UINT32 compno, resno;

	// pointers
	const opj_tcp_v2_t *tcp = 00;
	const opj_tccp_t * l_tccp = 00;
	const opj_image_comp_t * l_img_comp = 00;

	// to store l_dx, l_dy, w and h for each resolution and component.
	OPJ_UINT32 * lResolutionPtr;

	// position in x and y of tile
	OPJ_UINT32 p, q;

	// preconditions in debug
	assert(p_cp != 00);
	assert(p_image != 00);
	assert(tileno < p_cp->tw * p_cp->th);

	// initializations
	tcp = &p_cp->tcps [tileno];
	l_tccp = tcp->tccps;
	l_img_comp = p_image->comps;

	// position in x and y of tile

	p = tileno % p_cp->tw;
	q = tileno / p_cp->tw;

	/* here calculation of tx0, tx1, ty0, ty1, maxprec, l_dx and l_dy */
	*p_tx0 = int_max(p_cp->tx0 + p * p_cp->tdx, p_image->x0);
	*p_tx1 = int_min(p_cp->tx0 + (p + 1) * p_cp->tdx, p_image->x1);
	*p_ty0 = int_max(p_cp->ty0 + q * p_cp->tdy, p_image->y0);
	*p_ty1 = int_min(p_cp->ty0 + (q + 1) * p_cp->tdy, p_image->y1);

	// max precision and resolution is 0 (can only grow)
	*p_max_prec = 0;
	*p_max_res = 0;

	// take the largest value for dx_min and dy_min
	*p_dx_min = 0x7fffffff;
	*p_dy_min  = 0x7fffffff;

	for
		(compno = 0; compno < p_image->numcomps; ++compno)
	{
		// aritmetic variables to calculate
		OPJ_UINT32 l_level_no;
		OPJ_INT32 l_rx0, l_ry0, l_rx1, l_ry1;
		OPJ_INT32 l_px0, l_py0, l_px1, py1;
		OPJ_UINT32 l_product;
		OPJ_INT32 l_tcx0, l_tcy0, l_tcx1, l_tcy1;
		OPJ_UINT32 l_pdx, l_pdy , l_pw , l_ph;

		lResolutionPtr = p_resolutions[compno];

		l_tcx0 = int_ceildiv(*p_tx0, l_img_comp->dx);
		l_tcy0 = int_ceildiv(*p_ty0, l_img_comp->dy);
		l_tcx1 = int_ceildiv(*p_tx1, l_img_comp->dx);
		l_tcy1 = int_ceildiv(*p_ty1, l_img_comp->dy);
		if
			(l_tccp->numresolutions > *p_max_res)
		{
			*p_max_res = l_tccp->numresolutions;
		}

		// use custom size for precincts
		l_level_no = l_tccp->numresolutions - 1;
		for
			(resno = 0; resno < l_tccp->numresolutions; ++resno)
		{
			OPJ_UINT32 l_dx, l_dy;
			// precinct width and height
			l_pdx = l_tccp->prcw[resno];
			l_pdy = l_tccp->prch[resno];
			*lResolutionPtr++ = l_pdx;
			*lResolutionPtr++ = l_pdy;
			l_dx = l_img_comp->dx * (1 << (l_pdx + l_level_no));
			l_dy = l_img_comp->dy * (1 << (l_pdy + l_level_no));
			// take the minimum size for l_dx for each comp and resolution
			*p_dx_min = int_min(*p_dx_min, l_dx);
			*p_dy_min = int_min(*p_dy_min, l_dy);
			// various calculations of extents

			l_rx0 = int_ceildivpow2(l_tcx0, l_level_no);
			l_ry0 = int_ceildivpow2(l_tcy0, l_level_no);
			l_rx1 = int_ceildivpow2(l_tcx1, l_level_no);
			l_ry1 = int_ceildivpow2(l_tcy1, l_level_no);
			l_px0 = int_floordivpow2(l_rx0, l_pdx) << l_pdx;
			l_py0 = int_floordivpow2(l_ry0, l_pdy) << l_pdy;
			l_px1 = int_ceildivpow2(l_rx1, l_pdx) << l_pdx;
			py1 = int_ceildivpow2(l_ry1, l_pdy) << l_pdy;
			l_pw = (l_rx0==l_rx1)?0:((l_px1 - l_px0) >> l_pdx);
			l_ph = (l_ry0==l_ry1)?0:((py1 - l_py0) >> l_pdy);
			*lResolutionPtr++ = l_pw;
			*lResolutionPtr++ = l_ph;
			l_product = l_pw * l_ph;
			// update precision
			if
				(l_product > *p_max_prec)
			{
				*p_max_prec = l_product;
			}
			--l_level_no;
		}
		++l_tccp;
		++l_img_comp;
	}
}

/**
 * Allocates memory for a packet iterator. Data and data sizes are set by this operation.
 * No other data is set. The include section of the packet  iterator is not allocated.
 *
 * @param	image		the image used to initialize the packet iterator (only the number of components is relevant.
 * @param	cp		the coding parameters.
 * @param	tileno		the index of the tile from which creating the packet iterator.
 */
opj_pi_iterator_t * pi_create(	const opj_image_t *image,
								const opj_cp_v2_t *cp,
								OPJ_UINT32 tileno )
{
	// loop
	OPJ_UINT32 pino, compno;
	// number of poc in the p_pi
	OPJ_UINT32 l_poc_bound;

	// pointers to tile coding parameters and components.
	opj_pi_iterator_t *l_pi = 00;
	opj_tcp_v2_t *tcp = 00;
	const opj_tccp_t *tccp = 00;

	// current packet iterator being allocated
	opj_pi_iterator_t *l_current_pi = 00;

	// preconditions in debug
	assert(cp != 00);
	assert(image != 00);
	assert(tileno < cp->tw * cp->th);

	// initializations
	tcp = &cp->tcps[tileno];
	l_poc_bound = tcp->numpocs+1;

	// memory allocations
	l_pi = (opj_pi_iterator_t*) opj_calloc((l_poc_bound), sizeof(opj_pi_iterator_t));
	if (!l_pi) {
		return NULL;
	}
	memset(l_pi,0,l_poc_bound * sizeof(opj_pi_iterator_t));

	l_current_pi = l_pi;
	for (pino = 0; pino < l_poc_bound ; ++pino) {

		l_current_pi->comps = (opj_pi_comp_t*) opj_calloc(image->numcomps, sizeof(opj_pi_comp_t));
		if (! l_current_pi->comps) {
			pi_destroy_v2(l_pi, l_poc_bound);
			return NULL;
		}

		l_current_pi->numcomps = image->numcomps;
		memset(l_current_pi->comps,0,image->numcomps * sizeof(opj_pi_comp_t));

		for (compno = 0; compno < image->numcomps; ++compno) {
			opj_pi_comp_t *comp = &l_current_pi->comps[compno];

			tccp = &tcp->tccps[compno];

			comp->resolutions = (opj_pi_resolution_t*) opj_malloc(tccp->numresolutions * sizeof(opj_pi_resolution_t));
			if (!comp->resolutions) {
				pi_destroy_v2(l_pi, l_poc_bound);
				return 00;
			}

			comp->numresolutions = tccp->numresolutions;
			memset(comp->resolutions,0,tccp->numresolutions * sizeof(opj_pi_resolution_t));
		}
		++l_current_pi;
	}
	return l_pi;
}



/**
 * Destroys a packet iterator array.
 *
 * @param	p_pi			the packet iterator array to destroy.
 * @param	p_nb_elements	the number of elements in the array.
 */
void pi_destroy_v2(
				opj_pi_iterator_t *p_pi,
				OPJ_UINT32 p_nb_elements)
{
	OPJ_UINT32 compno, pino;
	opj_pi_iterator_t *l_current_pi = p_pi;
	if
		(p_pi)
	{
		if
			(p_pi->include)
		{
			opj_free(p_pi->include);
			p_pi->include = 00;
		}
		// TODO
		for
			(pino = 0; pino < p_nb_elements; ++pino)
		{
			if
				(l_current_pi->comps)
			{
				opj_pi_comp_t *l_current_component = l_current_pi->comps;
				for
					(compno = 0; compno < l_current_pi->numcomps; compno++)
				{
					if
						(l_current_component->resolutions)
					{
						opj_free(l_current_component->resolutions);
						l_current_component->resolutions = 00;
					}
					++l_current_component;
				}
				opj_free(l_current_pi->comps);
				l_current_pi->comps = 0;
			}
			++l_current_pi;
		}
		opj_free(p_pi);
	}
}



void pi_update_decode_poc (opj_pi_iterator_t * p_pi,opj_tcp_v2_t * p_tcp,OPJ_UINT32 p_max_precision,OPJ_UINT32 p_max_res)
{
	// loop
	OPJ_UINT32 pino;

	// encoding prameters to set
	OPJ_UINT32 l_bound;

	opj_pi_iterator_t * l_current_pi = 00;
	opj_poc_t* l_current_poc = 0;

	// preconditions in debug
	assert(p_pi != 00);
	assert(p_tcp != 00);

	// initializations
	l_bound = p_tcp->numpocs+1;
	l_current_pi = p_pi;
	l_current_poc = p_tcp->pocs;

	for
		(pino = 0;pino<l_bound;++pino)
	{
		l_current_pi->poc.prg = l_current_poc->prg;
		l_current_pi->first = 1;

		l_current_pi->poc.resno0 = l_current_poc->resno0;
		l_current_pi->poc.compno0 = l_current_poc->compno0;
		l_current_pi->poc.layno0 = 0;
		l_current_pi->poc.precno0 = 0;
		l_current_pi->poc.resno1 = l_current_poc->resno1;
		l_current_pi->poc.compno1 = l_current_poc->compno1;
		l_current_pi->poc.layno1 = l_current_poc->layno1;
		l_current_pi->poc.precno1 = p_max_precision;
		++l_current_pi;
		++l_current_poc;
	}
}


void pi_update_decode_not_poc (opj_pi_iterator_t * p_pi,opj_tcp_v2_t * p_tcp,OPJ_UINT32 p_max_precision,OPJ_UINT32 p_max_res)
{
	// loop
	OPJ_UINT32 pino;

	// encoding prameters to set
	OPJ_UINT32 l_bound;

	opj_pi_iterator_t * l_current_pi = 00;
	// preconditions in debug
	assert(p_tcp != 00);
	assert(p_pi != 00);

	// initializations
	l_bound = p_tcp->numpocs+1;
	l_current_pi = p_pi;

	for
		(pino = 0;pino<l_bound;++pino)
	{
		l_current_pi->poc.prg = p_tcp->prg;
		l_current_pi->first = 1;
		l_current_pi->poc.resno0 = 0;
		l_current_pi->poc.compno0 = 0;
		l_current_pi->poc.layno0 = 0;
		l_current_pi->poc.precno0 = 0;
		l_current_pi->poc.resno1 = p_max_res;
		l_current_pi->poc.compno1 = l_current_pi->numcomps;
		l_current_pi->poc.layno1 = p_tcp->numlayers;
		l_current_pi->poc.precno1 = p_max_precision;
		++l_current_pi;
	}
}
