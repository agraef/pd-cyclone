/* Copyright (c) 1997-2005 Miller Puckette, krzYszcz, and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* Put here bits and pieces likely to break with any new Pd version. */

#include <string.h>
#include "m_pd.h"
#include "unstable/magic.h"
#include "unstable/pd_imp.h"

/* this one rather belongs to fringe.c... */
t_symbol *magic_class_getexterndir(t_class *c)
{
    return (c->c_externdir);
}

int magic_class_count(void)
{
    return (pd_objectmaker->c_nmethod);
}

int magic_class_getnames(t_atom *av, int maxnames)
{
    int ac = pd_objectmaker->c_nmethod;
    t_methodentry *mp = pd_objectmaker->c_methods;
    if (ac > maxnames)
	ac = maxnames;
    else
	maxnames = ac;
    while (ac--)
    {
	SETSYMBOL(av, mp->me_name);
	mp++; av++;
    }
    return (maxnames);
}


t_newmethod magic_class_getalien(t_symbol *cname, t_newmethod thiscall,
				   t_atomtype **argtypesp)
{
    t_methodentry *mp = pd_objectmaker->c_methods;
    int count = pd_objectmaker->c_nmethod;
    while (count--)
    {
	if (mp->me_name == cname && mp->me_fun != (t_gotfn)thiscall)
	{
	    *argtypesp = mp->me_arg;
	    return ((t_newmethod)mp->me_fun);
	}
	mp++;
    }
    return (0);
}

/* A specialized copy of pd_typedmess() from m_class.c,
   somewhat simplified for readability. */

typedef t_pd *(*t_newgimme)(t_symbol *s, int ac, t_atom *av);
typedef t_pd *(*t_new0)(
    t_floatarg, t_floatarg, t_floatarg, t_floatarg, t_floatarg);
typedef t_pd *(*t_new1)(
    t_symbol*,
    t_floatarg, t_floatarg, t_floatarg, t_floatarg, t_floatarg);
typedef t_pd *(*t_new2)(
    t_symbol*, t_symbol*,
    t_floatarg, t_floatarg, t_floatarg, t_floatarg, t_floatarg);
typedef t_pd *(*t_new3)(
    t_symbol*, t_symbol*, t_symbol*,
    t_floatarg, t_floatarg, t_floatarg, t_floatarg, t_floatarg);
typedef t_pd *(*t_new4)(
    t_symbol*, t_symbol*, t_symbol*, t_symbol*,
    t_floatarg, t_floatarg, t_floatarg, t_floatarg, t_floatarg);
typedef t_pd *(*t_new5)(
    t_symbol*, t_symbol*, t_symbol*, t_symbol*, t_symbol*,
    t_floatarg, t_floatarg, t_floatarg, t_floatarg, t_floatarg);
typedef t_pd *(*t_new6)(
    t_symbol*, t_symbol*, t_symbol*, t_symbol*, t_symbol*, t_symbol*,
    t_floatarg, t_floatarg, t_floatarg, t_floatarg, t_floatarg);


void magic_class_printnames(char *msg, int firstndx, int lastndx)
{
    t_methodentry *mp = pd_objectmaker->c_methods;
    int ndx, len = strlen(msg);
    startpost(msg);
    for (ndx = firstndx, mp += ndx; ndx <= lastndx; ndx++, mp++)
    {
	t_symbol *s = mp->me_name;
	if (s && s->s_name[0] != '_')
	{
	    int l = 1 + strlen(s->s_name);
	    if ((len += l) > 66)
	    {
		endpost();
		startpost("   ");
		len = 3 + l;
	    }
	    poststring(s->s_name);
	}
    }
    endpost();
}

/* This structure is local to g_array.c.  We need it,
   because there is no other way to get into array's graph. */
struct _garray
{
    t_gobj x_gobj;
    t_glist *x_glist;
    /* ... */
};

t_glist *magic_garray_glist(void *arr)
{
    return (((struct _garray *)arr)->x_glist);
}

/* This is local to m_obj.c.
   LATER export write access to o_connections field ('grab' class).
   LATER encapsulate 'traverseoutlet' routines (not in the stable API yet). */
struct _outlet
{
    t_object *o_owner;
    struct _outlet *o_next;
    t_outconnect *o_connections;
    t_symbol *o_sym;
};

/* obj_starttraverseoutlet() replacement */
t_outconnect *magic_outlet_connections(t_outlet *o)
{
    return (o ? o->o_connections : 0);
}

t_outconnect *magic_outlet_nextconnection(t_outconnect *last,
					    t_object **destp, int *innop)
{
    t_inlet *dummy;
    return (obj_nexttraverseoutlet(last, destp, &dummy, innop));
}

/* These are local to m_obj.c. */
union inletunion
{
    t_symbol *iu_symto;
    t_gpointer *iu_pointerslot;
    t_float *iu_floatslot;
    t_symbol **iu_symslot;
    t_sample iu_floatsignalvalue;
};

struct _inlet
{
    t_pd i_pd;
    struct _inlet *i_next;
    t_object *i_owner;
    t_pd *i_dest;
    t_symbol *i_symfrom;
    union inletunion i_un;
};

/* simplified obj_findsignalscalar(), works for non-left inlets */
t_sample *magic_inlet_signalscalar(t_inlet *i)
{
    return (&i->i_un.iu_floatsignalvalue);
}