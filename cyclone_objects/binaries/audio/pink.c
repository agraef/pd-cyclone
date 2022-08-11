// matt barber and porres (2018) for ELSE and here borrowed to Cyclone
// based on SuperCollider's pink UGen

#include "m_pd.h"
#include <common/api.h>
#include <signal/random.h>

#define PINK_DEF_HZ 40
#define PINK_MAX_OCT 31

static t_class *pink_class;

typedef struct _pink{
    t_object        x_obj;
    float           x_signals[PINK_MAX_OCT];
    float           x_total;
    int             x_octaves;
    int             x_octaves_set; // flag for if octaves to be set at dsp time
    t_random_state  x_rstate;
}t_pink;

static unsigned int instanc_n = 0;

static void pink_init(t_pink *x){
    int octaves = x->x_octaves;
    float *signals = x->x_signals;
    float total = 0;
    t_random_state *rstate = &x->x_rstate;
    uint32_t *s1 = &rstate->s1;
    uint32_t *s2 = &rstate->s2;
    uint32_t *s3 = &rstate->s3;
    for(int i = 0; i < octaves; ++i){
        float newrand = (random_frand(s1, s2, s3));
        total += newrand;
        signals[i] = newrand;
    }
    x->x_total = total;
}

static void pink_seed(t_pink *x, t_symbol *s, int ac, t_atom *av){
    random_init(&x->x_rstate, get_seed(s, ac, av, ++instanc_n));
    pink_init(x);
}

static void pink_oct(t_pink *x, t_floatarg f){
    int octaves = (int)f;
    octaves = octaves < 0 ? 0 : octaves;
    octaves = octaves > PINK_MAX_OCT ? PINK_MAX_OCT : octaves;
    x->x_octaves = octaves;
    x->x_octaves_set = 0;
    pink_init(x);
}

static void pink_updatesr(t_pink *x, t_float sr){
    int octaves = 0;
    while(sr > PINK_DEF_HZ){
        sr *= 0.5;
        octaves++;
    }
    x->x_octaves = octaves;
    pink_init(x);
}

static t_int *pink_perform(t_int *w){
    t_pink *x = (t_pink *)(w[1]);
    int n = (t_int)(w[2]);
    t_random_state *rstate = (t_random_state *)(w[3]);
    float *signals = (float*)(w[4]);
    t_sample *out = (t_sample *)(w[5]);
    int octaves = x->x_octaves;
    float total = x->x_total;
    uint32_t *s1 = &rstate->s1;
    uint32_t *s2 = &rstate->s2;
    uint32_t *s3 = &rstate->s3;
    while(n--){
        uint32_t rcounter = random_trand(s1, s2, s3);
        float newrand = random_frand(s1, s2, s3);
        int k = (CLZ(rcounter));
        if(k < octaves){
            float prevrand = signals[k];
            signals[k] = newrand;
            total += (newrand - prevrand);
        }
        newrand = (random_frand(s1, s2, s3));
        *out++ = (t_float)(total+newrand)/(octaves+1);
    }
    x->x_total = total;
    return(w+6);
}

static void pink_dsp(t_pink *x, t_signal **sp){
    if(x->x_octaves_set)
        pink_updatesr(x, sp[0]->s_sr);
    dsp_add(pink_perform, 5, x, sp[0]->s_n, &x->x_rstate, &x->x_signals, sp[0]->s_vec);
}

static void *pink_new(t_symbol *s, int ac, t_atom *av){
    t_pink *x = (t_pink *)pd_new(pink_class);
    outlet_new(&x->x_obj, &s_signal);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("oct"));
    pink_seed(x, s, ac, av);
    if(ac > 1 && av[1].a_type == A_FLOAT)
        pink_oct(x, atom_getfloatarg(1, ac, av));
    else
        x->x_octaves_set = 1;
    return(x);
}

CYCLONE_OBJ_API void pink_tilde_setup(void){
    pink_class = class_new(gensym("pink~"), (t_newmethod)pink_new,
        0, sizeof(t_pink), 0, A_GIMME, 0);
    class_addmethod(pink_class, (t_method)pink_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(pink_class, (t_method)pink_seed, gensym("seed"), A_GIMME, 0);
    class_addmethod(pink_class, (t_method)pink_oct, gensym("oct"), A_FLOAT, 0);
}
