#include "inlay.h"

#include <cstdio>
#include <cstdlib>

#include <pthread.h>

#define EXPORT_OVERLAP

class render *base, *plug;

void *base_thread(void *arg)
{
    base = new render((const char *)arg);
    base->load();
    base->save_as_pgm("base.pgm");
    
    return NULL;
}

void *plug_thread(void *arg)
{
    plug =new render((const char *)arg);
    plug->load();
    plug->crop();
    plug->save_as_pgm("plug2.pgm");
    plug->cut_out();
    plug->flip_over();
    plug->save_as_pgm("plug.pgm");


    return NULL;
}


int main(int argc, char **argv)
{
    
    double offset, gap;
    
    if (argc < 2) {
        printf("Need 2 files as argument\n");
        exit(0);
    }

#ifdef USE_THREADS
    pthread_t base_t, plug_t;

    pthread_create(&base_t, NULL, base_thread, argv[1]);
    pthread_create(&plug_t, NULL, plug_thread, argv[2]);
        

    pthread_join(base_t, NULL);        
    pthread_join(plug_t, NULL);        
#else

    base_thread(argv[1]);
    plug_thread(argv[2]);
#endif
    offset = find_best_correlation(base, plug);    
    
    gap = save_as_xpm("result.xpm", base, plug, offset);
    save_as_stl("result.stl", base, plug, offset, true, true, 1.0/base->pixels_per_mm);
    save_as_stl("base.stl", base, plug, offset, true, false, 1.0/base->pixels_per_mm);
    save_as_stl("plug.stl", base, plug, offset, false, true, 1.0/base->pixels_per_mm);

    if (gap > 0.1) {    
        offset += gap;
#ifdef EXPORT_OVERLAP
        find_least_overlap(base, plug, offset); /* this is not fast */
        save_overlap_as_stl("overlap.stl", base, plug, offset, 1.0/base->pixels_per_mm);
        gap = save_as_xpm("result2.xpm", base, plug, offset);
#endif
    }

    plug->flip_over();
    
#if 1
    save_as_stl("plug_actual.stl", base, plug, offset, false, true, 1.0/base->pixels_per_mm);
    plug->swap_best();
    save_as_stl("plug_best.stl", base, plug, offset, false, true, 1.0/base->pixels_per_mm);
    plug->swap_best();
#endif
//    plug->make_validmap(-(offset + gap));    
    plug->make_validmap(-4);    
    plug->export_validmap("valid.pgm");
}
