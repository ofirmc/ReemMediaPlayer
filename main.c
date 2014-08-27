#include <string.h>
#include <stdlib.h>

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

#include "Subtitles_Header.h"
#include "copyFileFunc.h"
#include "CustomDataHeader.h"
#include "TimeSubsDisp.h"
#include "X_Screen_Size.h"
#define GST_MSECOND (GST_SECOND / G_GINT64_CONSTANT (1000))

char* ExtractCurrentTime(char* timeBuffer);
static gboolean expose_subtitles_window (GtkWidget *widget, GdkEventExpose *event, CustomData *data);
static void screen_changed(GtkWidget *widget, GdkScreen *old_screen, CustomData *data);
GdkPixbuf *create_pixbuf(const gchar * filename);
void ToggleFullscreen(GtkWidget* widget, CustomData *data);
void UnToggleFullscreen(GtkWidget* widget, CustomData *data);
static GstBusSyncReply bus_sync_handler (GstBus * bus, GstMessage * message, gpointer user_data);

static gulong video_window_xid = 0;

/* This function is called when the GUI toolkit creates the physical window that will hold the video.
 * At this point we can retrieve its handler (which has a different meaning depending on the windowing system)
 * and pass it to GStreamer through the XOverlay interface. */
static void realize_cb (GtkWidget *widget, CustomData *data)
{
    #if GTK_CHECK_VERSION(2,18,0)
  // This is here just for pedagogical purposes, GDK_WINDOW_XID will call
  // it as well in newer Gtk versions
  if (!gdk_window_ensure_native (widget->window))
  {
    g_error ("Couldn't create native window needed for GstXOverlay!");
  }
#endif

#ifdef GDK_WINDOWING_X11
  video_window_xid = GDK_WINDOW_XID (gtk_widget_get_window (data->video_window));
#endif
}


void ToggleFullscreen(GtkWidget* widget, CustomData *data)
{
    data->window = gtk_widget_get_window (widget);
    GdkCursor *_cursor = NULL;
    GdkCursorType cursor_type = GDK_BLANK_CURSOR; //Set cursor variables

    gtk_window_fullscreen(GTK_WINDOW(widget));
    gtk_widget_hide(data->play_button);
    gtk_widget_hide(data->pause_button);
    gtk_widget_hide(data->stop_button);
    gtk_widget_hide(data->btnOpen);
    gtk_widget_hide(data->btnSubtitles);
    gtk_widget_hide(data->lblCurrentTime);
    gtk_widget_hide(data->lblSubs);
    gtk_widget_hide(data->slider);    
    
    gtk_window_resize(GTK_WINDOW (data->subs_window), data->rootScrWidth, 140);
    
    gtk_widget_set_uposition(data->subs_window, 0, data->rootScrHight - 140);
    gtk_window_get_size(GTK_WINDOW(data->subs_window), &data->SubsScreenWidth, &data->SubsScreenHight);
    data->IsFullScreen = TRUE;       

    //Hide cursor
    _cursor = gdk_cursor_new (cursor_type);
    gdk_window_set_cursor (GDK_WINDOW(data->window), _cursor);
    gdk_flush ();
}

void UnToggleFullscreen(GtkWidget* widget, CustomData *data)
{
    data->window = gtk_widget_get_window (widget);

    gtk_window_unfullscreen(GTK_WINDOW(widget));
    gtk_widget_show(data->play_button);
    gtk_widget_show(data->pause_button);
    gtk_widget_show(data->stop_button);
    gtk_widget_show(data->btnOpen);
    gtk_widget_show(data->btnSubtitles);
    gtk_widget_show(data->lblCurrentTime);
    gtk_widget_show(data->lblSubs);
    gtk_widget_show(data->slider);    
    data->IsFullScreen = FALSE;
    gtk_window_resize(GTK_WINDOW (data->subs_window), 768, 52);
    gtk_window_get_size(GTK_WINDOW(data->subs_window), &data->SubsScreenWidth, &data->SubsScreenHight);
    
    //Show cursor
    gdk_window_set_cursor (GDK_WINDOW(data->window), NULL);
    gdk_flush ();
}

static gboolean on_key_press (GtkWidget* widget, GdkEventKey *event, CustomData *data)
{

    if(event->keyval == GDK_Escape)//Exit fullscreen mode
    {
        UnToggleFullscreen(widget, data);
    }
    
    if(event->keyval == GDK_F1)//Enter fullscreen mode
    {
        ToggleFullscreen(widget, data);
    }

    //jump 10 second forward(not working very well yet)
    if(event->keyval == GDK_Right)
    {
        gdouble value = gtk_range_get_value (GTK_RANGE (data->slider));
        gst_element_seek_simple (data->playbin2, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
                                 (gint64)((value + 10) * GST_SECOND));
    }
    
    //jump 10 second backward
    if(event->keyval == GDK_Left)
    {
        gdouble value = gtk_range_get_value (GTK_RANGE (data->slider));
        gst_element_seek_simple (data->playbin2, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
                                 (gint64)((value - 10) * GST_SECOND));
    }
    
    if(event->keyval == GDK_space)
    {
        if(!data->IsPaused)
        {
            gst_element_set_state (data->playbin2, GST_STATE_PAUSED);
            data->IsPaused = TRUE;
        }
        else if(data->IsPaused)
        {
            gst_element_set_state (data->playbin2, GST_STATE_PLAYING);
            data->IsPaused = FALSE;
        }
    }
    
    if(event->keyval == GDK_Up)
    {
        data->SubsScreenHight -= 1;
    }
    
    if(event->keyval == GDK_Down)
    {
        data->SubsScreenHight += 1;
    }
}

//This method is neccesssary for GTK_WINDOW_POPUP - for hiding the subtitles window 
gint on_main_window_focus_out (GtkWidget *widget, GdkEventButton *event, CustomData *data)
{
	if(data->IsSubtitlesLoaded == 1)
	{
		g_print ("on_main_window_focus_out.\n");
    	gtk_widget_hide(data->subs_window);    	
    }
    return FALSE;
}

//This method is neccesssary for GTK_WINDOW_POPUP - for showing the subtitles window
gint on_main_window_focus_in (GtkWidget *widget, GdkEventButton *event, CustomData *data)
{
	if(data->IsSubtitlesLoaded == 1)
	{
		g_print ("on_main_window_focus_in.\n");
    	gtk_widget_show(data->subs_window);    	
    }
    return FALSE;
}

gint mouseBtnOnClick (GtkWidget *widget, GdkEventButton *event, CustomData *data)
{
    if ((event->type==GDK_2BUTTON_PRESS ||
            event->type==GDK_3BUTTON_PRESS) )
    {
        if(data->IsFullScreen)
        {
            UnToggleFullscreen(widget, data);
            data->IsFullScreen = FALSE;
        }
        else
        {
            ToggleFullscreen(widget, data);
            data->IsFullScreen = TRUE;
        }
    }

    return FALSE;
}

gint mouse_move(GtkWidget *widget, GdkEventButton *event, CustomData *data)
{
    if(data->IsFullScreen)
    {
        //Show cursor
        gdk_window_set_cursor (GDK_WINDOW(data->window), NULL);
        gdk_flush ();
        data->IsMouseCurMove = TRUE;
    }

    return FALSE;
}

static void open_subtitles_cb( GtkButton *button, CustomData *data ,GtkWidget *widget)
{
    static GtkWidget *dialog = NULL;
    GtkWidget *subs_hbox;
    
    //setting a string for the iconv library command, to convert the encoding of characters
    char args[] = "iconv -f cp1255 -t utf-8 /tmp/tmp1.srt -o /tmp/tmp2.srt";
    
    GError **error = NULL;
    char          *output;
    GtkWidget *msgDialog;
    int x, y;

    GdkWindow *window = gtk_widget_get_window (data->main_window);
    gdk_window_get_position (window, &x, &y);

    if( ! dialog )
    {
        dialog = gtk_file_chooser_dialog_new( "Open file", data->parent,
                                              GTK_FILE_CHOOSER_ACTION_OPEN,
                                              GTK_STOCK_OPEN,
                                              GTK_RESPONSE_ACCEPT,
                                              GTK_STOCK_CANCEL,
                                              GTK_RESPONSE_REJECT,
                                              NULL );
    }

    if( gtk_dialog_run( GTK_DIALOG( dialog ) ) == GTK_RESPONSE_ACCEPT )
    {
        char *filename;
        char fileLocalLocation[] = "file://";
        int len1 = strlen(fileLocalLocation);
        int fileNameLen;

        filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ) );
        fileNameLen = strlen(filename);

        char *strRight;
        strRight = substring(filename, (fileNameLen-3), 3);
        if(strcmp(strRight, "srt") != 0)
        {
            msgDialog = gtk_message_dialog_new(data->parent, GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                               "Error: only .srt files can be loaded");
                                               
            gtk_dialog_run (GTK_DIALOG (msgDialog));
            gtk_widget_destroy (msgDialog);
            gtk_widget_hide( dialog );
            return;
        }
        
        //Copy srt file to /tmp dir inorder to exec iconv
        copyFile("/tmp/tmp1.srt", filename);
        g_print("\nfile:/tmp/tmp1.srt created\n");

        g_spawn_command_line_sync(args,&output, NULL, NULL, error);
        g_print("\nfile:/tmp/tmp2.srt created by iconv\n");

        char *cont;
        g_file_get_contents("/tmp/tmp2.srt", &cont, NULL, NULL);
        int siz = strlen(cont);
        g_print("\nnumber of chars is: %d", siz);
        remove("/tmp/tmp1.srt"); //Remove /tmp/tmp1.srt since we don't need it anymore
        g_print("\nfile:/tmp/tmp1.srt removed\n");

        data->hm = InsertSubsToHashTable("/tmp/tmp2.srt", siz);

        data->IsSubtitlesLoaded = 1;
        
        gtk_window_set_default_size (GTK_WINDOW (data->subs_window), 768, 52);        
        gtk_widget_set_uposition(data->subs_window, x, y + 400);
        gtk_widget_set_app_paintable(data->subs_window, TRUE);
        g_signal_connect (data->subs_window, "expose_event", G_CALLBACK (expose_subtitles_window), data);
        g_signal_connect(G_OBJECT(data->subs_window), "screen-changed", G_CALLBACK(screen_changed), data);
        screen_changed(data->subs_window, NULL, data);
        subs_hbox = gtk_hbox_new (FALSE, 0);

        gtk_container_add (GTK_CONTAINER (data->subs_window), subs_hbox);
        gtk_widget_show_all(data->subs_window);

    }
    gtk_widget_hide( dialog );

    //Get X11 window width and hight
    gtk_window_get_size(GTK_WINDOW(data->subs_window), &data->SubsScreenWidth, &data->SubsScreenHight);
}

static int open_cb( GtkButton *button, CustomData *data )
{
    static GtkWidget *dialog = NULL;
    GstStateChangeReturn ret;
    char *tempStr;    

    data->IsPaused = FALSE;

    if( ! dialog )
    {
        dialog = gtk_file_chooser_dialog_new( "Open file", data->parent,
                                              GTK_FILE_CHOOSER_ACTION_OPEN,
                                              GTK_STOCK_OPEN,
                                              GTK_RESPONSE_ACCEPT,
                                              GTK_STOCK_CANCEL,
                                              GTK_RESPONSE_REJECT,
                                              NULL );
    }

    if( gtk_dialog_run( GTK_DIALOG( dialog ) ) == GTK_RESPONSE_ACCEPT )
    {
        char *filename;
        char fileLocalLocation[] = "file://";
        int len1 = strlen(fileLocalLocation);
        int len2;

        filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ) );
        len2 = strlen(filename);

        tempStr = (char*)malloc((len1 + len2 + 1) * sizeof(char));
        
        if(!tempStr) //Check if memory allocated.
        {
        	return 0;
        }

        strcpy(tempStr, fileLocalLocation);
        strcpy(tempStr + len1, filename);

        g_print("\n%s", tempStr);

        gst_element_set_state (data->playbin2, GST_STATE_NULL);//Clear playbin2
        data->videosink = gst_element_factory_make ("xvimagesink", "videosink");
        g_object_set (G_OBJECT (data->videosink), "force-aspect-ratio", TRUE, NULL);
        g_object_set (data->playbin2, "uri",   tempStr, NULL);//Set playbin2
        g_object_set (data->playbin2, "video-sink", data->videosink, NULL);

        ret = gst_element_set_state (data->playbin2, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE)
        {
            g_printerr ("Unable to set the pipeline to the playing state.\n");
            gst_object_unref (data->playbin2);
            return -1;
        }
    }
    gtk_widget_hide( dialog );
    g_print("\n\nFullscreen: F1\n");
    g_print("Normal screen: Esc\n\n");
    free(tempStr);
}

/* This function is called when the PLAY button is clicked */
static void play_cb (GtkButton *button, CustomData *data)
{
    gst_element_set_state (data->playbin2, GST_STATE_PLAYING);
}

/* This function is called when the PAUSE button is clicked */
static void pause_cb (GtkButton *button, CustomData *data)
{
    gst_element_set_state (data->playbin2, GST_STATE_PAUSED);
}

/* This function is called when the STOP button is clicked */
static void stop_cb (GtkButton *button, CustomData *data)
{
    gst_element_set_state (data->playbin2, GST_STATE_READY);
}

/* This function is called when the main window is closed */
static void delete_event_cb (GtkWidget *widget, GdkEvent *event, CustomData *data)
{
    stop_cb (NULL, data);
    remove("/tmp/tmp2.srt"); //Delete the .srt file to let iconv create a file in the same name next time
    gtk_main_quit ();
}

void main_window_closing()
{
    remove("/tmp/tmp2.srt"); //Delete the .srt file to let iconv create a file in the same name next time    
    gtk_main_quit ();
}

/* This function is called everytime the video window needs to be redrawn (due to damage/exposure,
 * rescaling, etc). GStreamer takes care of this in the PAUSED and PLAYING states, otherwise,
 * we simply draw a black rectangle to avoid garbage showing up. */
static gboolean expose_cb (GtkWidget *widget, GdkEventExpose *event, CustomData *data)
{
    if (data->state < GST_STATE_PAUSED)
    {
        GtkAllocation allocation;
        GdkWindow *window = gtk_widget_get_window (widget);
        cairo_t *cr;

        /* Cairo is a 2D graphics library which we use here to clean the video window.
         * It is used by GStreamer for other reasons, so it will always be available to us. */
        gtk_widget_get_allocation (widget, &allocation);
        cr = gdk_cairo_create (window);
        cairo_set_source_rgb (cr, 0, 0, 0);
        cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
        cairo_fill (cr);
        cairo_destroy (cr);
    }
    return FALSE;
}

static void screen_changed(GtkWidget *widget, GdkScreen *old_screen, CustomData *data)
{
    /* To check if the display supports alpha channels, get the colormap */
    GdkScreen *screen = gtk_widget_get_screen(widget);
    GdkColormap *colormap = gdk_screen_get_rgba_colormap(screen);

    if (!colormap)
    {
        g_print("Your screen does not support alpha channels!\n");
        colormap = gdk_screen_get_rgb_colormap(screen);
        data->supports_alpha = FALSE;
    }
    else
    {
        g_print("Your screen supports alpha channels!\n");
        data->supports_alpha = TRUE;
    }

    gtk_widget_set_colormap(widget, colormap);
}

static gboolean expose_subtitles_window (GtkWidget *widget, GdkEventExpose *event, CustomData *data)
{				
    data->cr_window = gdk_cairo_create(widget->window);

    if (data->supports_alpha)
        cairo_set_source_rgba (data->cr_window, 1.0, 1.0, 1.0, 0.0); /* transparent */
    else
        cairo_set_source_rgb (data->cr_window, 1.0, 1.0, 1.0); /* opaque white */

    /* draw the background */
    cairo_set_operator (data->cr_window, CAIRO_OPERATOR_SOURCE);
    cairo_paint (data->cr_window);

    cairo_destroy(data->cr_window);

    return FALSE;
}

/* This function is called when the slider changes its position. We perform a seek to the
 * new position here. */
static void slider_cb (GtkRange *range, CustomData *data)
{
    gdouble value = gtk_range_get_value (GTK_RANGE (data->slider));
    gst_element_seek_simple (data->playbin2, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
                             (gint64)(value * GST_SECOND));
}

/* This creates all the GTK+ widgets that compose our application, and registers the callbacks */
static void create_ui (CustomData *data)
{
    GtkWidget *main_box;     /* VBox to hold main_hbox and the controls */
    GtkWidget *main_hbox;    /* HBox to hold the video_window and the stream info text widget */
    GtkWidget *controls;     /* HBox to hold the buttons and the slider */

    data->main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (G_OBJECT (data->main_window), "delete-event", G_CALLBACK (delete_event_cb), data);

    gtk_window_set_icon(GTK_WINDOW(data->main_window), create_pixbuf("/opt/reem_media_player/reem-media-player.png"));

    data->video_window = gtk_drawing_area_new ();
    gtk_widget_set_double_buffered (data->video_window, FALSE);
    g_signal_connect (data->video_window, "realize", G_CALLBACK (realize_cb), data);
    g_signal_connect (data->video_window, "expose_event", G_CALLBACK (expose_cb), data);
    g_signal_connect (data->main_window, "key-press-event", G_CALLBACK (on_key_press), data);

    gtk_widget_add_events (data->video_window , GDK_BUTTON_PRESS_MASK );
    g_signal_connect (data->main_window, "button-press-event",
                      G_CALLBACK (mouseBtnOnClick), data ) ;

    gtk_widget_add_events (data->video_window, GDK_BUTTON_RELEASE_MASK );
    g_signal_connect (data->main_window, "button-release-event",
                      G_CALLBACK (mouseBtnOnClick), data ) ;

    gtk_widget_add_events (data->main_window, GDK_POINTER_MOTION_MASK );
    g_signal_connect (data->main_window, "motion-notify-event",
                      G_CALLBACK (mouse_move), data ) ;

    data->subs_window = gtk_window_new (GTK_WINDOW_POPUP); //Set subtitles window        
    
    gtk_widget_add_events (data->main_window, GDK_FOCUS_CHANGE_MASK );
    g_signal_connect (data->main_window, "focus-out-event",
                      G_CALLBACK (on_main_window_focus_out), data ) ;
                      
    g_signal_connect( data->main_window, "focus-in-event",
                 G_CALLBACK( on_main_window_focus_in ), data);
                 

    data->lblCurrentTime = gtk_label_new("");
    data->lblSubs = gtk_label_new("\n\n\n");

    data->btnOpen = gtk_button_new_from_stock (GTK_STOCK_OPEN);
    g_signal_connect(G_OBJECT(data->btnOpen), "clicked", G_CALLBACK(open_cb), data);

    data->play_button = gtk_button_new_from_stock (GTK_STOCK_MEDIA_PLAY);
    g_signal_connect (G_OBJECT (data->play_button), "clicked", G_CALLBACK (play_cb), data);

    data->pause_button = gtk_button_new_from_stock (GTK_STOCK_MEDIA_PAUSE);
    g_signal_connect (G_OBJECT (data->pause_button), "clicked", G_CALLBACK (pause_cb), data);

    data->stop_button = gtk_button_new_from_stock (GTK_STOCK_MEDIA_STOP);
    g_signal_connect (G_OBJECT (data->stop_button), "clicked", G_CALLBACK (stop_cb), data);

    data->lblBtnSubtitles = gtk_label_new ("Subtitles");
    data->btnSubtitles = gtk_button_new();
    g_signal_connect(G_OBJECT(data->btnSubtitles), "clicked",G_CALLBACK(open_subtitles_cb), data);

    data->slider = gtk_hscale_new_with_range (0, 100.0, 1.0);
    gtk_scale_set_draw_value (GTK_SCALE (data->slider), 0);
    data->slider_update_signal_id = g_signal_connect (G_OBJECT (data->slider), "value-changed", G_CALLBACK (slider_cb), data);

    data->IsSubtitlesLoaded = 0;    

    controls = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (controls), data->btnOpen, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (controls), data->play_button, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (controls), data->pause_button, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (controls), data->stop_button, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (controls), data->btnSubtitles, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (controls), data->slider, TRUE, TRUE, 2);

    main_hbox = gtk_hbox_new (FALSE, 0);

    gtk_box_pack_start (GTK_BOX (main_hbox), data->video_window, TRUE, TRUE, 0);   

    main_box = gtk_vbox_new (FALSE, 0);    
    gtk_box_pack_start (GTK_BOX (main_box), main_hbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (main_box), controls, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (main_box), data->lblCurrentTime, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (main_box), data->lblSubs, FALSE, FALSE, 2);
    gtk_container_add (GTK_CONTAINER (data->main_window), main_box);
    gtk_container_add (GTK_CONTAINER (data->btnSubtitles), data->lblBtnSubtitles);
    gtk_window_set_default_size (GTK_WINDOW (data->main_window), 768, 576);

    gtk_widget_show_all (data->main_window);

    // realize window now so that the video window gets created and we can
  // obtain its XID before the pipeline is started up and the videosink
  // asks for the XID of the window to render onto
  gtk_widget_realize (data->video_window);

  // we should have the XID now
  g_assert (video_window_xid != 0);

  // set up sync handler for setting the xid once the pipeline is started
  data->bus = gst_pipeline_get_bus (GST_PIPELINE (data->playbin2));
  gst_bus_set_sync_handler (data->bus, (GstBusSyncHandler) bus_sync_handler, NULL);  
}

static GstBusSyncReply
bus_sync_handler (GstBus * bus, GstMessage * message, gpointer user_data)
{
 // ignore anything but 'prepare-xwindow-id' element messages
 if (GST_MESSAGE_TYPE (message) != GST_MESSAGE_ELEMENT)
 {
   return GST_BUS_PASS;
 }
 
 if (!gst_structure_has_name (message->structure, "prepare-xwindow-id"))
 {
   return GST_BUS_PASS;
 }

 if (video_window_xid != 0) 
 {
   GstXOverlay *xoverlay;

   // GST_MESSAGE_SRC (message) will be the video sink element
   xoverlay = GST_X_OVERLAY (GST_MESSAGE_SRC (message));
   gst_x_overlay_set_window_handle (xoverlay, video_window_xid);
 } 
 else 
 {
   g_warning ("Should have obtained video_window_xid by now!");
 }

 gst_message_unref (message);
 return GST_BUS_DROP;
}

/* This function is called periodically to refresh the GUI */
static gboolean refresh_ui (CustomData *data)
{
    GstFormat fmt = GST_FORMAT_TIME;
    gint64 current = -1;
    GstQuery *query;
    gint64 position;

    /* Get location of main window for subtitles window inorder to stay inside the main window
    when not in Fullscreen mode */
    int x, y;
    GdkWindow *window = gtk_widget_get_window (data->main_window);
    gdk_window_get_position (window, &x, &y);

    /* We do not want to update anything unless we are in the PAUSED or PLAYING states */
    if (data->state < GST_STATE_PAUSED)
        return TRUE;

    /* If we didn't know it yet, query the stream duration */
    if (!GST_CLOCK_TIME_IS_VALID (data->duration))
    {
        if (!gst_element_query_duration (data->playbin2, &fmt, &data->duration))
        {
            g_printerr ("Could not query current duration.\n");
        }
        else
        {
            /* Set the range of the slider to the clip duration, in SECONDS */
            gtk_range_set_range (GTK_RANGE (data->slider), 0, (gdouble)data->duration / GST_SECOND);
        }
    }
    gst_element_query_position(data->playbin2, &fmt, &current);

    if(data->IsSubtitlesLoaded)
    {
        if(!data->IsFullScreen)
        {
            //Update subtitles window position according to the main window position
            gtk_widget_set_uposition(data->subs_window, x, y + 400);
        }
    }

    //Set current time label
    sprintf(data->timeBuffer, "%02u:%02u:%02u:%02u", GST_TIME_ARGS (current));

    gtk_label_set_text(GTK_LABEL(data->lblCurrentTime), data->timeBuffer);
    query = gst_query_new_position (GST_FORMAT_TIME);
    
    if (gst_element_query (data->playbin2, query))
    {
        gst_query_parse_position (query, NULL, &position);                                  
    }
    else
    {
        position = 0;
    }

    gst_query_unref (query);

    //Extract hours, minutes etc from palybin2 query position
    //and display subtitles with current time
    DispSubsByPosition(position, data);

    if (gst_element_query_position (data->playbin2, &fmt, &current))
    {
        /* Block the "value-changed" signal, so the slider_cb function is not called
         * (which would trigger a seek the user has not requested) */
        g_signal_handler_block (data->slider, data->slider_update_signal_id);
        
        /* Set the position of the slider to the current pipeline positoin, in SECONDS */
        gtk_range_set_value (GTK_RANGE (data->slider), (gdouble)current / GST_SECOND);

        /* Re-enable the signal */
        g_signal_handler_unblock (data->slider, data->slider_update_signal_id);
    }
    return TRUE;
}

/* This function is called when an error message is posted on the bus */
static void error_cb (GstBus *bus, GstMessage *msg, CustomData *data)
{
    GError *err;
    gchar *debug_info;

    /* Print error details on the screen */
    gst_message_parse_error (msg, &err, &debug_info);
    g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
    g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
    g_clear_error (&err);
    g_free (debug_info);

    /* Set the pipeline to READY (which stops playback) */
    gst_element_set_state (data->playbin2, GST_STATE_READY);
}

/* This function is called when an End-Of-Stream message is posted on the bus.
 * We just set the pipeline to READY (which stops playback) */
static void eos_cb (GstBus *bus, GstMessage *msg, CustomData *data)
{
    g_print ("End-Of-Stream reached.\n");
    gst_element_set_state (data->playbin2, GST_STATE_READY);
}

/* This function is called when the pipeline changes states. We use it to
 * keep track of the current state. */
static void state_changed_cb (GstBus *bus, GstMessage *msg, CustomData *data)
{
    GstState old_state, new_state, pending_state;
    gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
    
    if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->playbin2))
    {
        data->state = new_state;
        g_print ("State set to %s\n", gst_element_state_get_name (new_state));
        if (old_state == GST_STATE_READY && new_state == GST_STATE_PAUSED)
        {
            /* For extra responsiveness, we refresh the GUI as soon as we reach the PAUSED state */
            refresh_ui (data);
        }
    }
}

GdkPixbuf *create_pixbuf(const gchar * filename)
{
    GdkPixbuf *pixbuf;
    GError *error = NULL;
    pixbuf = gdk_pixbuf_new_from_file(filename, &error);
    
    if(!pixbuf)
    {
        g_print("%s\n", error->message);
        g_error_free(error);
    }

    return pixbuf;
}

int main(int argc, char *argv[])
{
    CustomData data;

    /* Initialize GTK */
    gtk_init (&argc, &argv);

    /* Initialize GStreamer */
    gst_init (&argc, &argv);

    /* Initialize our data structure */
    memset (&data, 0, sizeof (data));
    data.duration = GST_CLOCK_TIME_NONE;

    /* Create the elements */
    data.playbin2 = gst_element_factory_make ("playbin2", "playbin2");

    if (!data.playbin2)
    {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }

    /* Create the GUI */
    create_ui (&data);

    data.IsFullScreen = FALSE;
    data.supports_alpha = FALSE;
    getScreenSize(&data.rootScrWidth, &data.rootScrHight);

    /* Create worker thread */


    /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
    data.bus = gst_element_get_bus (data.playbin2);
    gst_bus_add_signal_watch (data.bus);
    g_signal_connect (G_OBJECT (data.bus), "message::error", (GCallback)error_cb, &data);
    g_signal_connect (G_OBJECT (data.bus), "message::eos", (GCallback)eos_cb, &data);
    g_signal_connect (G_OBJECT (data.bus), "message::state-changed", (GCallback)state_changed_cb, &data);
    gst_object_unref (data.bus);

    /* Register a function that GLib will call every second */
    g_timeout_add(2,(GSourceFunc)refresh_ui, &data);

    /* Start the GTK main loop. We will not regain control until gtk_main_quit is called. */
    gtk_main ();

    /* Free resources */
    hm_delete(data.hm);
    gst_element_set_state (data.playbin2, GST_STATE_NULL);
    gst_object_unref (data.playbin2);

    return 0;
}
