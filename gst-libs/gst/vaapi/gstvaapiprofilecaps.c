/*
 *  gstvaapiprofilecaps.h - VA config attributes as gstreamer capabilities
 *
 *  Copyright (C) 2019 Igalia, S.L.
 *    Author: Víctor Jáquez <vjaquez@igalia.com>
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
 * SECTION:gstvaapiprofilecaps
 * @short_description: VA config attributes as gstreamer capabilities
 */

#include "sysdeps.h"
#include "gstvaapicompat.h"
#include "gstvaapicontext.h"
#include "gstvaapiprofilecaps.h"
#include "gstvaapisurface.h"
#include "gstvaapiutils.h"

static gboolean
init_context_info (GstVaapiDisplay * display, GstVaapiContextInfo * cip)
{
  guint value = 0;

  /* XXX: Only try a context from he first RTFormat in config. */
  if (!gst_vaapi_get_config_attribute (display,
          gst_vaapi_profile_get_va_profile (cip->profile),
          gst_vaapi_entrypoint_get_va_entrypoint (cip->entrypoint),
          VAConfigAttribRTFormat, &value)) {
    return FALSE;
  }

  cip->chroma_type = to_GstVaapiChromaType (value);
  return cip->chroma_type != 0;
}

static GstVaapiContext *
create_context (GstVaapiDisplay * display, GstVaapiContextInfo * cip)
{
  if (!init_context_info (display, cip))
    return NULL;
  return gst_vaapi_context_new (display, cip);
}

static gboolean
append_caps (GstVaapiContext * context, GstStructure * structure)
{
  GstVaapiConfigSurfaceAttributes attribs = { 0, };

  if (!gst_vaapi_context_get_surface_attributes (context, &attribs))
    return FALSE;

  if (attribs.min_width >= attribs.max_width ||
      attribs.min_height >= attribs.max_height)
    return FALSE;

  gst_structure_set (structure, "width", GST_TYPE_INT_RANGE, attribs.min_width,
      attribs.max_width, "height", GST_TYPE_INT_RANGE, attribs.min_height,
      attribs.max_height, NULL);

  return TRUE;
}

static gboolean
append_caps_with_context_info (GstVaapiDisplay * display,
    GstVaapiContextInfo * cip, GstStructure * structure)
{
  GstVaapiContext *context;
  gboolean ret;

  context = create_context (display, cip);
  if (!context)
    return FALSE;

  ret = append_caps (context, structure);
  gst_vaapi_context_unref (context);
  return ret;
}

/**
 * gst_vaapi_decoder_add_profile_caps:
 * @display: a #GstVaapiDisplay
 * @profile: a #GstVaapiProfile
 * @structure: a #GstStructure
 *
 * Extracts the config's surface attributes, from @profile, in a
 * decoder context, and transforms it into a caps formats and appended
 * into @structure.
 *
 * Returns: %TRUE if the capabilities could be extracted and appended
 * into @structure; otherwise %FALSE
 **/
gboolean
gst_vaapi_profile_caps_append_decoder (GstVaapiDisplay * display,
    GstVaapiProfile profile, GstStructure * structure)
{
  GstVaapiContextInfo cip = {
    GST_VAAPI_CONTEXT_USAGE_DECODE, profile, GST_VAAPI_ENTRYPOINT_VLD, 0,
  };

  g_return_val_if_fail (display != NULL, FALSE);
  g_return_val_if_fail (structure != NULL, FALSE);

  return append_caps_with_context_info (display, &cip, structure);
}

/**
 * gst_vaapi_mem_type_supports:
 * @va_mem_types:  memory types from VA surface attributes
 * @mem_type: the #GstVaapiBufferMemoryType to test
 *
 * Test if @va_mem_types handles @mem_type
 *
 * Returns: %TRUE if @mem_type is supported in @va_mem_types;
 *    otherwise %FALSE
 **/
gboolean
gst_vaapi_mem_type_supports (guint va_mem_types, guint mem_type)
{
  return ((va_mem_types & from_GstVaapiBufferMemoryType (mem_type)) != 0);
}

/**
 * gst_vaapi_chroma_type_from_caps:
 * @caps: the #GstCaps where to extract the chromatype
 *
 * Try to extract the #GstVaapiChromaType from @caps
 *
 * Returns: the #GstVaapiChromaType from @caps,
 *   %GST_VAAPI_CHROMA_TYPE_YUV420 if they are not specified,
 *   or 0 if caps are inconsistent or undefined.
 **/
guint
gst_vaapi_chroma_type_from_caps (const GstCaps * caps)
{
  GstStructure *st = gst_caps_get_structure (caps, 0);
  const gchar *chroma_format;
  guint depth = 0;

  if (!gst_structure_has_field (st, "chroma-format"))
    return GST_VAAPI_CHROMA_TYPE_YUV420;

  chroma_format = gst_structure_get_string (st, "chroma-format");

  if (!gst_structure_get_uint (st, "bit-depth-chroma", &depth))
    depth = 8;

  if (g_strcmp0 (chroma_format, "4:0:0") == 0) {
    return GST_VAAPI_CHROMA_TYPE_YUV400;
  } else if (g_strcmp0 (chroma_format, "4:2:0") == 0) {
    if (depth == 8)
      return GST_VAAPI_CHROMA_TYPE_YUV420;
    if (depth == 10)
      return GST_VAAPI_CHROMA_TYPE_YUV420_10BPP;
    else if (depth == 12)
      return GST_VAAPI_CHROMA_TYPE_YUV420_12BPP;
  } else if (g_strcmp0 (chroma_format, "4:2:2") == 0) {
    if (depth == 8)
      return GST_VAAPI_CHROMA_TYPE_YUV422;
    else if (depth == 10)
      return GST_VAAPI_CHROMA_TYPE_YUV422_10BPP;
    else if (depth == 12)
      return GST_VAAPI_CHROMA_TYPE_YUV422_12BPP;
  } else if (g_strcmp0 (chroma_format, "4:4:4") == 0) {
    if (depth == 8)
      return GST_VAAPI_CHROMA_TYPE_YUV444;
    else if (depth == 10)
      return GST_VAAPI_CHROMA_TYPE_YUV444_10BPP;
    else if (depth == 12)
      return GST_VAAPI_CHROMA_TYPE_YUV444_12BPP;
  }

  return 0;
}
