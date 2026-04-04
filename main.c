#include <gst/gst.h>
#include <stdio.h>
#include <tcam-property-1.0.h>

int main(int argc, char* argv[])
{
    gst_debug_set_default_threshold(GST_LEVEL_WARNING);
    gst_init(&argc, &argv);

    const char* serial = NULL;
    GError* err = NULL;

    GstElement* pipeline =
        gst_parse_launch("tcambin name=source ! video/x-raw,format=BGRx ! videoconvert ! ximagesink", &err);

    if (pipeline == NULL)
    {
        printf("Could not create pipeline. Cause: %s\n", err ? err->message : "unknown");
        return 1;
    }

    GstElement* source = gst_bin_get_by_name(GST_BIN(pipeline), "source");

    if (serial != NULL)
    {
        GValue val = G_VALUE_INIT;
        g_value_init(&val, G_TYPE_STRING);
        g_value_set_static_string(&val, serial);
        g_object_set_property(G_OBJECT(source), "serial", &val);
        g_value_unset(&val);
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    tcam_property_provider_set_tcam_enumeration(
        TCAM_PROPERTY_PROVIDER(source), "TriggerMode", "On", &err);

    if (err)
    {
        printf("Error while setting trigger mode: %s\n", err->message);
        g_error_free(err);
        err = NULL;
    }

    while (1)
    {
        printf("Press 'q' then Enter to stop the stream.\n");
        printf("Press Enter to trigger a new image.\n");

        char c = getchar();

        if (c == 'q')
        {
            break;
        }

        tcam_property_provider_set_tcam_command(
            TCAM_PROPERTY_PROVIDER(source), "TriggerSoftware", &err);

        if (err)
        {
            printf("Could not trigger: %s\n", err->message);
            g_error_free(err);
            err = NULL;
        }
        else
        {
            printf("Triggered image.\n");
        }
    }

    tcam_property_provider_set_tcam_enumeration(
        TCAM_PROPERTY_PROVIDER(source), "TriggerMode", "Off", &err);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(source);
    gst_object_unref(pipeline);

    return 0;
}