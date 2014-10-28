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

/**
 * SECTION:gstgtkglsink
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstgtkglsink.h"

#if GST_GL_HAVE_PLATFORM_EGL
#include <gst/gl/egl/gsteglimagememory.h>
#endif

GST_DEBUG_CATEGORY (gst_debug_gtk_gl_sink);
#define GST_CAT_DEFAULT gst_debug_gtk_gl_sink

static void gst_gtk_gl_sink_finalize (GObject * object);
static void gst_gtk_gl_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * param_spec);
static void gst_gtk_gl_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * param_spec);

static gboolean gst_gtk_gl_sink_stop (GstBaseSink * bsink);

static gboolean gst_gtk_gl_sink_query (GstBaseSink * bsink, GstQuery * query);

static GstStateChangeReturn
gst_gtk_gl_sink_change_state (GstElement * element, GstStateChange transition);

static void gst_gtk_gl_sink_get_times (GstBaseSink * bsink, GstBuffer * buf,
    GstClockTime * start, GstClockTime * end);
static gboolean gst_gtk_gl_sink_set_caps (GstBaseSink * bsink, GstCaps * caps);
static GstFlowReturn gst_gtk_gl_sink_show_frame (GstVideoSink * bsink,
    GstBuffer * buf);
static gboolean gst_gtk_gl_sink_propose_allocation (GstBaseSink * bsink,
    GstQuery * query);

static GstStaticPadTemplate gst_gtk_gl_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE_WITH_FEATURES
        (GST_CAPS_FEATURE_MEMORY_GL_MEMORY,
            "RGBA") "; "
#if GST_GL_HAVE_PLATFORM_EGL
        GST_VIDEO_CAPS_MAKE_WITH_FEATURES
        (GST_CAPS_FEATURE_MEMORY_EGL_IMAGE, "RGBA") "; "
#endif
        GST_VIDEO_CAPS_MAKE_WITH_FEATURES
        (GST_CAPS_FEATURE_META_GST_VIDEO_GL_TEXTURE_UPLOAD_META,
            "RGBA") "; " GST_VIDEO_CAPS_MAKE (GST_GL_COLOR_CONVERT_FORMATS))
    );

enum
{
  ARG_0,
  PROP_WIDGET
};

enum
{
  SIGNAL_0,
  LAST_SIGNAL
};

static guint gst_gtk_gl_sink_signals[LAST_SIGNAL] = { 0 };

#define gst_gtk_gl_sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstGtkGLSink, gst_gtk_gl_sink,
    GST_TYPE_VIDEO_SINK, GST_DEBUG_CATEGORY_INIT (gst_debug_gtk_gl_sink, "gtkglsink", 0,
        "Gtk Video Sink"));

static void
gst_gtk_gl_sink_class_init (GstGtkGLSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbasesink_class;
  GstVideoSinkClass *gstvideosink_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;
  gstvideosink_class = (GstVideoSinkClass *) klass;

  gobject_class->set_property = gst_gtk_gl_sink_set_property;
  gobject_class->get_property = gst_gtk_gl_sink_get_property;

  gst_element_class_set_metadata (gstelement_class, "Gtk Video Sink",
      "Sink/Video", "A video sink the renders to a GtkGstGLWidget",
      "Matthew Waters <matthew@centricular.com>");

  g_object_class_install_property (gobject_class, PROP_WIDGET,
      g_param_spec_object ("widget", "Gtk Widget",
          "Which GtkGstGLWidget to render into",
          GTK_TYPE_GST_GL_WIDGET, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_gtk_gl_sink_template));

  gobject_class->finalize = gst_gtk_gl_sink_finalize;

  gstelement_class->change_state = gst_gtk_gl_sink_change_state;
  gstbasesink_class->query = gst_gtk_gl_sink_query;
  gstbasesink_class->set_caps = gst_gtk_gl_sink_set_caps;
  gstbasesink_class->get_times = gst_gtk_gl_sink_get_times;
  gstbasesink_class->propose_allocation = gst_gtk_gl_sink_propose_allocation;
  gstbasesink_class->stop = gst_gtk_gl_sink_stop;

  gstvideosink_class->show_frame = gst_gtk_gl_sink_show_frame;
}

static void
gst_gtk_gl_sink_init (GstGtkGLSink * gtk_sink)
{
}

static void
gst_gtk_gl_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstGtkGLSink *gtk_sink;

  g_return_if_fail (GST_IS_GTK_GL_SINK (object));

  gtk_sink = GST_GTK_GL_SINK (object);

  switch (prop_id) {
    case PROP_WIDGET:
      GST_OBJECT_LOCK (gtk_sink);
      if (gtk_sink->widget)
        g_object_unref (gtk_sink->widget);
      gtk_sink->widget = g_value_get_object (value);
      GST_OBJECT_UNLOCK (gtk_sink);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_gtk_gl_sink_finalize (GObject * object)
{
  GstGtkGLSink *gtk_sink;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_gtk_gl_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstGtkGLSink *gtk_sink;

  gtk_sink = GST_GTK_GL_SINK (object);

  switch (prop_id) {
    case PROP_WIDGET:
      g_value_set_object (value, gtk_sink->widget);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_gtk_gl_sink_query (GstBaseSink * bsink, GstQuery * query)
{
  GstGtkGLSink *gtk_sink = GST_GTK_GL_SINK (bsink);
  gboolean res = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CONTEXT:
    {
      return gst_gl_handle_context_query ((GstElement *) gtk_sink, query,
          &gtk_sink->display, &gtk_sink->gtk_context);
    }
    default:
      res = GST_BASE_SINK_CLASS (parent_class)->query (bsink, query);
      break;
  }

  return res;
}

static gboolean
gst_gtk_gl_sink_stop (GstBaseSink * bsink)
{
  return TRUE;
}

static GstStateChangeReturn
gst_gtk_gl_sink_change_state (GstElement * element, GstStateChange transition)
{
  GstGtkGLSink *gtk_sink;
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

  GST_DEBUG ("changing state: %s => %s",
      gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (transition)),
      gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)));

  gtk_sink = GST_GTK_GL_SINK (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

  return ret;
}

static void
gst_gtk_gl_sink_get_times (GstBaseSink * bsink, GstBuffer * buf,
    GstClockTime * start, GstClockTime * end)
{
  GstGtkGLSink *gtk_sink;

  gtk_sink = GST_GTK_GL_SINK (bsink);

  if (GST_BUFFER_TIMESTAMP_IS_VALID (buf)) {
    *start = GST_BUFFER_TIMESTAMP (buf);
    if (GST_BUFFER_DURATION_IS_VALID (buf))
      *end = *start + GST_BUFFER_DURATION (buf);
    else {
      if (GST_VIDEO_INFO_FPS_N (&gtk_sink->v_info) > 0) {
        *end = *start +
            gst_util_uint64_scale_int (GST_SECOND,
            GST_VIDEO_INFO_FPS_D (&gtk_sink->v_info),
            GST_VIDEO_INFO_FPS_N (&gtk_sink->v_info));
      }
    }
  }
}

gboolean
gst_gtk_gl_sink_set_caps (GstBaseSink * bsink, GstCaps * caps)
{
  GstGtkGLSink *gtk_sink = GST_GTK_GL_SINK (bsink);

  GST_DEBUG ("set caps with %" GST_PTR_FORMAT, caps);

  if (!gst_video_info_from_caps (&gtk_sink->v_info, caps))
    return FALSE;

  if (!gtk_sink->widget)
    return FALSE;
  gtk_widget_realize (GTK_WIDGET (gtk_sink->widget));

  if (!gtk_gst_gl_widget_set_caps (gtk_sink->widget, caps))
    return FALSE;

  gtk_sink->display = gtk_gst_gl_widget_get_display (gtk_sink->widget);
  gtk_sink->context = gtk_gst_gl_widget_get_context (gtk_sink->widget);
  gtk_sink->gtk_context = gtk_gst_gl_widget_get_gtk_context (gtk_sink->widget);

  if (!gtk_sink->display || !gtk_sink->context || !gtk_sink->gtk_context)
    return FALSE;

  return TRUE;
}

static GstFlowReturn
gst_gtk_gl_sink_show_frame (GstVideoSink * vsink, GstBuffer * buf)
{
  GstGtkGLSink *gtk_sink;
  GstBuffer *stored_buffer;

  GST_TRACE ("rendering buffer:%p", buf);

  gtk_sink = GST_GTK_GL_SINK (vsink);

  gtk_gst_gl_widget_set_buffer (gtk_sink->widget, buf);

  return GST_FLOW_OK;
}

static gboolean
gst_gtk_gl_sink_propose_allocation (GstBaseSink * bsink, GstQuery * query)
{
  GstGtkGLSink *gtk_sink = GST_GTK_GL_SINK (bsink);
  GstBufferPool *pool;
  GstStructure *config;
  GstCaps *caps;
  guint size;
  gboolean need_pool;
  GstStructure *gl_context;
  gchar *platform, *gl_apis;
  gpointer handle;
  GstAllocator *allocator = NULL;
  GstAllocationParams params;

  if (!gtk_sink->display || !gtk_sink->context)
    return FALSE;

  gst_query_parse_allocation (query, &caps, &need_pool);

  if (caps == NULL)
    goto no_caps;

  if ((pool = gtk_sink->pool))
    gst_object_ref (pool);

  if (pool != NULL) {
    GstCaps *pcaps;

    /* we had a pool, check caps */
    GST_DEBUG_OBJECT (gtk_sink, "check existing pool caps");
    config = gst_buffer_pool_get_config (pool);
    gst_buffer_pool_config_get_params (config, &pcaps, &size, NULL, NULL);

    if (!gst_caps_is_equal (caps, pcaps)) {
      GST_DEBUG_OBJECT (gtk_sink, "pool has different caps");
      /* different caps, we can't use this pool */
      gst_object_unref (pool);
      pool = NULL;
    }
    gst_structure_free (config);
  }

  if (pool == NULL && need_pool) {
    GstVideoInfo info;

    if (!gst_video_info_from_caps (&info, caps))
      goto invalid_caps;

    GST_DEBUG_OBJECT (gtk_sink, "create new pool");
    pool = gst_gl_buffer_pool_new (gtk_sink->context);

    /* the normal size of a frame */
    size = info.size;

    config = gst_buffer_pool_get_config (pool);
    gst_buffer_pool_config_set_params (config, caps, size, 0, 0);
    if (!gst_buffer_pool_set_config (pool, config))
      goto config_failed;
  }
  /* we need at least 2 buffer because we hold on to the last one */
  if (pool) {
    gst_query_add_allocation_pool (query, pool, size, 2, 0);
    gst_object_unref (pool);
  }

  /* we also support various metadata */
  gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, 0);

  gl_apis =
      gst_gl_api_to_string (gst_gl_context_get_gl_api (gtk_sink->context));
  platform =
      gst_gl_platform_to_string (gst_gl_context_get_gl_platform
      (gtk_sink->context));
  handle = (gpointer) gst_gl_context_get_gl_context (gtk_sink->context);

  gl_context =
      gst_structure_new ("GstVideoGLTextureUploadMeta", "gst.gl.GstGLContext",
      GST_GL_TYPE_CONTEXT, gtk_sink->context, "gst.gl.context.handle",
      G_TYPE_POINTER, handle, "gst.gl.context.type", G_TYPE_STRING, platform,
      "gst.gl.context.apis", G_TYPE_STRING, gl_apis, NULL);
  gst_query_add_allocation_meta (query,
      GST_VIDEO_GL_TEXTURE_UPLOAD_META_API_TYPE, gl_context);

  g_free (gl_apis);
  g_free (platform);
  gst_structure_free (gl_context);

  gst_allocation_params_init (&params);

  allocator = gst_allocator_find (GST_GL_MEMORY_ALLOCATOR);
  gst_query_add_allocation_param (query, allocator, &params);
  gst_object_unref (allocator);

#if GST_GL_HAVE_PLATFORM_EGL
  if (gst_gl_context_check_feature (gtk_sink->context,
          "EGL_KHR_image_base")) {
    allocator = gst_allocator_find (GST_EGL_IMAGE_MEMORY_TYPE);
    gst_query_add_allocation_param (query, allocator, &params);
    gst_object_unref (allocator);
  }
#endif

  return TRUE;

  /* ERRORS */
no_caps:
  {
    GST_DEBUG_OBJECT (bsink, "no caps specified");
    return FALSE;
  }
invalid_caps:
  {
    GST_DEBUG_OBJECT (bsink, "invalid caps specified");
    return FALSE;
  }
config_failed:
  {
    GST_DEBUG_OBJECT (bsink, "failed setting config");
    return FALSE;
  }
}
