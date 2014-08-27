#ifndef CUSTOMDATAHEADER_H_INCLUDED
#define CUSTOMDATAHEADER_H_INCLUDED
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
#include <pango/pangocairo.h>

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#elif defined (GDK_WINDOWING_WIN32)
#include <gdk/gdkwin32.h>
#elif defined (GDK_WINDOWING_QUARTZ)
#include <gdk/gdkquartz.h>
#endif

#include "hashmap.h"

/* Structure to contain all our information, so we can pass it around */
typedef struct _CustomData
{
    GstElement *playbin2, *videosink;           /* Our one and only pipeline */
    GdkWindow *window;
    GtkWidget *main_window;  /* The uppermost window, containing all other windows */
    GtkWidget *video_window; /* The drawing area where the video will be shown */
    GstBus *bus;
    GtkWidget *slider;              /* Slider widget to keep track of current position */
    GtkWidget *streams_list;        /* Text widget to display info about the streams */
    GtkWidget *lblCurrentTime;  /* label to show current movie time */
    gulong slider_update_signal_id; /* Signal ID for the slider update signal */
    GtkWindow *parent;
    GstState state;                 /* Current state of the pipeline */
    gint64 duration;                /* Duration of the clip, in nanoseconds */
    char *uri;
    GtkWidget *play_button, *pause_button, *stop_button, *btnOpen, *btnSubtitles;/*Buttons*/
    GtkWidget *lblBtnSubtitles, *lblSubs, *menubar;
    char timeBuffer[20], *SubtLineFromHash;
    char *curTime, Hbuf[BUFSIZ];
    int IsSubtitlesLoaded, SubsScreenHight, SubsScreenWidth, rootScrHight, rootScrWidth;
    HashMap *hm;
    GtkWidget *subs_window; //A new trasparent subtitles windows
    cairo_t *cr, *cr_window;
    PangoLayout *layout;
    gboolean IsPaused, IsFullScreen, IsMouseCurMove, supports_alpha;
} CustomData;
#endif // CUSTOMDATAHEADER_H_INCLUDED
