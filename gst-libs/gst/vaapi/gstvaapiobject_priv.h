/*
 *  gstvaapiobject_priv.h - Base VA object (private definitions)
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

#ifndef GST_VAAPI_OBJECT_PRIV_H
#define GST_VAAPI_OBJECT_PRIV_H

#include <gst/vaapi/gstvaapiobject.h>
#include "gstvaapiminiobject.h"
#include "gstvaapidisplay_priv.h"

G_BEGIN_DECLS

#define GST_VAAPI_OBJECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_VAAPI_OBJECT, GstVaapiObjectClass))
#define GST_VAAPI_IS_OBJECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_VAAPI_OBJECT))
#define GST_VAAPI_OBJECT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_VAAPI_OBJECT, GstVaapiObjectClass))

typedef struct _GstVaapiObjectClass GstVaapiObjectClass;

typedef void (*GstVaapiObjectFinalizeFunc) (GObject * object);

#define GST_VAAPI_OBJECT_DEFINE_CLASS(TN, t_n)                  \
  static void G_PASTE(t_n, _finalize) (TN * obj);               \
  static void                                                   \
  G_PASTE(t_n, _class_init) (G_PASTE(TN, Class) * klass) {      \
    GObjectClass *const object_class = G_OBJECT_CLASS (klass);  \
    object_class->finalize = (GstVaapiObjectFinalizeFunc)       \
                             G_PASTE(t_n, _finalize);           \
  }                                                             \
  GType G_PASTE(t_n, _get_type) (void) G_GNUC_CONST;            \
  static void G_PASTE(t_n, _init) (TN * obj) { }                \
  G_DEFINE_TYPE (TN, t_n, GST_TYPE_VAAPI_OBJECT);

/**
 * GST_VAAPI_OBJECT_ID:
 * @object: a #GstVaapiObject
 *
 * Macro that evaluates to the #GstVaapiID contained in @object.
 * This is an internal macro that does not do any run-time type checks.
 */
#define GST_VAAPI_OBJECT_ID(object) \
  (GST_VAAPI_OBJECT (object)->object_id)

/**
 * GST_VAAPI_OBJECT_DISPLAY:
 * @object: a #GstVaapiObject
 *
 * Macro that evaluates to the #GstVaapiDisplay the @object is bound to.
 * This is an internal macro that does not do any run-time type check.
 */
#define GST_VAAPI_OBJECT_DISPLAY(object) \
  (GST_VAAPI_OBJECT (object)->display)

/**
 * GST_VAAPI_OBJECT_DISPLAY_X11:
 * @object: a #GstVaapiObject
 *
 * Macro that evaluates to the #GstVaapiDisplayX11 the @object is bound to.
 * This is an internal macro that does not do any run-time type check
 * and requires #include "gstvaapidisplay_x11_priv.h"
 */
#define GST_VAAPI_OBJECT_DISPLAY_X11(object) \
  GST_VAAPI_DISPLAY_X11_CAST (GST_VAAPI_OBJECT_DISPLAY (object))

/**
 * GST_VAAPI_OBJECT_DISPLAY_GLX:
 * @object: a #GstVaapiObject
 *
 * Macro that evaluates to the #GstVaapiDisplayGLX the @object is bound to.
 * This is an internal macro that does not do any run-time type check
 * and requires #include "gstvaapidisplay_glx_priv.h".
 */
#define GST_VAAPI_OBJECT_DISPLAY_GLX(object) \
  GST_VAAPI_DISPLAY_GLX_CAST (GST_VAAPI_OBJECT_DISPLAY (object))

/**
 * GST_VAAPI_OBJECT_DISPLAY_WAYLAND:
 * @object: a #GstVaapiObject
 *
 * Macro that evaluates to the #GstVaapiDisplayWayland the @object is
 * bound to.  This is an internal macro that does not do any run-time
 * type check and requires #include "gstvaapidisplay_wayland_priv.h"
 */
#define GST_VAAPI_OBJECT_DISPLAY_WAYLAND(object) \
  GST_VAAPI_DISPLAY_WAYLAND_CAST (GST_VAAPI_OBJECT_DISPLAY (object))

/**
 * GST_VAAPI_OBJECT_VADISPLAY:
 * @object: a #GstVaapiObject
 *
 * Macro that evaluates to the #VADisplay of @display.
 * This is an internal macro that does not do any run-time type check
 * and requires #include "gstvaapidisplay_priv.h".
 */
#define GST_VAAPI_OBJECT_VADISPLAY(object) \
  GST_VAAPI_DISPLAY_VADISPLAY (GST_VAAPI_OBJECT_DISPLAY (object))

/**
 * GST_VAAPI_OBJECT_NATIVE_DISPLAY:
 * @object: a #GstVaapiObject
 *
 * Macro that evaluates to the underlying native @display object.
 * This is an internal macro that does not do any run-time type check.
 */
#define GST_VAAPI_OBJECT_NATIVE_DISPLAY(object) \
  GST_VAAPI_DISPLAY_NATIVE (GST_VAAPI_OBJECT_DISPLAY (object))

/**
 * GST_VAAPI_OBJECT_LOCK_DISPLAY:
 * @object: a #GstVaapiObject
 *
 * Macro that locks the #GstVaapiDisplay contained in the @object.
 * This is an internal macro that does not do any run-time type check.
 */
#define GST_VAAPI_OBJECT_LOCK_DISPLAY(object) \
  GST_VAAPI_DISPLAY_LOCK (GST_VAAPI_OBJECT_DISPLAY (object))

/**
 * GST_VAAPI_OBJECT_UNLOCK_DISPLAY:
 * @object: a #GstVaapiObject
 *
 * Macro that unlocks the #GstVaapiDisplay contained in the @object.
 * This is an internal macro that does not do any run-time type check.
 */
#define GST_VAAPI_OBJECT_UNLOCK_DISPLAY(object) \
  GST_VAAPI_DISPLAY_UNLOCK (GST_VAAPI_OBJECT_DISPLAY (object))

/**
 * GstVaapiObject:
 *
 * VA object base.
 */
struct _GstVaapiObject
{
  /*< private >*/
  GstObject parent_instance;

  GstVaapiDisplay *display;
  GstVaapiID object_id;
};

/**
 * GstVaapiObjectClass:
 *
 * VA object base class.
 */
struct _GstVaapiObjectClass
{
  /*< private >*/
  GstObjectClass parent_class;
};

gpointer
gst_vaapi_object_new (GType type, GstVaapiDisplay * display);

G_END_DECLS

#endif /* GST_VAAPI_OBJECT_PRIV_H */
