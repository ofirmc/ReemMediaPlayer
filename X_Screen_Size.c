#include <X11/Xlib.h>

int getScreenSize(int *w, int*h)
{
    Display* pdsp = NULL;
    Screen* pscr = NULL;

    pdsp = XOpenDisplay( NULL );
    
    if ( !pdsp )
    {
        g_print("Failed to open default display.\n");
        return -1;
    }

    pscr = DefaultScreenOfDisplay( pdsp );
    
    if ( !pscr )
    {
        g_print("Failed to obtain the default screen of given display.\n");
        return -2;
    }

    *w = pscr->width;
    *h = pscr->height;

    XCloseDisplay( pdsp );
    return 0;
}
