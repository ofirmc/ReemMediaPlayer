#include "CustomDataHeader.h"
#include "RenderText.h"

void ClearSubsWindow(CustomData *data);

void DispSubsByPosition(gint64 position, CustomData *data)
{
    char curTotlaTime[40], curMin[5], curSec[5], curenthours[5], milsectimeBuf[5];
    char endTImeLine[100] = "--> ";    
    guint milSec = 9999999;
    milSec = position / GST_MSECOND;
    guint sec = ((milSec % (1000*60*60)) % (1000*60)) / 1000;
    guint min = (milSec % (1000*60*60)) / (1000*60);
    guint hours = milSec / (1000 * 60 * 60);
    guint milsec2 = ((position / GST_MSECOND) % 1000) / 10;

    sprintf(curenthours, "0%d", hours);

    if(sec < 10)
    {
        sprintf(curSec, "0%d", sec);
    }
    else
    {
        sprintf(curSec, "%d", sec);
	}
	
    if(min < 10)
    {
        sprintf(curMin, "0%d", min);
    }
    else
    {
        sprintf(curMin, "%d", min);
    }

    if(milsec2 < 10)
    {
        sprintf(milsectimeBuf, "0%d", milsec2);
    }
    else
    {
        sprintf(milsectimeBuf, "%d", milsec2);
    }

    if(data->IsSubtitlesLoaded)
    {
        curTotlaTime[0] = 0;
        strcat(curTotlaTime, curenthours);
        strcat(curTotlaTime, ":");
        strcat(curTotlaTime, curMin);
        strcat(curTotlaTime, ":");
        strcat(curTotlaTime, curSec);
        strcat(curTotlaTime, ":");
        strcat(curTotlaTime, milsectimeBuf);
        strcat(endTImeLine, curTotlaTime);
                
        int Hresult;
        hm_get(data->hm, curTotlaTime, data->Hbuf, sizeof(data->Hbuf));

        if(hm_exists(data->hm, curTotlaTime))
        {
            Hresult = hm_get(data->hm, curTotlaTime, data->Hbuf, sizeof(data->Hbuf));
            if(Hresult == 0)
            {
                g_error("Value from hash table not found");
            }

            ClearSubsWindow(data);//Clear subtitles window before new text render
            rendertext(data, data->subs_window);
            g_print("%s", data->Hbuf);
        }
        else if(hm_exists(data->hm, endTImeLine))
        {
            //We have to use ClearSubsWindow twise to clear subs window
            ClearSubsWindow(data);
            strcpy(data->Hbuf, "1");//Must have a string and not an empty one neither spase
            rendertext(data, data->subs_window);
            ClearSubsWindow(data);
        }
    }
}

void ClearSubsWindow(CustomData *data)
{
    data->cr_window = gdk_cairo_create(data->subs_window->window);

    if (data->supports_alpha)
    {
        cairo_set_source_rgba (data->cr_window, 1.0, 1.0, 1.0, 0.0); /* transparent */
    }
    else
    {
        cairo_set_source_rgb (data->cr_window, 1.0, 1.0, 1.0); /* opaque white */
    }

    /* draw the background */
    cairo_set_operator (data->cr_window, CAIRO_OPERATOR_SOURCE);
    cairo_paint (data->cr_window);
    cairo_destroy(data->cr_window);
    gdk_flush ();
}
