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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "gtkgstglwidget.h"
#include <gst/video/video.h>

#if GTK_GST_HAVE_X11
#include <gdk/gdkx.h>
#include <gst/gl/x11/gstgldisplay_x11.h>
#include <gst/gl/x11/gstglcontext_glx.h>
#endif

/**
 * SECTION:gtkgstglwidget
 * @short_description: a #GtkGLArea that renders GStreamer video #GstBuffers
 * @see_also: #GtkGLArea, #GstBuffer
 *
 * #GtkGstGLWidget is an #GtkWidget that renders GStreamer video buffers.
 */

G_DEFINE_TYPE (GtkGstGLWidget, gtk_gst_gl_widget, GTK_TYPE_GL_AREA);

#define GTK_GST_GL_WIDGET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
    GTK_TYPE_GST_GL_WIDGET, GtkGstGLWidgetPrivate))

struct _GtkGstGLWidgetPrivate
{
  GMutex            lock;

  gboolean          negotiated;
  GstBuffer        *buffer;
  GstCaps          *gl_caps;
  GstCaps          *caps;
  GstVideoInfo      v_info;
  gboolean          new_buffer;

  gboolean          initted;
  GstGLDisplay     *display;
  GstGLContext     *other_context;
  GstGLContext     *context;
  GstGLUpload      *upload;
  GstGLShader      *shader;
  GLuint            vao;
  GLuint            vertex_buffer;
  GLuint            attr_position;
  GLuint            attr_texture;
  GLuint            current_tex;
};

static void
gtk_gst_gl_widget_get_preferred_width (GtkWidget * widget, gint *min, gint *natural)
{
  GtkGstGLWidget *gst_widget = (GtkGstGLWidget *) widget;
  gint video_width = GST_VIDEO_INFO_WIDTH (&gst_widget->priv->v_info);

  if (!gst_widget->priv->negotiated)
    video_width = 10;

  if (min)
    *min = 1;
  if (natural)
    *natural = video_width;
}

static void
gtk_gst_gl_widget_get_preferred_height (GtkWidget * widget, gint *min, gint *natural)
{
  GtkGstGLWidget *gst_widget = (GtkGstGLWidget *) widget;
  gint video_height = GST_VIDEO_INFO_HEIGHT (&gst_widget->priv->v_info);

  if (!gst_widget->priv->negotiated)
    video_height = 10;

  if (min)
    *min = 1;
  if (natural)
    *natural = video_height;
}

static const GLfloat vertices[] = {
     1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 1.0f
};

static void
gtk_gst_gl_widget_bind_buffer (GtkGstGLWidget * gst_widget)
{
  const GstGLFuncs *gl = gst_widget->priv->context->gl_vtable;

  gl->BindBuffer (GL_ARRAY_BUFFER, gst_widget->priv->vertex_buffer);

  /* Load the vertex position */
  gl->VertexAttribPointer (gst_widget->priv->attr_position, 3, GL_FLOAT, GL_FALSE,
      5 * sizeof (GLfloat), (void *) 0);

  /* Load the texture coordinate */
  gl->VertexAttribPointer (gst_widget->priv->attr_texture, 2, GL_FLOAT, GL_FALSE,
      5 * sizeof (GLfloat), (void *) (3 * sizeof (GLfloat)));

  gl->EnableVertexAttribArray (gst_widget->priv->attr_position);
  gl->EnableVertexAttribArray (gst_widget->priv->attr_texture);
}

static void
gtk_gst_gl_widget_unbind_buffer (GtkGstGLWidget * gst_widget)
{
  const GstGLFuncs *gl = gst_widget->priv->context->gl_vtable;

  gl->BindBuffer (GL_ARRAY_BUFFER, 0);

  gl->DisableVertexAttribArray (gst_widget->priv->attr_position);
  gl->DisableVertexAttribArray (gst_widget->priv->attr_texture);
}

static void
gtk_gst_gl_widget_init_redisplay (GtkGstGLWidget * gst_widget)
{
  const GstGLFuncs *gl = gst_widget->priv->context->gl_vtable;

  gst_widget->priv->shader = gst_gl_shader_new (gst_widget->priv->context);

  gst_gl_shader_compile_with_default_vf_and_check
      (gst_widget->priv->shader, &gst_widget->priv->attr_position,
      &gst_widget->priv->attr_texture);

  if (gl->GenVertexArrays) {
    gl->GenVertexArrays (1, &gst_widget->priv->vao);
    gl->BindVertexArray (gst_widget->priv->vao);
  }

  gl->GenBuffers (1, &gst_widget->priv->vertex_buffer);
  gl->BindBuffer (GL_ARRAY_BUFFER, gst_widget->priv->vertex_buffer);
  gl->BufferData (GL_ARRAY_BUFFER, 4 * 5 * sizeof (GLfloat), vertices,
      GL_STATIC_DRAW);

  if (gl->GenVertexArrays) {
    gtk_gst_gl_widget_bind_buffer (gst_widget);
    gl->BindVertexArray (0);
  }

  gl->BindBuffer (GL_ARRAY_BUFFER, 0);

  gst_widget->priv->initted = TRUE;
}

static void
_flush_gl (GstGLContext * context, gpointer data)
{
  const GstGLFuncs *gl = context->gl_vtable;

  gl->Flush ();
}

static void
_redraw_texture (GtkGstGLWidget * gst_widget, guint tex)
{
  const GstGLFuncs *gl = gst_widget->priv->context->gl_vtable;
  GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

  gst_gl_shader_use (gst_widget->priv->shader);

  if (gl->BindVertexArray)
    gl->BindVertexArray (gst_widget->priv->vao);
  else
    gtk_gst_gl_widget_bind_buffer (gst_widget);

  gl->ActiveTexture (GL_TEXTURE0);
  gl->BindTexture (GL_TEXTURE_2D, tex);
  gst_gl_shader_set_uniform_1i (gst_widget->priv->shader, "tex", 0);

  gl->DrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);

  if (gl->BindVertexArray)
    gl->BindVertexArray (0);
  else
    gtk_gst_gl_widget_unbind_buffer (gst_widget);

  gl->BindTexture (GL_TEXTURE_2D, 0);
}

static gboolean
gtk_gst_gl_widget_render (GtkGLArea * widget, GdkGLContext *context)
{
  GtkGstGLWidget *gst_widget = (GtkGstGLWidget *) widget;
  const GstGLFuncs *gl = gst_widget->priv->context->gl_vtable;
  guint widget_width, widget_height;
  cairo_surface_t *surface;
  GstVideoFrame frame;
  guint fbo;
  GstBuffer *current_buffer;

  g_mutex_lock (&gst_widget->priv->lock);

  if (!gst_widget->priv->initted)
    gtk_gst_gl_widget_init_redisplay (gst_widget);

  if (gst_widget->priv->initted && gst_widget->priv->negotiated
        && gst_widget->priv->buffer && gst_widget->priv->upload) {
    gst_gl_upload_set_caps (gst_widget->priv->upload, gst_widget->priv->caps,
        gst_widget->priv->gl_caps);

    if (gst_widget->priv->new_buffer || gst_widget->priv->current_tex == 0) {
      GstBuffer *gl_buf;
      GstVideoFrame gl_frame;

      if (!gst_gl_upload_perform_with_buffer (gst_widget->priv->upload,
            gst_widget->priv->buffer, &gl_buf)) {
        goto error;
      }

      if (!gst_video_frame_map (&gl_frame, &gst_widget->priv->v_info, gl_buf,
            GST_MAP_READ | GST_MAP_GL)) {
        goto error;
      }

      gst_widget->priv->current_tex = *(guint *) gl_frame.data[0];
    }
    gst_gl_context_thread_add (gst_widget->priv->context,
        (GstGLContextThreadFunc) _flush_gl, NULL);

    _redraw_texture (gst_widget, gst_widget->priv->current_tex);
    gst_widget->priv->new_buffer = FALSE;
  } else {
error:
    /* FIXME: nothing to display */
    glClearColor (1.0, 0.0, 0.0, 1.0);
    glClear (GL_COLOR_BUFFER_BIT);
  }

  g_mutex_unlock (&gst_widget->priv->lock);
  return FALSE;
}

static void
_reset_gl (GstGLContext * context, GtkGstGLWidget * gst_widget)
{
  const GstGLFuncs *gl = gst_widget->priv->context->gl_vtable;

  if (gst_widget->priv->vao) {
    gl->DeleteVertexArrays (1, &gst_widget->priv->vao);
    gst_widget->priv->vao = 0;
  }

  if (gst_widget->priv->vertex_buffer) {
    gl->DeleteBuffers (1, &gst_widget->priv->vertex_buffer);
    gst_widget->priv->vertex_buffer = 0;
  }
}

static void
_reset (GtkGstGLWidget * gst_widget)
{
  gst_buffer_replace (&gst_widget->priv->buffer, NULL);

  if (gst_widget->priv->shader) {
    gst_object_unref (gst_widget->priv->shader);
    gst_widget->priv->shader = NULL;
  }

  if (gst_widget->priv->upload) {
    gst_object_unref (gst_widget->priv->upload);
    gst_widget->priv->upload = NULL;
  }

  gst_caps_replace (&gst_widget->priv->caps, NULL);

  if (gst_widget->priv->context) {
    gst_gl_context_thread_add (gst_widget->priv->context,
        (GstGLContextThreadFunc) _reset_gl, gst_widget);
    gst_object_unref (gst_widget->priv->other_context);
    gst_widget->priv->other_context = NULL;
  }

  if (gst_widget->priv->context) {
    gst_object_unref (gst_widget->priv->context);
    gst_widget->priv->context = NULL;
  }

  gst_widget->priv->negotiated = FALSE;
  gst_widget->priv->initted = FALSE;
  gst_widget->priv->vao = 0;
  gst_widget->priv->vertex_buffer = 0;
  gst_widget->priv->attr_position = 0;
  gst_widget->priv->attr_texture = 0;
  gst_widget->priv->current_tex = 0;
  gst_widget->priv->new_buffer = TRUE;
}

static void
gtk_gst_gl_widget_finalize (GObject * object)
{
  GtkGstGLWidget *widget = GTK_GST_GL_WIDGET_CAST (object);

  g_mutex_clear (&widget->priv->lock);

  _reset (widget);

  if (widget->priv->display) {
    gst_object_unref (widget->priv->display);
    widget->priv->display = NULL;
  }

  G_OBJECT_CLASS (gtk_gst_gl_widget_parent_class)->finalize (object);
}

static void
gtk_gst_gl_widget_class_init (GtkGstGLWidgetClass * klass)
{
  GtkWidgetClass *widget_klass = (GtkWidgetClass *) klass;
  GtkGLAreaClass *gl_widget_klass = (GtkGLAreaClass *) klass;

  g_type_class_add_private (klass, sizeof (GtkGstGLWidgetPrivate));

  gl_widget_klass->render = gtk_gst_gl_widget_render;
  widget_klass->get_preferred_width = gtk_gst_gl_widget_get_preferred_width;
  widget_klass->get_preferred_height = gtk_gst_gl_widget_get_preferred_height;

  G_OBJECT_CLASS (klass)->finalize = gtk_gst_gl_widget_finalize;
}

static void
gtk_gst_gl_widget_init (GtkGstGLWidget * widget)
{
  GdkDisplay *display;

  widget->priv = GTK_GST_GL_WIDGET_GET_PRIVATE (widget);

  g_mutex_init (&widget->priv->lock);

  display = gdk_display_get_default ();

#if GTK_GST_HAVE_X11
  if (GDK_IS_X11_DISPLAY (display))
    widget->priv->display = (GstGLDisplay *) gst_gl_display_x11_new_with_display (gdk_x11_display_get_xdisplay (display));
#endif

  if (!widget->priv->display)
    widget->priv->display = gst_gl_display_new ();

  gtk_gl_area_set_has_alpha ((GtkGLArea *) widget, TRUE);
}

GtkWidget *
gtk_gst_gl_widget_new (void)
{
  return (GtkWidget *) g_object_new (GTK_TYPE_GST_GL_WIDGET, NULL);
}

static gboolean
_queue_draw (GtkGstGLWidget * widget)
{
  gtk_widget_queue_draw (GTK_WIDGET (widget));

  return G_SOURCE_REMOVE;
}

void
gtk_gst_gl_widget_set_buffer (GtkGstGLWidget * widget, GstBuffer *buffer)
{
  GMainContext *main_context = g_main_context_default ();

  g_return_if_fail (GTK_IS_GST_GL_WIDGET (widget));
  g_return_if_fail (widget->priv->negotiated);
  g_return_if_fail (GST_IS_BUFFER (buffer));

  g_mutex_lock (&widget->priv->lock);

  gst_buffer_replace (&widget->priv->buffer, buffer);
  widget->priv->new_buffer = TRUE;

  g_mutex_unlock (&widget->priv->lock);

  g_main_context_invoke (main_context, (GSourceFunc) _queue_draw, widget);
}

struct get_context
{
  GtkGstGLWidget *widget;
  GMutex lock;
  GCond cond;
  gboolean fired;
  GstGLContext *other_context;
};

static gboolean
_get_gl_context (struct get_context *info)
{
  GtkGstGLWidget *gst_widget = info->widget;
  GdkGLContext *gdk_context;
  GstGLPlatform platform;
  GstGLAPI gl_api;
  guintptr gl_handle;

  g_mutex_lock (&info->lock);

  gtk_widget_realize (GTK_WIDGET (gst_widget));

  gdk_context = gtk_gl_area_get_context (GTK_GL_AREA (gst_widget));
  if (gdk_context == NULL) {
    g_assert_not_reached ();
    goto out;
  }

  gdk_gl_context_make_current (gdk_context);

#if GTK_GST_HAVE_X11
  if (GST_IS_GL_DISPLAY_X11 (gst_widget->priv->display)) {
    platform = GST_GL_PLATFORM_GLX;
    gl_api = gst_gl_context_get_current_gl_api (NULL, NULL);
    gl_handle = gst_gl_context_get_current_gl_context (platform);
    if (gl_handle)
      info->other_context = gst_gl_context_new_wrapped (gst_widget->priv->display, gl_handle, platform, gl_api);
  }
#endif

out:
  info->fired = TRUE;
  g_cond_signal (&info->cond);
  g_mutex_unlock (&info->lock);
  return G_SOURCE_REMOVE;
}

static gboolean
_queue_resize (GtkGstGLWidget * widget)
{
  gtk_widget_queue_resize (GTK_WIDGET (widget));

  return G_SOURCE_REMOVE;
}

gboolean
gtk_gst_gl_widget_set_caps (GtkGstGLWidget * widget, GstCaps *caps)
{
  GMainContext *main_context = g_main_context_default ();
  GstVideoInfo v_info;

  g_return_val_if_fail (GTK_IS_GST_GL_WIDGET (widget), FALSE);
  g_return_val_if_fail (GST_IS_CAPS (caps), FALSE);
  g_return_val_if_fail (gst_caps_is_fixed (caps), FALSE);

  if (widget->priv->caps && gst_caps_is_equal_fixed (widget->priv->caps, caps))
    return TRUE;

  if (!gst_video_info_from_caps (&v_info, caps))
    return FALSE;

  g_mutex_lock (&widget->priv->lock);

  _reset (widget);

  if (!widget->priv->negotiated) {
    struct get_context info;

    info.widget = widget;
    g_mutex_init (&info.lock);
    g_cond_init (&info.cond);
    info.fired = FALSE;
    info.other_context = NULL;

    g_main_context_invoke (main_context, (GSourceFunc) _get_gl_context, &info);

    g_mutex_unlock (&widget->priv->lock);
    g_mutex_lock (&info.lock);
    while (!info.fired)
      g_cond_wait (&info.cond, &info.lock);
    g_mutex_unlock (&info.lock);
    g_mutex_lock (&widget->priv->lock);

    g_mutex_clear (&info.lock);
    g_cond_clear (&info.cond);
    widget->priv->other_context = info.other_context;
  }

  if (!GST_GL_IS_CONTEXT (widget->priv->other_context)) {
    g_mutex_unlock (&widget->priv->lock);
    g_return_val_if_fail (GST_GL_IS_CONTEXT (widget->priv->other_context), FALSE);
  }

  widget->priv->context = gst_gl_context_new (widget->priv->display);
  gst_gl_context_create (widget->priv->context, widget->priv->other_context, NULL);

  gst_caps_replace (&widget->priv->caps, caps);

  gst_caps_replace (&widget->priv->gl_caps, NULL);
  widget->priv->gl_caps = gst_video_info_to_caps (&v_info);
  gst_caps_set_features (widget->priv->gl_caps, 0,
      gst_caps_features_from_string (GST_CAPS_FEATURE_MEMORY_GL_MEMORY));

  if (widget->priv->upload)
    gst_object_unref (widget->priv->upload);
  widget->priv->upload = gst_gl_upload_new (widget->priv->context);

  widget->priv->v_info = v_info;
  widget->priv->negotiated = TRUE;

  g_mutex_unlock (&widget->priv->lock);

  g_main_context_invoke (main_context, (GSourceFunc) _queue_resize, widget);

  return TRUE;
}

GstGLContext *
gtk_gst_gl_widget_get_gtk_context (GtkGstGLWidget *gst_widget)
{
  g_return_val_if_fail (gst_widget->priv->negotiated, NULL);

  return gst_object_ref (gst_widget->priv->other_context);
}

GstGLContext *
gtk_gst_gl_widget_get_context (GtkGstGLWidget *gst_widget)
{
  g_return_val_if_fail (gst_widget->priv->negotiated, NULL);

  return gst_object_ref (gst_widget->priv->context);
}

GstGLDisplay *
gtk_gst_gl_widget_get_display (GtkGstGLWidget *gst_widget)
{
  g_return_val_if_fail (gst_widget->priv->negotiated, NULL);

  return gst_object_ref (gst_widget->priv->display);
}
