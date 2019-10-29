/*
 *  gstvaapipad.c - GStreamer VA-API Pads
 *
 *  Copyright (C) 2019 Intel Corporation
 *    Author: U. Artie Eoff <ullysses.a.eoff@intel.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#include "gstcompat.h"
#include "gstvaapipad.h"

/* ********************* */
/* GstVaapiPad */
/* ********************* */

G_DEFINE_TYPE (GstVaapiPad, gst_vaapi_pad, GST_TYPE_PAD);

static void
gst_vaapi_pad_finalize (GObject * o)
{
  GstVaapiPad *pad = GST_VAAPI_PAD (o);

  gst_caps_replace (&pad->caps, NULL);
  gst_caps_replace (&pad->allowed_raw_caps, NULL);
  gst_video_info_init (&pad->info);

  g_clear_object (&pad->buffer_pool);
  g_clear_object (&pad->allocator);

  pad->buffer_size = 0;
  pad->caps_is_raw = FALSE;

  G_OBJECT_CLASS (gst_vaapi_pad_parent_class)->finalize (o);
}

static void
gst_vaapi_pad_class_init (GstVaapiPadClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = gst_vaapi_pad_finalize;
}

static void
gst_vaapi_pad_init (GstVaapiPad * pad)
{
  gst_video_info_init (&pad->info);
  pad->caps = NULL;
  pad->allowed_raw_caps = NULL;
  pad->buffer_pool = NULL;
  pad->allocator = NULL;
  pad->buffer_size = 0;
  pad->caps_is_raw = FALSE;
}

/* ************************* */
/* GstVaapiSinkPad */
/* ************************* */

G_DEFINE_TYPE (GstVaapiSinkPad, gst_vaapi_sink_pad, GST_TYPE_VAAPI_PAD);

static void
gst_vaapi_sink_pad_finalize (GObject * o)
{
  G_OBJECT_CLASS (gst_vaapi_sink_pad_parent_class)->finalize (o);
}

static void
gst_vaapi_sink_pad_class_init (GstVaapiSinkPadClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = gst_vaapi_sink_pad_finalize;
}

static void
gst_vaapi_sink_pad_init (GstVaapiSinkPad * pad)
{
  return;
}

/* ************************ */
/* GstVaapiSrcPad */
/* ************************ */

G_DEFINE_TYPE (GstVaapiSrcPad, gst_vaapi_src_pad, GST_TYPE_VAAPI_PAD);

static void
gst_vaapi_src_pad_finalize (GObject * o)
{
  GstVaapiSrcPad *pad = GST_VAAPI_SRC_PAD (o);

  g_clear_object (&pad->other_allocator);
  pad->can_dmabuf = FALSE;

  G_OBJECT_CLASS (gst_vaapi_src_pad_parent_class)->finalize (o);
}

static void
gst_vaapi_src_pad_class_init (GstVaapiSrcPadClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = gst_vaapi_src_pad_finalize;
}

static void
gst_vaapi_src_pad_init (GstVaapiSrcPad * pad)
{
  pad->can_dmabuf = FALSE;
  pad->other_allocator = NULL;
}
