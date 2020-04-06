/*
 *  gstvaapiencode_jpeg.c - VA-API JPEG encoder
 *
 *  Copyright (C) 2015 Intel Corporation
 *    Author: Sreerenj Balachandran <sreerenj.balachandran@intel.com>
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

/**
 * SECTION:element-vaapijpegenc
 * @short_description: A VA-API based JPEG image encoder
 *
 * Encodes raw images into JPEG images.
 *
 * ## Example launch line
 *
 * |[
 *  gst-launch-1.0 -ev videotestsrc num-buffers=1 ! timeoverlay ! vaapijpegenc ! filesink location=test.jpg
 * ]|
 */

#include "gstcompat.h"
#include <gst/vaapi/gstvaapidisplay.h>
#include <gst/vaapi/gstvaapiencoder_jpeg.h>
#include "gstvaapiencode_jpeg.h"
#include "gstvaapipluginutil.h"
#include "gstvaapivideomemory.h"

#define GST_PLUGIN_NAME "vaapijpegenc"
#define GST_PLUGIN_DESC "A VA-API based JPEG video encoder"

GST_DEBUG_CATEGORY_STATIC (gst_vaapi_jpeg_encode_debug);
#define GST_CAT_DEFAULT gst_vaapi_jpeg_encode_debug

#define GST_CODEC_CAPS                          \
  "image/jpeg"

/* *INDENT-OFF* */
static const char gst_vaapiencode_jpeg_src_caps_str[] =
  GST_CODEC_CAPS;
/* *INDENT-ON* */

/* *INDENT-OFF* */
static GstStaticPadTemplate gst_vaapiencode_jpeg_src_factory =
  GST_STATIC_PAD_TEMPLATE ("src",
      GST_PAD_SRC,
      GST_PAD_ALWAYS,
      GST_STATIC_CAPS (gst_vaapiencode_jpeg_src_caps_str));
/* *INDENT-ON* */

/* jpeg encode */
static GType encode_type = G_TYPE_INVALID;
static GstElementClass *parent_class = NULL;

static void
gst_vaapiencode_jpeg_init (GstVaapiEncodeJpeg * encode)
{
  /* nothing to do here */
}

static void
gst_vaapiencode_jpeg_finalize (GObject * object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GstCaps *
gst_vaapiencode_jpeg_get_caps (GstVaapiEncode * base_encode)
{
  GstCaps *caps;

  caps = gst_caps_from_string (GST_CODEC_CAPS);

  return caps;
}

static GstVaapiEncoder *
gst_vaapiencode_jpeg_alloc_encoder (GstVaapiEncode * base,
    GstVaapiDisplay * display)
{
  return gst_vaapi_encoder_jpeg_new (display);
}

static void
gst_vaapiencode_jpeg_class_init (GstVaapiEncodeJpegClass * klass, gpointer data)
{
  GObjectClass *const object_class = G_OBJECT_CLASS (klass);
  GstElementClass *const element_class = GST_ELEMENT_CLASS (klass);
  GstVaapiEncodeClass *const encode_class = GST_VAAPIENCODE_CLASS (klass);
  GstCaps *sink_caps = GST_CAPS_CAST (data);
  gpointer encoder_class;

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gst_vaapiencode_jpeg_finalize;
  object_class->set_property = gst_vaapiencode_set_property_subclass;
  object_class->get_property = gst_vaapiencode_get_property_subclass;

  encode_class->get_caps = gst_vaapiencode_jpeg_get_caps;
  encode_class->alloc_encoder = gst_vaapiencode_jpeg_alloc_encoder;

  gst_element_class_set_static_metadata (element_class,
      "VA-API JPEG encoder",
      "Codec/Encoder/Image/Hardware",
      GST_PLUGIN_DESC,
      "Sreerenj Balachandran <sreerenj.balachandran@intel.com>");

  /* sink pad */
  g_assert (sink_caps);
  gst_element_class_add_pad_template (element_class,
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS, sink_caps));
  gst_caps_unref (sink_caps);

  /* src pad */
  gst_element_class_add_static_pad_template (element_class,
      &gst_vaapiencode_jpeg_src_factory);

  encoder_class = g_type_class_ref (GST_TYPE_VAAPI_ENCODER_JPEG);
  g_assert (encoder_class);
  gst_vaapiencode_class_install_properties (encode_class, encoder_class);
  g_type_class_unref (encoder_class);
}

GType
gst_vaapiencode_jpeg_register_type (GstVaapiDisplay * display)
{
  GstCaps *caps;
  guint i;
  GTypeInfo type_info = {
    sizeof (GstVaapiEncodeClass),
    NULL,
    NULL,
    (GClassInitFunc) gst_vaapiencode_jpeg_class_init,
    NULL,
    NULL,
    sizeof (GstVaapiEncodeJpeg),
    0,
    (GInstanceInitFunc) gst_vaapiencode_jpeg_init,
  };
  GArray *extra_fmts;
  GstVideoFormat fmt;

  GST_DEBUG_CATEGORY_INIT (gst_vaapi_jpeg_encode_debug,
      GST_PLUGIN_NAME, 0, GST_PLUGIN_DESC);

  extra_fmts = g_array_new (FALSE, FALSE, sizeof (GstVideoFormat));
  /* Only add BGRA for jpeg now */
  fmt = GST_VIDEO_FORMAT_BGRA;
  g_array_append_val (extra_fmts, fmt);
  caps = gst_vaapiencode_detect_codec_input_caps (display,
      GST_VAAPI_CODEC_JPEG, extra_fmts);
  g_array_unref (extra_fmts);
  if (!caps) {
    GST_ERROR ("failed to get sink caps for jpeg encode, can not register");
    return G_TYPE_INVALID;
  }

  for (i = 0; i < gst_caps_get_size (caps); i++) {
    GstStructure *structure = gst_caps_get_structure (caps, i);
    if (!structure)
      continue;
    gst_structure_set (structure, "interlace-mode", G_TYPE_STRING,
        "progressive", NULL);
  }
  GST_DEBUG ("jpeg encode's sink caps %" GST_PTR_FORMAT, caps);

  type_info.class_data = caps;
  encode_type = g_type_register_static (GST_TYPE_VAAPIENCODE,
      "GstVaapiEncodeJpeg", &type_info, 0);

  return encode_type;
}

GType
gst_vaapiencode_jpeg_get_type (void)
{
  g_assert (encode_type != G_TYPE_INVALID);
  return encode_type;
}
