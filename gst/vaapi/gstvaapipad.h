/*
 *  gstvaapipad.h - GStreamer VA-API Pads
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

#ifndef GST_VAAPI_PAD_H
#define GST_VAAPI_PAD_H

#include <gst/gst.h>
#include <gst/video/video-info.h>

G_BEGIN_DECLS

/* ********************* */
/* GstVaapiPad */
/* ********************* */

#define GST_TYPE_VAAPI_PAD \
  (gst_vaapi_pad_get_type())
#define GST_VAAPI_PAD(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VAAPI_PAD,GstVaapiPad))
#define GST_VAAPI_PAD_CAST(obj) \
  ((GstVaapiPad *)(obj))
#define GST_VAAPI_PAD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VAAPI_PAD,GstVaapiPadClass))
#define GST_VAAPI_PAD_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),GST_TYPE_VAAPI_PAD,GstVaapiPadClass))

/* Convenience Macros */
#define GST_VAAPI_PAD_CAPS(pad) \
  (GST_VAAPI_PAD(pad)->caps)
#define GST_VAAPI_PAD_INFO(pad) \
  (&GST_VAAPI_PAD(pad)->info)
#define GST_VAAPI_PAD_BUFFER_POOL(pad) \
  (GST_VAAPI_PAD(pad)->buffer_pool)
#define GST_VAAPI_PAD_ALLOCATOR(pad) \
  (GST_VAAPI_PAD(pad)->allocator)
#define GST_VAAPI_PAD_BUFFER_SIZE(pad) \
  (GST_VAAPI_PAD(pad)->buffer_size)
#define GST_VAAPI_PAD_ALLOWED_RAW_CAPS(pad) \
  (GST_VAAPI_PAD(pad)->allowed_raw_caps)
#define GST_VAAPI_PAD_CAPS_IS_RAW(pad) \
  (GST_VAAPI_PAD(pad)->caps_is_raw)

typedef struct _GstVaapiPad GstVaapiPad;
typedef struct _GstVaapiPadClass GstVaapiPadClass;

struct _GstVaapiPad
{
  union {
    GstPad pad;
  } parent_instance;

  GstCaps *caps;
  GstCaps *allowed_raw_caps;
  GstVideoInfo info;
  GstBufferPool *buffer_pool;
  GstAllocator *allocator;
  guint buffer_size;
  gboolean caps_is_raw;
};

struct _GstVaapiPadClass {
  union {
    GstPadClass pad_class;
  } parent_class;
};

GType
gst_vaapi_pad_get_type (void) G_GNUC_CONST;


/* ************************* */
/* GstVaapiSinkPad */
/* ************************* */

#define GST_TYPE_VAAPI_SINK_PAD \
  (gst_vaapi_sink_pad_get_type())
#define GST_VAAPI_SINK_PAD(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VAAPI_SINK_PAD,GstVaapiSinkPad))
#define GST_VAAPI_SINK_PAD_CAST(obj) \
  ((GstVaapiSinkPad *)(obj))
#define GST_VAAPI_SINK_PAD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VAAPI_SINK_PAD, \
      GstVaapiSinkPadClass))
#define GST_VAAPI_SINK_PAD_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),GST_TYPE_VAAPI_SINK_PAD, \
      GstVaapiSinkPadClass))

typedef struct _GstVaapiSinkPad GstVaapiSinkPad;
typedef struct _GstVaapiSinkPadClass GstVaapiSinkPadClass;

struct _GstVaapiSinkPad
{
  union {
    GstVaapiPad pad;
  } parent_instance;
};

struct _GstVaapiSinkPadClass
{
  union {
    GstVaapiPadClass pad_class;
  } parent_class;
};

GType
gst_vaapi_sink_pad_get_type (void) G_GNUC_CONST;


/* ************************ */
/* GstVaapiSrcPad */
/* ************************ */

#define GST_TYPE_VAAPI_SRC_PAD \
  (gst_vaapi_src_pad_get_type())
#define GST_VAAPI_SRC_PAD(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VAAPI_SRC_PAD,GstVaapiSrcPad))
#define GST_VAAPI_SRC_PAD_CAST(obj) \
  ((GstVaapiSrcPad *)(obj))
#define GST_VAAPI_SRC_PAD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VAAPI_SRC_PAD,GstVaapiSrcPadClass))
#define GST_VAAPI_SRC_PAD_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),GST_TYPE_VAAPI_SRC_PAD,GstVaapiSrcPadClass))

typedef struct _GstVaapiSrcPad GstVaapiSrcPad;
typedef struct _GstVaapiSrcPadClass GstVaapiSrcPadClass;

struct _GstVaapiSrcPad
{
  GstVaapiPad parent_instance;

  gboolean can_dmabuf;

  GstAllocator *other_allocator;
  GstAllocationParams other_allocator_params;
};

struct _GstVaapiSrcPadClass
{
  GstVaapiPadClass parent_class;
};

GType
gst_vaapi_src_pad_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* GST_VAAPI_PAD_H */
