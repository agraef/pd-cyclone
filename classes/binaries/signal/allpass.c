/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

// should not allow blowup for float input in inlets...

#include <string.h>
#include "m_pd.h"

typedef struct _allpass
{
    t_object  x_obj;
    t_inlet  *x_del_inlet;
    t_inlet  *x_gain_inlet;
    float     x_sr;
    float     x_ksr;
    t_float  *x_buf;
    int       x_bufsize;   /* as allocated */
    int       x_maxsize;   /* as used */
    float     x_maxdelay;  /* same in ms */
    int       x_phase;     /* writing head */
} t_allpass;

static t_class *allpass_class;

#define ALLPASS_DEFMAXDELAY  10.0
#define ALLPASS_MAXFEEDBACK  1

static void allpass_clear(t_allpass *x)
{
    memset(x->x_buf, 0, x->x_maxsize * sizeof(*x->x_buf));
    x->x_phase = 0;
}

static void allpass_resize(t_allpass *x, int newsize)
{
    if (newsize > 0 && newsize != x->x_maxsize)
    {
	if (newsize > x->x_bufsize)
	{
	    x->x_buf = resizebytes(x->x_buf,
				   x->x_bufsize * sizeof(*x->x_buf),
				   newsize * sizeof(*x->x_buf));
	    /* LATER test for failure */
	    x->x_bufsize = newsize;
	}
	x->x_maxsize = newsize;
    }
    allpass_clear(x);
}

static t_int *allpass_perform(t_int *w)
{
    t_allpass *x = (t_allpass *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *xin = (t_float *)(w[3]);
    t_float *din = (t_float *)(w[4]);
    t_float *gin = (t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
    t_float *buf = x->x_buf;
    int maxsize = x->x_maxsize;
    int guardpoint = maxsize - 1;
    float ksr = x->x_ksr;
    int wph = x->x_phase;
    while (nblock--)
    {  /* TDFII scheme */
	float xn = *xin++;
	float delsize = ksr * *din++;
	float gain = *gin++;
	float yn;
	float rph;  /* reading head */
	yn = xn;
	if (delsize > 0)
	{
        yn = -gain * xn;
	    int ndx;
	    float val;
	    rph = wph - (delsize > guardpoint ? guardpoint : delsize);
	    if (rph < 0) rph += guardpoint;
	    ndx = (int)rph;
	    val = buf[ndx];
	    /* linear interpolation */
	    yn += val + (buf[ndx+1] - val) * (rph - ndx);
	}
	*out++ = yn;
	if (wph == guardpoint)
	{
	    buf[wph] = *buf = xn + gain * yn;
	    wph = 1;
	}
	else buf[wph++] = xn + gain * yn;
    }
    x->x_phase = wph;
    return (w + 7);
}

static void allpass_dsp(t_allpass *x, t_signal **sp)
{
    float sr = sp[0]->s_sr;
    if (sr != x->x_sr)
    {
	x->x_sr = sr;
	x->x_ksr = sr * 0.001;
	allpass_resize(x, x->x_ksr * x->x_maxdelay);
    }
    else allpass_clear(x);
    dsp_add(allpass_perform, 6, x, sp[0]->s_n,
	    sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *allpass_new(t_floatarg f1, t_floatarg f2, t_floatarg f3)
{
    t_allpass *x;
    float maxdelay = (f1 > 0 ? f1 : ALLPASS_DEFMAXDELAY);
    float sr = sys_getsr();
    float ksr = sr  * 0.001;
    int bufsize = ksr * maxdelay;
    t_float *buf = (t_float *)getbytes(bufsize * sizeof(*buf));
    if (!buf)
	return (0);
    x = (t_allpass *)pd_new(allpass_class);
    x->x_maxdelay = maxdelay;
    x->x_sr = sr;
    x->x_ksr = ksr;
    x->x_bufsize = x->x_maxsize = bufsize;
    x->x_buf = buf;
    if (f2 < 0) f2 = 0;
    
    x->x_del_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_del_inlet, f2);
    
    x->x_gain_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_gain_inlet, f3);
    
    outlet_new((t_object *)x, &s_signal);
    allpass_clear(x);
    return (x);
}

static void allpass_free(t_allpass *x)
{
    if (x->x_buf) freebytes(x->x_buf, x->x_bufsize * sizeof(*x->x_buf));
}

void allpass_tilde_setup(void)
{
    allpass_class = class_new(gensym("allpass~"),
			      (t_newmethod)allpass_new,
			      (t_method)allpass_free,
			      sizeof(t_allpass), 0,
			      A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(allpass_class, nullfn, gensym("signal"), 0);
    class_addmethod(allpass_class, (t_method)allpass_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(allpass_class, (t_method)allpass_clear, gensym("clear"), 0);
}
