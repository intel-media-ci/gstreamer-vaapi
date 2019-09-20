/*
 *  gstvaapiobject.h - Base VA object
 *
 *  Copyright (C) 2010-2011 Splitted-Desktop Systems
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@splitted-desktop.com>
 *  Copyright (C) 2012-2013 Intel Corporation
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

#ifndef GST_VAAPI_OBJECT_H
#define GST_VAAPI_OBJECT_H

#include <gst/vaapi/gstvaapitypes.h>
#include <gst/vaapi/gstvaapidisplay.h>

G_BEGIN_DECLS

#define GST_VAAPI_OBJECT(obj) \
  ((GstVaapiObject *) (obj))

typedef struct _GstVaapiObject GstVaapiObject;

/**
 * GstVaapiObject:
 *
 * All VA object common header.
 */
struct _GstVaapiObject
{
  GstMiniObject mini_object;
  GstVaapiDisplay *display;
  GstVaapiID object_id;
};

static inline gpointer
gst_vaapi_object_ref (gpointer object)
{
  return gst_mini_object_ref ((GstMiniObject *) object);
}

static inline void
gst_vaapi_object_unref (gpointer object)
{
  gst_mini_object_unref ((GstMiniObject *) object);
}

static inline void
gst_vaapi_object_replace (gpointer old_object_ptr, gpointer new_object)
{
  gst_mini_object_replace ((GstMiniObject **) old_object_ptr,
      (GstMiniObject *) new_object);
}

static inline GstVaapiDisplay *
gst_vaapi_object_get_display (GstVaapiObject * object)
{
  if (object)
    return object->display;
  return NULL;
}

G_END_DECLS
#endif /* GST_VAAPI_OBJECT_H */
