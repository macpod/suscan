/*

  Copyright (C) 2017 Gonzalo José Carracedo Carballal

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program.  If not, see
  <http://www.gnu.org/licenses/>

*/

#include <glib-object.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include "symview.h"

G_DEFINE_TYPE(SuGtkSymView, sugtk_sym_view, GTK_TYPE_DRAWING_AREA);

void
sugtk_sym_view_clear(SuGtkSymView *view)
{
  if (view->data_buf != NULL) {
    free(view->data_buf);
    view->data_buf = NULL;
  }

  view->data_alloc = 0;
  view->data_size = 0;
}

gboolean
sugtk_sym_view_append(SuGtkSymView *view, uint8_t data)
{
  uint8_t *tmp = NULL;
  unsigned int new_alloc;
  gboolean ok = FALSE;

  if (view->data_alloc <= view->data_size) {
    if (view->data_alloc > 0)
      new_alloc = view->data_alloc << 1;
    else
      new_alloc = 1;

    if ((tmp = realloc(view->data_buf, new_alloc)) == NULL)
      goto fail;

    view->data_alloc = new_alloc;
    view->data_buf = tmp;
    tmp = NULL;
  }

  view->data_buf[view->data_size++] = data;

  if (view->autoscroll)
    if (view->window_stride * view->window_height < view->data_size)
      view->window_offset = view->window_stride
          * (1 + view->data_size / view->window_stride - view->window_height);

  ok = TRUE;

fail:
  if (tmp != NULL)
    free(tmp);

  return ok;
}

void
sugtk_sym_view_set_autoscroll(SuGtkSymView *view, gboolean value)
{
  view->autoscroll = value;
}

static void
sugtk_sym_view_dispose(GObject* object)
{
  G_OBJECT_CLASS(sugtk_sym_view_parent_class)->dispose(object);
}

static void
sugtk_sym_view_class_init(SuGtkSymViewClass *class)
{
  GObjectClass  *g_object_class;

  g_object_class = G_OBJECT_CLASS(class);

  g_object_class->dispose = sugtk_sym_view_dispose;
}

static gboolean
sugtk_sym_view_on_configure_event(
    GtkWidget *widget,
    GdkEventConfigure *event,
    gpointer data)
{
  SuGtkSymView *view = SUGTK_SYM_VIEW(widget);

  view->window_width = event->width;
  view->window_height = event->height;
  view->window_stride = cairo_format_stride_for_width(
      CAIRO_FORMAT_A8,
      view->window_width);

  return TRUE;
}

static gboolean
suscan_constellation_on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
  SuGtkSymView *view = SUGTK_SYM_VIEW(widget);
  cairo_surface_t *surface;
  unsigned int height;
  unsigned int width;
  unsigned int tail = 0;
  unsigned int offset;

  width = view->window_stride;
  height = view->window_height;

  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_paint(cr);

  offset = view->window_offset;

  if (offset < view->data_size) {
    if (width * height + offset > view->data_size) {
      height = (view->data_size - offset) / width;
      tail = view->data_size - offset - width * height;
    }

    if (height + tail > 0) {
      surface = cairo_image_surface_create_for_data(
          view->data_buf + offset,
          CAIRO_FORMAT_A8,
          width,
          height,
          width);
      cairo_set_source_rgb(cr, 1, 1, 0);
      cairo_mask_surface(cr, surface, 0, 0);
      cairo_surface_destroy(surface);

      if (tail > 0) {
        surface = cairo_image_surface_create_for_data(
            view->data_buf + width * height + offset,
            CAIRO_FORMAT_A8,
            tail,
            1,
            width);
        cairo_set_source_rgb(cr, 1, 1, 0);
        cairo_mask_surface(cr, surface, 0, height);
        cairo_surface_destroy(surface);
      }
    }
  }

  return TRUE;
}

static void
sugtk_sym_view_init(SuGtkSymView *self)
{
  self->data_alloc = 0;
  self->data_size = 0;
  self->data_buf = NULL;

  self->autoscroll = TRUE;

  g_signal_connect(
      self,
      "configure-event",
      (GCallback) sugtk_sym_view_on_configure_event,
      NULL);

  g_signal_connect(
      self,
      "draw",
      (GCallback) suscan_constellation_on_draw,
      NULL);

}

GtkWidget *
sugtk_sym_view_new(void)
{
  return (GtkWidget *) g_object_new(SUGTK_TYPE_SYM_VIEW, NULL);
}

