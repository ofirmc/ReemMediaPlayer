#include "CustomDataHeader.h"

void pangotext(PangoLayout *layout, CustomData *data);

void rendertext(CustomData *data, GtkWidget *widget)
{
    int strWidth, strHight, finalHTextPos = 0;
    GtkAllocation allocation;
    GdkWindow *window = gtk_widget_get_window (widget);
    gtk_widget_get_allocation (widget, &allocation);

    data->cr = gdk_cairo_create (window);
    gtk_window_get_size(GTK_WINDOW(data->subs_window), &data->SubsScreenWidth, &data->SubsScreenHight);

    // layout for a paragraph of text
    data->layout = pango_cairo_create_layout(data->cr);	// init pango layout ready for use
    cairo_set_line_join(data->cr, CAIRO_LINE_JOIN_ROUND);
    pango_cairo_layout_path(data->cr, data->layout);
    pango_layout_set_auto_dir(data->layout, TRUE); // Set RTL option
    pango_layout_set_alignment(data->layout, PANGO_ALIGN_CENTER); // align text to center(not position of text)
    pango_layout_set_ellipsize(data->layout, PANGO_ELLIPSIZE_MIDDLE);

    pangotext(data->layout, data);	// pass the layout pointer to the function which does Pango rendering

    cairo_new_path(data->cr);


    // Get string width inorder to align the text to the center.
    pango_layout_get_pixel_size(data->layout, &strWidth, &strHight);
    finalHTextPos = data->SubsScreenWidth - strWidth;
    finalHTextPos /= 2;

    if(data->IsFullScreen)
        cairo_move_to(data->cr, finalHTextPos ,data->SubsScreenHight - 140); // Set text to final horizon position on screen
    else if(!data->IsFullScreen)
        cairo_move_to(data->cr, finalHTextPos ,data->SubsScreenHight - 50); // Set text to final horizon position on screen

    cairo_set_line_width(data->cr, 2.0); // sets the line width, default is 2.0

    cairo_set_source_rgba(data->cr, 0.0, 0.0, 0.0, 1.0); // set the colour to blue
    cairo_set_line_join(data->cr, CAIRO_LINE_JOIN_ROUND);
    // if the target surface or transformation properties of the cairo instance
    // have changed, update the pango layout to reflect this
    pango_cairo_update_layout(data->cr, data->layout); 
    
    pango_cairo_layout_path(data->cr, data->layout); // draw the pango layout onto the cairo surface
    cairo_stroke_preserve(data->cr); // draw a stroke along the path, but do not fill it

    cairo_set_source_rgb(data->cr, 1.0, 1.0, 1.0);
    cairo_fill(data->cr);

    cairo_destroy(data->cr);
    g_object_unref(data->layout);
}

void pangotext(PangoLayout *layout, CustomData *data)
{
    PangoFontDescription *desc;	// this structure stores a description of the style of font you'd most like
    pango_layout_set_text(layout, data->Hbuf, -1);	// sets the text to be associated with the layout (final arg is length, -1
    // to calculate automatically when passing a nul-terminated string)

    // specify the font that would be ideal for your particular use
    if(data->IsFullScreen)
    {
        desc = pango_font_description_from_string("arial 44");
    }
    else if(!data->IsFullScreen)
    {
        desc = pango_font_description_from_string("arial 16");
	}

    pango_layout_set_font_description(layout, desc); // assign the previous font description to the layout
    pango_font_description_free(desc); // free the description
}
