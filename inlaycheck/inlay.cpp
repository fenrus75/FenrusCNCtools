#include "inlay.h"

#include <cstdio>
#include <cstdlib>

#include <pthread.h>


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
    pthread_t base_t, plug_t;
    
    double offset;
    
    if (argc < 2) {
        printf("Need 2 files as argument\n");
        exit(0);
    }

/*    
    pthread_create(&base_t, NULL, base_thread, argv[1]);
    pthread_create(&plug_t, NULL, plug_thread, argv[2]);
        

    pthread_join(base_t, NULL);        
    pthread_join(plug_t, NULL);        
*/
    base_thread(argv[1]);
    plug_thread(argv[2]);

    offset = find_best_correlation(base, plug);    
    
    save_as_xpm("result.xpm", base, plug, offset);
    save_as_stl("result.stl", base, plug, offset);
    save_as_stl("base.stl", base, plug, offset, true, false);
    save_as_stl("plug.stl", base, plug, offset, false, true);
}
