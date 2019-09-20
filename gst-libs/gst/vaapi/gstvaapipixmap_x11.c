/*
 *  gstvaapipixmap_x11.c - X11 pixmap abstraction
 *
 *  Copyright (C) 2013-2014 Intel Corporation
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
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
 * SECTION:gstvaapipixmap_x11
 * @short_description: X11 pixmap abstraction
 */

#include "sysdeps.h"
#include "gstvaapicompat.h"
#include "gstvaapipixmap_x11.h"
#include "gstvaapipixmap_priv.h"
#include "gstvaapidisplay_x11.h"
#include "gstvaapidisplay_x11_priv.h"
#include "gstvaapiutils.h"
#include "gstvaapiutils_x11.h"
#include "gstvaapisurface_priv.h"

#define DEBUG 1
#include "gstvaapidebug.h"

struct _GstVaapiPixmapX11
{
  GstVaapiPixmap parent_instance;
};

static gboolean
gst_vaapi_pixmap_x11_create_from_xid (GstVaapiPixmap * pixmap, Pixmap xid)
{
  guint depth;
  gboolean success;

  if (!xid)
    return FALSE;

  GST_VAAPI_OBJECT_LOCK_DISPLAY (pixmap);
  success = x11_get_geometry (GST_VAAPI_OBJECT_NATIVE_DISPLAY (pixmap), xid,
      NULL, NULL, &pixmap->width, &pixmap->height, &depth);
  GST_VAAPI_OBJECT_UNLOCK_DISPLAY (pixmap);
  if (!success)
    return FALSE;

  pixmap->format =
      gst_vaapi_display_x11_get_pixmap_format (GST_VAAPI_OBJECT_DISPLAY_X11
      (pixmap), depth);
  if (pixmap->format == GST_VIDEO_FORMAT_UNKNOWN)
    return FALSE;
  return TRUE;
}

static gboolean
gst_vaapi_pixmap_x11_create (GstVaapiPixmap * pixmap)
{
  GstVaapiDisplayX11 *const display =
      GST_VAAPI_DISPLAY_X11 (GST_VAAPI_OBJECT_DISPLAY (pixmap));
  Display *const dpy = GST_VAAPI_OBJECT_NATIVE_DISPLAY (display);
  Window rootwin;
  Pixmap xid;
  guint depth;

  if (pixmap->use_foreign_pixmap)
    return gst_vaapi_pixmap_x11_create_from_xid (pixmap,
        GST_VAAPI_OBJECT_ID (pixmap));

  depth = gst_vaapi_display_x11_get_pixmap_depth (display, pixmap->format);
  if (!depth)
    return FALSE;

  GST_VAAPI_OBJECT_LOCK_DISPLAY (pixmap);
  rootwin = RootWindow (dpy, DefaultScreen (dpy));
  xid = XCreatePixmap (dpy, rootwin, pixmap->width, pixmap->height, depth);
  GST_VAAPI_OBJECT_UNLOCK_DISPLAY (pixmap);

  GST_DEBUG ("xid %" GST_VAAPI_ID_FORMAT, GST_VAAPI_ID_ARGS (xid));
  GST_VAAPI_OBJECT_ID (pixmap) = xid;
  return xid != None;
}

static void
gst_vaapi_pixmap_x11_free (GstVaapiPixmap * pixmap)
{
  const Pixmap xid = GST_VAAPI_OBJECT_ID (pixmap);

  if (xid) {
    if (!pixmap->use_foreign_pixmap) {
      GST_VAAPI_OBJECT_LOCK_DISPLAY (pixmap);
      XFreePixmap (GST_VAAPI_OBJECT_NATIVE_DISPLAY (pixmap), xid);
      GST_VAAPI_OBJECT_UNLOCK_DISPLAY (pixmap);
    }
    GST_VAAPI_OBJECT_ID (pixmap) = None;
  }

  gst_vaapi_display_replace (&pixmap->display, NULL);

  g_slice_free1 (sizeof (GstVaapiPixmapX11), pixmap);
}

static gboolean
gst_vaapi_pixmap_x11_render (GstVaapiPixmap * pixmap, GstVaapiSurface * surface,
    const GstVaapiRectangle * crop_rect, guint flags)
{
  VASurfaceID surface_id;
  VAStatus status;

  surface_id = GST_VAAPI_OBJECT_ID (surface);
  if (surface_id == VA_INVALID_ID)
    return FALSE;

  GST_VAAPI_OBJECT_LOCK_DISPLAY (pixmap);
  status = vaPutSurface (GST_VAAPI_OBJECT_VADISPLAY (pixmap),
      surface_id,
      GST_VAAPI_OBJECT_ID (pixmap),
      crop_rect->x, crop_rect->y,
      crop_rect->width, crop_rect->height,
      0, 0,
      GST_VAAPI_PIXMAP_WIDTH (pixmap),
      GST_VAAPI_PIXMAP_HEIGHT (pixmap),
      NULL, 0, from_GstVaapiSurfaceRenderFlags (flags)
      );
  GST_VAAPI_OBJECT_UNLOCK_DISPLAY (pixmap);
  if (!vaapi_check_status (status, "vaPutSurface() [pixmap]"))
    return FALSE;
  return TRUE;
}

GType gst_vaapi_pixmap_x11_get_type (void);
GST_DEFINE_MINI_OBJECT_TYPE (GstVaapiPixmapX11, gst_vaapi_pixmap_x11);

/**
 * gst_vaapi_pixmap_x11_new:
 * @display: a #GstVaapiDisplay
 * @format: the requested pixmap format
 * @width: the requested pixmap width, in pixels
 * @height: the requested windo height, in pixels
 *
 * Creates a pixmap with the specified @format, @width and
 * @height. The pixmap will be attached to the @display.
 *
 * Return value: the newly allocated #GstVaapiPixmap object
 */
GstVaapiPixmap *
gst_vaapi_pixmap_x11_new (GstVaapiDisplay * display,
    GstVideoFormat format, guint width, guint height)
{
  GstVaapiPixmapX11 *pixmap_x11;

  GST_DEBUG ("new pixmap, format %s, size %ux%u",
      gst_vaapi_video_format_to_string (format), width, height);

  g_return_val_if_fail (GST_VAAPI_IS_DISPLAY_X11 (display), NULL);

  pixmap_x11 = g_slice_new0 (GstVaapiPixmapX11);
  if (!pixmap_x11)
    return NULL;

  gst_mini_object_init (GST_MINI_OBJECT_CAST (pixmap_x11), 0,
      gst_vaapi_pixmap_x11_get_type (), NULL, NULL,
      (GstMiniObjectFreeFunction) gst_vaapi_pixmap_x11_free);

  return gst_vaapi_pixmap_init ((GstVaapiPixmap *) pixmap_x11,
      gst_vaapi_pixmap_x11_create, gst_vaapi_pixmap_x11_render,
      display, format, width, height);
}

/**
 * gst_vaapi_pixmap_x11_new_with_xid:
 * @display: a #GstVaapiDisplay
 * @xid: an X11 #Pixmap id
 *
 * Creates a #GstVaapiPixmap using the X11 Pixmap @xid. The caller
 * still owns the pixmap and must call XFreePixmap() when all
 * #GstVaapiPixmap references are released. Doing so too early can
 * yield undefined behaviour.
 *
 * Return value: the newly allocated #GstVaapiPixmap object
 */
GstVaapiPixmap *
gst_vaapi_pixmap_x11_new_with_xid (GstVaapiDisplay * display, Pixmap xid)
{
  GstVaapiPixmapX11 *pixmap_x11;

  GST_DEBUG ("new pixmap from xid 0x%08x", (guint) xid);

  g_return_val_if_fail (GST_VAAPI_IS_DISPLAY_X11 (display), NULL);
  g_return_val_if_fail (xid != None, NULL);

  pixmap_x11 = g_slice_new0 (GstVaapiPixmapX11);
  if (!pixmap_x11)
    return NULL;

  gst_mini_object_init (GST_MINI_OBJECT_CAST (pixmap_x11), 0,
      gst_vaapi_pixmap_x11_get_type (), NULL, NULL,
      (GstMiniObjectFreeFunction) gst_vaapi_pixmap_x11_free);

  return gst_vaapi_pixmap_init_from_native ((GstVaapiPixmap *) pixmap_x11,
      gst_vaapi_pixmap_x11_create, gst_vaapi_pixmap_x11_render,
      display, GSIZE_TO_POINTER (xid));
}

/**
 * gst_vaapi_pixmap_x11_get_xid:
 * @pixmap: a #GstVaapiPixmapX11
 *
 * Returns the underlying X11 Pixmap that was created by
 * gst_vaapi_pixmap_x11_new() or that was bound with
 * gst_vaapi_pixmap_x11_new_with_xid().
 *
 * Return value: the underlying X11 Pixmap bound to @pixmap.
 */
Pixmap
gst_vaapi_pixmap_x11_get_xid (GstVaapiPixmapX11 * pixmap)
{
  g_return_val_if_fail (pixmap != NULL, None);

  return GST_VAAPI_OBJECT_ID (pixmap);
}

/**
 * gst_vaapi_pixmap_x11_is_foreign_xid:
 * @pixmap: a #GstVaapiPixmapX11
 *
 * Checks whether the @pixmap XID was created by gst_vaapi_pixmap_x11_new()
 * or was bound with gst_vaapi_pixmap_x11_new_with_xid().
 *
 * Return value: %TRUE if the underlying X pixmap is owned by the
 *   caller (foreign pixmap)
 */
gboolean
gst_vaapi_pixmap_x11_is_foreign_xid (GstVaapiPixmapX11 * pixmap)
{
  g_return_val_if_fail (pixmap != NULL, FALSE);

  return GST_VAAPI_PIXMAP (pixmap)->use_foreign_pixmap;
}
