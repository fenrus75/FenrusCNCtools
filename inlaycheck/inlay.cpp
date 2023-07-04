#include "inlay.h"

#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv)
{
    class render *base, *plug;
    
    double offset;
    
    if (argc < 2) {
        printf("Need 2 files as argument\n");
        exit(0);
    }
        
    base = new render(argv[1]);
    base->load();
    base->save_as_pgm("base.pgm");
        
    plug =new render(argv[2]);
    plug->load();
    plug->crop();
    plug->cut_out();
    plug->flip_over();
    plug->save_as_pgm("plug.pgm");


    offset = find_best_correlation(base, plug);    
    
    save_as_xpm("result.xpm", base, plug, offset);
}
