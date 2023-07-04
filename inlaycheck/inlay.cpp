#include "inlay.h"


int main(int argc, char **argv)
{
    class render *ren1;
    if (argc >= 1) {
        
        ren1 = new render(argv[1]);
        
        ren1->load();
        
        ren1->crop();
        ren1->cut_out();
        
        ren1->save_as_pgm("output.pgm");
    }
    
}
