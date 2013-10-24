/*
 *	Copyright (c) 2013, Nenad Markus
 *	All rights reserved.
 *
 *	This is an implementation of the algorithm described in the following paper:
 *		N. Markus, M. Frljak, I. S. Pandzic, J. Ahlberg and R. Forchheimer,
 *		A method for object detection based on pixel intensity comparisons,
 *		http://arxiv.org/abs/1305.4537
 *
 *	Redistribution and use of this program as source code or in binary form, with or without modifications, are permitted provided that the following conditions are met:
 *		1. Redistributions may not be sold, nor may they be used in a commercial product or activity without prior permission from the copyright holder (contact him at nenad.markus@fer.hr).
 *		2. Redistributions may not be used for military purposes.
 *		3. Any published work which utilizes this program shall include the reference to the paper available at http://arxiv.org/abs/1305.4537
 *		4. Redistributions must retain the above copyright notice and the reference to the algorithm on which the implementation is based on, this list of conditions and the following disclaimer.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "rotodet.h"

#include <math.h>

/*
	
*/

#ifdef _WIN32
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int32 int32_t;
#else
#include <stdint.h>
#endif

/*
	
*/

	#define NROTS 10

	static int rrotlut[256][256][NROTS];
	static int crotlut[256][256][NROTS];
	
	void precompute_rotluts()
	{
		int i, j, t;
		
		const float pi = 3.14159265f;

		//
		for(t=0; t<NROTS; ++t)
			for(i=0; i<256; ++i)
				for(j=0; j<256; ++j)
				{
					uint8_t rcode;
					uint8_t ccode;

					int r, c;
					float theta;

					//
					rcode = i;
					ccode = j;

					r = *(int8_t*)&rcode;
					c = *(int8_t*)&ccode;

					//
					theta = t*2.0f*pi/NROTS;

					//
					rrotlut[i][j][t] = (int)(+r*cos(theta) + c*sin(theta));
					crotlut[i][j][t] = (int)(-r*sin(theta) + c*cos(theta));
				}
	}

	int bintest(int tcode, int r, int c, int s, int rotind, unsigned char pixels[], int nrows, int ncols, int ldim)
	{
		//
		int r1, c1, r2, c2;

		//
		uint8_t* p = (uint8_t*)&tcode;

		r1 = (256*r + rrotlut[p[0]][p[1]][rotind]*s)/256;
		c1 = (256*c + crotlut[p[0]][p[1]][rotind]*s)/256;

		r2 = (256*r + rrotlut[p[2]][p[3]][rotind]*s)/256;
		c2 = (256*c + crotlut[p[2]][p[3]][rotind]*s)/256;

		//
		return pixels[r1*ldim+c1] <= pixels[r2*ldim+c2];
	}

/*
	
*/
	float get_dtree_output(int8_t tree[], int r, int c, int s, int rotind, unsigned char pixels[], int nrows, int ncols, int ldim)
	{
		int d, lutidx, idx;

		int32_t tdepth;
		int32_t* tcodes;
		float* tlut;

		//
		//toutdim = *(int32*)&tree[0];
		tdepth = *(int32_t*)&tree[sizeof(int32_t)];
		tcodes = (int32_t*)&tree[2*sizeof(int32_t)];
		tlut = (float*)&tree[(2+((1<<tdepth)-1))*sizeof(int32_t)];

		//
		idx = 0;

		for(d=0; d<tdepth; ++d)
		{
			if( bintest(tcodes[idx], r, c, s, rotind, pixels, nrows, ncols, ldim) )	
				idx = 2*idx + 1;
			else
				idx = 2*idx + 2;
		}

		//
		lutidx = idx - ((1<<tdepth)-1);
		
		//
		return tlut[lutidx];
	}
	
	int get_dtree_size(int8_t dtree[])
	{
		int32_t tdepth, toutdim;
		
		//
		toutdim = *(int32_t*)&dtree[0];
		tdepth = *(int32_t*)&dtree[sizeof(int32_t)];
		
		//
		return 2*sizeof(int32_t) + ((1<<tdepth)-1)*sizeof(int32_t) + (1<<tdepth)*toutdim*sizeof(float);
	}

/*
	
*/
	int odet_classify_region(void* od, float* o, float r_f, float c_f, float s_f, int rotind, unsigned char pixels[], int nrows, int ncols, int ldim)
	{
		int8_t* ptr;
		int loc;

		int32_t nstages;
		float ratio;

		int i, j;
		int r, c, s;
		float treshold;

		//
		ptr = (int8_t*)od;
		loc = 0;
		
		*o = 0.0f;

		//
		ratio = *(float*)&ptr[loc];
		loc += sizeof(float);

		//
		nstages = *(int32_t*)&ptr[loc];
		loc += sizeof(int32_t);

		if(!nstages)
			return 0;

		//
		r = (int)r_f;
		c = (int)c_f;
		s = (int)s_f;

		//
		i = 0;

		while(i<nstages)
		{
			int numtrees;

			//
			numtrees = *(int32_t*)&ptr[loc];
			loc += sizeof(int32_t);

			//
			for(j=0; j<numtrees; ++j)
			{
				//
				*o += get_dtree_output(&ptr[loc], r, c, s, rotind, pixels, nrows, ncols, ldim);

				loc += get_dtree_size(&ptr[loc]);
			}

			//
			treshold = *(float*)&ptr[loc];
			loc += sizeof(float);

			//
			if(*o <= treshold)
				return -1;

			//
			++i;
		}

		//
		*o = *o - treshold;

		//
		return 1;
	}

/*
	
*/
	#ifndef MAX
		#define MAX(a, b) ((a)>(b)?(a):(b))
	#endif

	#ifndef MIN
		#define MIN(a, b) ((a)<(b)?(a):(b))
	#endif

	float get_overlap(float r1, float c1, float s1, float r2, float c2, float s2, float ratio)
	{
		float overr, overc;

		//
		overr = MAX(0, MIN(r1+s1/2, r2+s2/2) - MAX(r1-s1/2, r2-s2/2));
		overc = MAX(0, MIN(c1+ratio*s1/2, c2+ratio*s2/2) - MAX(c1-ratio*s1/2, c2-ratio*s2/2));

		//
		return overr*overc/(ratio*s1*s1+ratio*s2*s2-overr*overc);
	}

	void ccdfs(int a[], int i, float rs[], float cs[], float ss[], float ratio, int n)
	{
		int j;

		//
		for(j=0; j<n; ++j)
			if(a[j]==0 && get_overlap(rs[i], cs[i], ss[i], rs[j], cs[j], ss[j], ratio)>0.3f)
			{
				//
				a[j] = a[i];

				//
				ccdfs(a, j, rs, cs, ss, ratio, n);
			}
	}

	int find_connected_components(int a[], float rs[], float cs[], float ss[], float ratio, int n)
	{
		int i, ncc, cc;

		//
		if(!n)
			return 0;

		//
		for(i=0; i<n; ++i)
			a[i] = 0;

		//
		ncc = 0;
		cc = 1;

		for(i=0; i<n; ++i)
			if(a[i] == 0)
			{
				//
				a[i] = cc;

				//
				ccdfs(a, i, rs, cs, ss, ratio, n);

				//
				++ncc;
				++cc;
			}

		//
		return ncc;
	}

	int cluster_detections(float rs[], float cs[], float ss[], float qs[], float ratio, int n, float qcutoff)
	{
		int idx, ncc, cc;
		int a[4096];

		//
		ncc = find_connected_components(a, rs, cs, ss, ratio, n);

		if(!ncc)
			return 0;

		//
		idx = 0;

		for(cc=1; cc<=ncc; ++cc)
		{
			int i, k;

			float sumqs=0.0f, sumrs=0.0f, sumcs=0.0f, sumss=0.0f;

			//
			k = 0;

			for(i=0; i<n; ++i)
				if(a[i] == cc)
				{
					sumqs += qs[i];
					sumrs += rs[i];
					sumcs += cs[i];
					sumss += ss[i];

					++k;
				}

			if(sumqs >= qcutoff)
			{
				//
				qs[idx] = sumqs; // accumulated confidence measure

				//
				rs[idx] = sumrs/k;
				cs[idx] = sumcs/k;
				ss[idx] = sumss/k;

				//
				++idx;
			}
		}

		//
		return idx;
	}

	int find_rotated_objects(float rs[], float cs[], float ss[], float qs[], int maxndetections,
						void* od,
						unsigned char pixels[], int nrows, int ncols, int ldim,
						float scalefactor, float stridefactor, float smin, float smax, float qcutoff,
						int clusterdetections)
	{
		float s, ratio;
		int ndetections;

		//
		ratio = *(float*)od;

		ndetections = 0;

		s = smin;

		while(s<=smax)
		{
			float r, c, dr, dc;
		
			//
			dr = dc = MAX(stridefactor*s, 1.0f);

			//
			for(r=0.707107f*s+1; r<=nrows-0.707107f*s-1; r+=dr)
				for(c=0.707107f*s+1; c<=ncols-0.707107f*s-1; c+=dc)
				{
					float q;
					int t;

					for(t=0; t<NROTS; ++t)
						if(odet_classify_region(od, &q, r, c, s, t, pixels, nrows, ncols, ldim)>0)
						{
							if(ndetections < maxndetections)
							{
								qs[ndetections] = q;
								rs[ndetections] = r;
								cs[ndetections] = c;
								ss[ndetections] = s;

								//
								++ndetections;
							}
						}
				}

			//
			s = scalefactor*s;
		}

		//
		if(clusterdetections)
			ndetections = cluster_detections(rs, cs, ss, qs, ratio, ndetections, qcutoff);

		//
		return ndetections;
	}
