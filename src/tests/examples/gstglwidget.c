/*
 * GStreamer
 * Copyright (C) 2014 Matthew Waters <matthew@centricular.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <gtkgstglwidget.h>
#include <gst/app/gstappsink.h>
#if GST_GL_HAVE_WINDOW_X11
#include <gst/gl/x11/gstgldisplay_x11.h>
#endif

static void
button_state_null_cb (GtkWidget * widget, GstElement * pipeline)
{
  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_print ("GST_STATE_NULL\n");
}

static void
button_state_ready_cb (GtkWidget * widget, GstElement * pipeline)
{
  gst_element_set_state (pipeline, GST_STATE_READY);
  g_print ("GST_STATE_READY\n");
}

static void
button_state_paused_cb(GtkWidget * widget, GstElement * pipeline)
{
  gst_element_set_state (pipeline, GST_STATE_PAUSED);
  g_print ("GST_STATE_PAUSED\n");
}

static void
button_state_playing_cb(GtkWidget * widget, GstElement * pipeline)
{
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  g_print ("GST_STATE_PLAYING\n");
}

static void
end_stream_cb (GstBus * bus, GstMessage * message, GstElement * pipeline)
{
  g_print("End of stream\n");

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  gtk_main_quit();
}

static void
destroy_cb (GtkWidget * widget, GdkEvent * event, GstElement * pipeline)
{
  g_print("Close\n");

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  gtk_main_quit();
}

static void
eos (GstAppSink * app_sink, gpointer widget)
{
  gtk_main_quit();
}

GstFlowReturn
new_sample (GstAppSink * app_sink, gpointer widget)
{
  GtkGstGLWidget *gst_widget = widget;
  GstSample *sample = gst_base_sink_get_last_sample ((GstBaseSink *) app_sink);

  if (!sample)
    return GST_FLOW_OK;

  GstCaps *caps = gst_sample_get_caps (sample);
  GstBuffer *buffer = gst_sample_get_buffer (sample);

  gtk_gst_gl_widget_set_caps (gst_widget, caps);
  gtk_gst_gl_widget_set_buffer (gst_widget, buffer);

  if (GST_IS_SAMPLE (sample))
    gst_sample_unref (sample);

  return GST_FLOW_OK;
}

static GstAppSinkCallbacks appsink_cbs = {
  (void(*)(GstAppSink*,gpointer))eos, new_sample, new_sample,
};

int
main (int argc, char *argv[])
{
#if GST_GL_HAVE_WINDOW_X11
  XInitThreads ();
#endif
  gst_init (&argc, &argv);
  gtk_init (&argc, &argv);

  GstElement* pipeline = gst_pipeline_new ("pipeline");

  //window that contains an area where the video is drawn
  GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), 640, 480);
  gtk_window_move (GTK_WINDOW (window), 300, 10);
  gtk_window_set_title (GTK_WINDOW (window), "gtkgstglwidget");

  //window to control the states
  GtkWidget* window_control = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_resizable (GTK_WINDOW (window_control), FALSE);
  gtk_window_move (GTK_WINDOW (window_control), 10, 10);
  GtkWidget* grid = gtk_grid_new ();
  gtk_container_add (GTK_CONTAINER (window_control), grid);

  //control state null
  GtkWidget* button_state_null = gtk_button_new_with_label ("GST_STATE_NULL");
  g_signal_connect (G_OBJECT (button_state_null), "clicked",
      G_CALLBACK (button_state_null_cb), pipeline);
  gtk_grid_attach (GTK_GRID (grid), button_state_null, 0, 1, 1, 1);
  gtk_widget_show (button_state_null);

  //control state ready
  GtkWidget* button_state_ready = gtk_button_new_with_label ("GST_STATE_READY");
  g_signal_connect (G_OBJECT (button_state_ready), "clicked",
      G_CALLBACK (button_state_ready_cb), pipeline);
  gtk_grid_attach (GTK_GRID (grid), button_state_ready, 0, 2, 1, 1);
  gtk_widget_show (button_state_ready);

  //control state paused
  GtkWidget* button_state_paused = gtk_button_new_with_label ("GST_STATE_PAUSED");
  g_signal_connect (G_OBJECT (button_state_paused), "clicked",
      G_CALLBACK (button_state_paused_cb), pipeline);
  gtk_grid_attach (GTK_GRID (grid), button_state_paused, 0, 3, 1, 1);
  gtk_widget_show (button_state_paused);

  //control state playing
  GtkWidget* button_state_playing = gtk_button_new_with_label ("GST_STATE_PLAYING");
  g_signal_connect (G_OBJECT (button_state_playing), "clicked",
      G_CALLBACK (button_state_playing_cb), pipeline);
  gtk_grid_attach (GTK_GRID (grid), button_state_playing, 0, 4, 1, 1);
  gtk_widget_show (button_state_playing);

  gtk_widget_show (grid);
  gtk_widget_show (window_control);

  //area where the video is drawn
  GtkWidget* area = gtk_gst_gl_widget_new();
  gtk_container_add (GTK_CONTAINER (window), area);

  gtk_widget_realize(area);

  g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(destroy_cb), pipeline);

  //configure the pipeline
  GstElement* videosrc = gst_element_factory_make ("videotestsrc", "videotestsrc");
  GstElement* videosink = gst_element_factory_make ("appsink", "appsink");

  gst_app_sink_set_callbacks ((GstAppSink *) videosink, &appsink_cbs, area, NULL);

  GstCaps *caps = gst_caps_new_simple("video/x-raw",
                                      "width", G_TYPE_INT, 640,
                                      "height", G_TYPE_INT, 480,
                                      "format", G_TYPE_STRING, "BGRA",
                                      NULL) ;
  g_object_set (videosink, "caps", caps, NULL);

  gst_bin_add_many (GST_BIN (pipeline), videosrc, videosink, NULL);

  gboolean link_ok = gst_element_link(videosrc, videosink) ;
  gst_caps_unref(caps) ;
  if(!link_ok)
  {
      g_warning("Failed to link videosrc to glfiltercube!\n") ;
      return -1;
  }

  //set window id on this event
  GstBus* bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  g_signal_connect(bus, "message::error", G_CALLBACK(end_stream_cb), pipeline);
  g_signal_connect(bus, "message::warning", G_CALLBACK(end_stream_cb), pipeline);
  g_signal_connect(bus, "message::eos", G_CALLBACK(end_stream_cb), pipeline);
  gst_object_unref (bus);

  //start
  GstStateChangeReturn ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE)
  {
      g_print ("Failed to start up pipeline!\n");
      return -1;
  }

  gtk_widget_show_all (window);

  gtk_main();

  gst_deinit ();

  return 0;
}


