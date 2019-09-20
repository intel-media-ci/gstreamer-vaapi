/*
 *  gstvaapipixmap_priv.h - Pixmap abstraction (private definitions)
 *
 *  Copyright (C) 2013 Intel Corporation
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

#ifndef GST_VAAPI_PIXMAP_PRIV_H
#define GST_VAAPI_PIXMAP_PRIV_H

#include "gstvaapiobject_priv.h"

G_BEGIN_DECLS

/**
 * GST_VAAPI_PIXMAP_FORMAT:
 * @pixmap: a #GstVaapiPixmap
 *
 * Macro that evaluates to the format in pixels of the @pixmap.
 */
#undef  GST_VAAPI_PIXMAP_FORMAT
#define GST_VAAPI_PIXMAP_FORMAT(pixmap) \
    (GST_VAAPI_PIXMAP(pixmap)->format)

/**
 * GST_VAAPI_PIXMAP_WIDTH:
 * @pixmap: a #GstVaapiPixmap
 *
 * Macro that evaluates to the width in pixels of the @pixmap.
 */
#undef  GST_VAAPI_PIXMAP_WIDTH
#define GST_VAAPI_PIXMAP_WIDTH(pixmap) \
    (GST_VAAPI_PIXMAP(pixmap)->width)

/**
 * GST_VAAPI_PIXMAP_HEIGHT:
 * @pixmap: a #GstVaapiPixmap
 *
 * Macro that evaluates to the height in pixels of the @pixmap.
 */
#undef  GST_VAAPI_PIXMAP_HEIGHT
#define GST_VAAPI_PIXMAP_HEIGHT(pixmap) \
    (GST_VAAPI_PIXMAP(pixmap)->height)

/* GstVaapiPixmapClass hooks */
typedef gboolean  (*GstVaapiPixmapCreateFunc)  (GstVaapiPixmap *pixmap);
typedef gboolean  (*GstVaapiPixmapRenderFunc)  (GstVaapiPixmap *pixmap,
    GstVaapiSurface *surface, const GstVaapiRectangle *crop_rect, guint flags);

/**
 * GstVaapiPixmap:
 * @create: virtual function to create a pixmap with width and height
 * @render: virtual function to render a #GstVaapiSurface into a pixmap
 *
 * Base class for system-dependent pixmaps.
 */
struct _GstVaapiPixmap {
    /*< common header >*/
    GstMiniObject mini_object;
    GstVaapiDisplay *display;
    GstVaapiID object_id;

    /*< protected >*/
    GstVaapiPixmapCreateFunc    create;
    GstVaapiPixmapRenderFunc    render;
    GstVideoFormat      format;
    guint               width;
    guint               height;
    guint               use_foreign_pixmap      : 1;
};

G_GNUC_INTERNAL
GstVaapiPixmap *
gst_vaapi_pixmap_init (GstVaapiPixmap * pixmap,
    GstVaapiPixmapCreateFunc create,
    GstVaapiPixmapRenderFunc render,
    GstVaapiDisplay * display, GstVideoFormat format, guint width, guint height);

G_GNUC_INTERNAL
GstVaapiPixmap *
gst_vaapi_pixmap_init_from_native (GstVaapiPixmap * pixmap,
    GstVaapiPixmapCreateFunc create,
    GstVaapiPixmapRenderFunc render,
    GstVaapiDisplay * display, gpointer native_pixmap);

G_END_DECLS

#endif /* GST_VAAPI_PIXMAP_PRIV_H */
