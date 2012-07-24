/*
 *  gstvaapipluginbuffer.c - Private GStreamer/VA video buffers
 *
 *  Copyright (C) 2012 Intel Corporation
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gst/vaapi/gstvaapivideobuffer.h>
#include <gst/vaapi/gstvaapivideobuffer_priv.h>
#if USE_GLX
# include <gst/vaapi/gstvaapivideobuffer_glx.h>
#endif
#include "gstvaapipluginbuffer.h"

static inline GType
get_type(GstVaapiDisplay *display)
{
#if USE_GLX
    if (GST_VAAPI_IS_DISPLAY_GLX(display))
        return GST_VAAPI_TYPE_VIDEO_BUFFER_GLX;
#endif
    return GST_VAAPI_TYPE_VIDEO_BUFFER;
}

GstBuffer *
gst_vaapi_video_buffer_new(GstVaapiDisplay *display)
{
    g_return_val_if_fail(GST_VAAPI_IS_DISPLAY(display), NULL);

    return gst_vaapi_video_buffer_typed_new(get_type(display), display);
}

GstBuffer *
gst_vaapi_video_buffer_new_from_pool(GstVaapiVideoPool *pool)
{
    GstVaapiDisplay *display;

    g_return_val_if_fail(GST_VAAPI_IS_VIDEO_POOL(pool), NULL);

    display = gst_vaapi_video_pool_get_display(pool);
    if (!display)
        return NULL;
    return gst_vaapi_video_buffer_typed_new_from_pool(get_type(display), pool);
}

GstBuffer *
gst_vaapi_video_buffer_new_from_buffer(GstBuffer *buffer)
{
    GstVaapiVideoBuffer *vbuffer;
    GstVaapiDisplay *display;

    g_return_val_if_fail(GST_VAAPI_IS_VIDEO_BUFFER(buffer), NULL);

    vbuffer = GST_VAAPI_VIDEO_BUFFER(buffer);
    display = gst_vaapi_video_buffer_get_display(vbuffer);
    if (!display)
        return NULL;

    return gst_vaapi_video_buffer_typed_new_from_buffer(
        get_type(display), buffer);
}

GstBuffer *
gst_vaapi_video_buffer_new_with_image(GstVaapiImage *image)
{
    GstVaapiDisplay *display;

    g_return_val_if_fail(GST_VAAPI_IS_IMAGE(image), NULL);

    display = gst_vaapi_object_get_display(GST_VAAPI_OBJECT(image));
    if (!display)
        return NULL;

    return gst_vaapi_video_buffer_typed_new_with_image(
        get_type(display), image);
}

GstBuffer *
gst_vaapi_video_buffer_new_with_surface(GstVaapiSurface *surface)
{
    GstVaapiDisplay *display;

    g_return_val_if_fail(GST_VAAPI_IS_SURFACE(surface), NULL);

    display = gst_vaapi_object_get_display(GST_VAAPI_OBJECT(surface));
    if (!display)
        return NULL;

    return gst_vaapi_video_buffer_typed_new_with_surface(
        get_type(display), surface);
}

GstBuffer *
gst_vaapi_video_buffer_new_with_surface_proxy(GstVaapiSurfaceProxy *proxy)
{
    GstVaapiDisplay *display;

    g_return_val_if_fail(GST_VAAPI_IS_SURFACE_PROXY(proxy), NULL);

    display = gst_vaapi_object_get_display(GST_VAAPI_OBJECT(proxy));
    if (!display)
        return NULL;

    return gst_vaapi_video_buffer_typed_new_with_surface_proxy(
        get_type(display), proxy);
}
