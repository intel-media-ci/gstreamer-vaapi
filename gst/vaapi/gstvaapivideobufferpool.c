/*
 *  gstvaapivideobufferpool.c - Gstreamer/VA video buffer pool
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

#include "gstcompat.h"
#include "gstvaapivideobufferpool.h"
#include "gstvaapivideobuffer.h"
#include "gstvaapivideomemory.h"
#include "gstvaapipluginutil.h"
#if (USE_GLX || USE_EGL)
#include "gstvaapivideometa_texture.h"
#endif

GST_DEBUG_CATEGORY_STATIC (gst_debug_vaapivideopool);
#define GST_CAT_DEFAULT gst_debug_vaapivideopool

enum
{
  PROP_0,

  PROP_DISPLAY,
};

struct _GstVaapiVideoBufferPoolPrivate
{
  GstAllocator *allocator;
  GstVideoInfo vmeta_vinfo;
  GstVaapiDisplay *display;
  guint options;
  guint use_dmabuf_memory:1;
  guint forced_video_meta:1;
  /* Map between surface and GstMemory, only DMA */
  GHashTable *dma_mem_map;
};

G_DEFINE_TYPE_WITH_PRIVATE (GstVaapiVideoBufferPool,
    gst_vaapi_video_buffer_pool, GST_TYPE_BUFFER_POOL);

static void
gst_vaapi_video_buffer_pool_finalize (GObject * object)
{
  GstVaapiVideoBufferPoolPrivate *const priv =
      GST_VAAPI_VIDEO_BUFFER_POOL (object)->priv;

  gst_vaapi_display_replace (&priv->display, NULL);
  g_clear_object (&priv->allocator);
  if (priv->dma_mem_map)
    g_hash_table_destroy (priv->dma_mem_map);

  G_OBJECT_CLASS (gst_vaapi_video_buffer_pool_parent_class)->finalize (object);
}

static void
gst_vaapi_video_buffer_pool_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstVaapiVideoBufferPoolPrivate *const priv =
      GST_VAAPI_VIDEO_BUFFER_POOL (object)->priv;

  switch (prop_id) {
    case PROP_DISPLAY:
      priv->display = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_vaapi_video_buffer_pool_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstVaapiVideoBufferPoolPrivate *const priv =
      GST_VAAPI_VIDEO_BUFFER_POOL (object)->priv;

  switch (prop_id) {
    case PROP_DISPLAY:
      g_value_set_pointer (value, priv->display);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
fill_video_alignment (GstVaapiVideoBufferPool * pool, GstVideoAlignment * align)
{
  GstVideoInfo *const vip = &pool->priv->vmeta_vinfo;
  guint i;
  gint nth_bit;

  gst_video_alignment_reset (align);
  for (i = 0; i < GST_VIDEO_INFO_N_PLANES (vip); i++) {
    nth_bit = g_bit_nth_lsf (GST_VIDEO_INFO_PLANE_STRIDE (vip, i), 0);
    if (nth_bit >= 0)
      align->stride_align[i] = (1U << nth_bit) - 1;
  }
}

static const gchar **
gst_vaapi_video_buffer_pool_get_options (GstBufferPool * pool)
{
  static const gchar *g_options[] = {
    GST_BUFFER_POOL_OPTION_VIDEO_META,
    GST_BUFFER_POOL_OPTION_VAAPI_VIDEO_META,
    GST_BUFFER_POOL_OPTION_VIDEO_GL_TEXTURE_UPLOAD_META,
    GST_BUFFER_POOL_OPTION_VIDEO_ALIGNMENT,
    NULL,
  };

  return g_options;
}

static gboolean
gst_vaapi_video_buffer_pool_set_config (GstBufferPool * pool,
    GstStructure * config)
{
  GstVaapiVideoBufferPool *const base_pool = GST_VAAPI_VIDEO_BUFFER_POOL (pool);
  GstVaapiVideoBufferPoolPrivate *const priv = base_pool->priv;
  GstCaps *caps;
  GstVideoInfo new_allocation_vinfo;
  const GstVideoInfo *allocator_vinfo;
  const GstVideoInfo *negotiated_vinfo;
  GstVideoAlignment align;
  GstAllocator *allocator;
  gboolean ret;
  guint size, min_buffers, max_buffers;
  guint surface_alloc_flags;

  GST_DEBUG_OBJECT (base_pool, "config %" GST_PTR_FORMAT, config);

  caps = NULL;
  if (!gst_buffer_pool_config_get_params (config, &caps, &size, &min_buffers,
          &max_buffers))
    goto error_invalid_config;
  if (!caps)
    goto error_no_caps;
  if (!gst_video_info_from_caps (&new_allocation_vinfo, caps))
    goto error_invalid_caps;

  if (!gst_buffer_pool_config_has_option (config,
          GST_BUFFER_POOL_OPTION_VAAPI_VIDEO_META))
    goto error_no_vaapi_video_meta_option;

  allocator = NULL;
  if (!gst_buffer_pool_config_get_allocator (config, &allocator, NULL))
    goto error_invalid_allocator;

  /* it is a valid allocator? */
  if (allocator
      && (g_strcmp0 (allocator->mem_type, GST_VAAPI_VIDEO_MEMORY_NAME) != 0
          && g_strcmp0 (allocator->mem_type,
              GST_VAAPI_DMABUF_ALLOCATOR_NAME) != 0))
    goto error_invalid_allocator;

  /* get the allocator properties */
  if (allocator) {
    priv->use_dmabuf_memory = gst_vaapi_is_dmabuf_allocator (allocator);
    negotiated_vinfo =
        gst_allocator_get_vaapi_negotiated_video_info (allocator);
    allocator_vinfo =
        gst_allocator_get_vaapi_video_info (allocator, &surface_alloc_flags);
  } else {
    priv->use_dmabuf_memory = FALSE;
    negotiated_vinfo = NULL;
    allocator_vinfo = NULL;
    surface_alloc_flags = 0;
  }

  /* reset or update the allocator if video resolution changed */
  if (allocator_vinfo
      && gst_video_info_changed (allocator_vinfo, &new_allocation_vinfo)) {
    gst_object_replace ((GstObject **) & priv->allocator, NULL);

    /* dmabuf allocator can change its parameters: no need to create a
     * new one */
    if (priv->use_dmabuf_memory) {
      gst_allocator_set_vaapi_video_info (allocator, &new_allocation_vinfo,
          surface_alloc_flags);
    } else {
      allocator = NULL;
    }
  }

  /* create a new allocator if needed */
  if (!allocator) {
    /* if no allocator set, let's create a VAAPI one */
    allocator = gst_vaapi_video_allocator_new (priv->display,
        &new_allocation_vinfo, surface_alloc_flags, 0);
    if (!allocator)
      goto error_no_allocator;

    if (negotiated_vinfo) {
      gst_allocator_set_vaapi_negotiated_video_info (allocator,
          negotiated_vinfo);
    }

    GST_INFO_OBJECT (base_pool, "created new allocator %" GST_PTR_FORMAT,
        allocator);
    gst_buffer_pool_config_set_allocator (config, allocator, NULL);
    gst_object_unref (allocator);
  }

  /* use the allocator and set the video info for the vmeta */
  if (allocator) {
    if (priv->allocator)
      gst_object_unref (priv->allocator);
    if ((priv->allocator = allocator))
      gst_object_ref (allocator);

    negotiated_vinfo =
        gst_allocator_get_vaapi_negotiated_video_info (priv->allocator);
    allocator_vinfo = gst_allocator_get_vaapi_video_info (allocator, NULL);
    priv->vmeta_vinfo = (negotiated_vinfo) ?
        *negotiated_vinfo : *allocator_vinfo;

    /* last resource to set the correct buffer size */
    if (GST_VIDEO_INFO_SIZE (allocator_vinfo) != size) {
      gst_buffer_pool_config_set_params (config, caps,
          GST_VIDEO_INFO_SIZE (allocator_vinfo), min_buffers, max_buffers);
    }
  }
  if (!priv->allocator)
    goto error_no_allocator;

  priv->options = 0;
  if (gst_buffer_pool_config_has_option (config,
          GST_BUFFER_POOL_OPTION_VIDEO_META)) {
    priv->options |= GST_VAAPI_VIDEO_BUFFER_POOL_OPTION_VIDEO_META;
  } else if (gst_caps_is_video_raw (caps) && !priv->use_dmabuf_memory) {
    gint i;

    for (i = 0; i < GST_VIDEO_INFO_N_PLANES (&new_allocation_vinfo); i++) {
      if (GST_VIDEO_INFO_PLANE_OFFSET (&new_allocation_vinfo, i) !=
          GST_VIDEO_INFO_PLANE_OFFSET (&priv->vmeta_vinfo, i) ||
          GST_VIDEO_INFO_PLANE_STRIDE (&new_allocation_vinfo, i) !=
          GST_VIDEO_INFO_PLANE_STRIDE (&priv->vmeta_vinfo, i) ||
          GST_VIDEO_INFO_SIZE (&new_allocation_vinfo) !=
          GST_VIDEO_INFO_SIZE (&priv->vmeta_vinfo)) {
        priv->options |= GST_VAAPI_VIDEO_BUFFER_POOL_OPTION_VIDEO_META;
        priv->forced_video_meta = TRUE;
        GST_INFO_OBJECT (base_pool, "adding unrequested video meta");
        break;
      }
    }
  }

  if (gst_buffer_pool_config_has_option (config,
          GST_BUFFER_POOL_OPTION_VIDEO_ALIGNMENT)) {
    fill_video_alignment (GST_VAAPI_VIDEO_BUFFER_POOL (pool), &align);
    gst_buffer_pool_config_set_video_alignment (config, &align);
  }

  if (!priv->use_dmabuf_memory && gst_buffer_pool_config_has_option (config,
          GST_BUFFER_POOL_OPTION_VIDEO_GL_TEXTURE_UPLOAD_META))
    priv->options |= GST_VAAPI_VIDEO_BUFFER_POOL_OPTION_GL_TEXTURE_UPLOAD;

  ret =
      GST_BUFFER_POOL_CLASS
      (gst_vaapi_video_buffer_pool_parent_class)->set_config (pool, config);
  return ret;

  /* ERRORS */
error_invalid_config:
  {
    GST_WARNING_OBJECT (base_pool, "invalid config");
    return FALSE;
  }
error_no_caps:
  {
    GST_WARNING_OBJECT (base_pool, "no caps in config");
    return FALSE;
  }
error_invalid_caps:
  {
    GST_WARNING_OBJECT (base_pool, "invalid caps %" GST_PTR_FORMAT, caps);
    return FALSE;
  }
error_invalid_allocator:
  {
    GST_INFO_OBJECT (base_pool, "no allocator in config");
    return FALSE;
  }
error_no_vaapi_video_meta_option:
  {
    GST_WARNING_OBJECT (base_pool, "no GstVaapiVideoMeta option in config");
    return FALSE;
  }
error_no_allocator:
  {
    GST_WARNING_OBJECT (base_pool, "no allocator defined");
    return FALSE;
  }
}

static void
vaapi_buffer_pool_cache_dma_mem (GstVaapiVideoBufferPool * pool,
    GstVaapiSurfaceProxy * proxy, GstMemory * mem)
{
  GstVaapiVideoBufferPoolPrivate *const priv = pool->priv;
  GstVaapiSurface *surface;

  surface = GST_VAAPI_SURFACE_PROXY_SURFACE (proxy);
  g_assert (surface);
  g_assert (gst_vaapi_surface_peek_buffer_proxy (surface));

  if (!priv->dma_mem_map)
    priv->dma_mem_map = g_hash_table_new_full (g_direct_hash,
        g_direct_equal, NULL, (GDestroyNotify) gst_memory_unref);

  if (!g_hash_table_contains (priv->dma_mem_map, surface)) {
    g_hash_table_insert (priv->dma_mem_map, surface, gst_memory_ref (mem));
  } else {
    g_assert (g_hash_table_lookup (priv->dma_mem_map, surface) == mem);
  }
}

static GstMemory *
vaapi_buffer_pool_lookup_dma_mem (GstVaapiVideoBufferPool * pool,
    GstVaapiSurfaceProxy * proxy)
{
  GstVaapiSurface *surface;
  GstVaapiVideoBufferPoolPrivate *const priv = pool->priv;
  GstVaapiBufferProxy *buf_proxy;
  GstMemory *mem;

  g_assert (priv->use_dmabuf_memory);

  if (!priv->dma_mem_map)
    return NULL;

  surface = GST_VAAPI_SURFACE_PROXY_SURFACE (proxy);
  g_assert (surface);

  buf_proxy = gst_vaapi_surface_peek_buffer_proxy (surface);
  /* Have not exported yet */
  if (!buf_proxy) {
    g_assert (!g_hash_table_contains (priv->dma_mem_map, surface));
    return NULL;
  }

  mem = g_hash_table_lookup (priv->dma_mem_map, surface);
  g_assert (mem);

  return gst_memory_ref (mem);
}

static GstFlowReturn
gst_vaapi_video_buffer_pool_alloc_buffer (GstBufferPool * pool,
    GstBuffer ** out_buffer_ptr, GstBufferPoolAcquireParams * params)
{
  GstVaapiVideoBufferPool *const base_pool = GST_VAAPI_VIDEO_BUFFER_POOL (pool);
  GstVaapiVideoBufferPoolPrivate *const priv = base_pool->priv;
  GstVaapiVideoBufferPoolAcquireParams *const priv_params =
      (GstVaapiVideoBufferPoolAcquireParams *) params;
  GstVaapiVideoMeta *meta = NULL;
  GstMemory *mem = NULL;
  GstBuffer *buffer = NULL;
  gboolean is_external_surface = !params ? FALSE :
      (params->flags &
      GST_VAAPI_VIDEO_BUFFER_POOL_ACQUIRE_FLAG_EXTERNAL_SURFACE) != 0;
  GstVaapiSurfaceProxy *external_proxy =
      priv_params ? priv_params->proxy : NULL;

  if (!priv->allocator) {
    GST_ERROR_OBJECT (base_pool, "no GstAllocator in buffer pool");
    goto error_to_create;
  }

  if (priv->use_dmabuf_memory && is_external_surface && !external_proxy) {
    GST_ERROR ("When create DMA buffer with external surface, "
        "the surface can not be NULL");
    goto error_to_create;
  }

  meta = gst_vaapi_video_meta_new (priv->display);
  if (!meta) {
    GST_ERROR_OBJECT (base_pool, "failed to allocate vaapi video meta");
    goto error_to_create;
  }

  buffer = gst_vaapi_video_buffer_new (meta);
  if (!buffer) {
    GST_ERROR_OBJECT (base_pool, "failed to create video buffer");
    goto error_to_create;
  }

  if (is_external_surface)
    gst_vaapi_video_meta_set_surface_proxy (meta, external_proxy);

  if (priv->use_dmabuf_memory) {
    mem = NULL;
    if (is_external_surface) {
      mem = vaapi_buffer_pool_lookup_dma_mem (base_pool, external_proxy);
      if (!mem) {
        mem = gst_vaapi_dmabuf_memory_new (priv->allocator, meta);
        if (!mem)
          goto error_to_create;

        vaapi_buffer_pool_cache_dma_mem (base_pool, external_proxy, mem);
      }
    } else {
      mem = gst_vaapi_dmabuf_memory_new (priv->allocator, meta);
    }
  } else {
    mem = gst_vaapi_video_memory_new (priv->allocator,
        meta, is_external_surface);
  }
  if (!mem)
    goto error_to_create;

  gst_vaapi_video_meta_replace (&meta, NULL);
  gst_buffer_append_memory (buffer, mem);

  if (priv->options & GST_VAAPI_VIDEO_BUFFER_POOL_OPTION_VIDEO_META) {
    GstVideoInfo *const vip = &priv->vmeta_vinfo;
    GstVideoMeta *vmeta;

    vmeta = gst_buffer_add_video_meta_full (buffer, 0,
        GST_VIDEO_INFO_FORMAT (vip), GST_VIDEO_INFO_WIDTH (vip),
        GST_VIDEO_INFO_HEIGHT (vip), GST_VIDEO_INFO_N_PLANES (vip),
        &GST_VIDEO_INFO_PLANE_OFFSET (vip, 0),
        &GST_VIDEO_INFO_PLANE_STRIDE (vip, 0));

    if (GST_VAAPI_IS_VIDEO_MEMORY (mem)) {
      vmeta->map = gst_video_meta_map_vaapi_memory;
      vmeta->unmap = gst_video_meta_unmap_vaapi_memory;
    }

    GST_META_FLAG_SET (vmeta, GST_META_FLAG_POOLED);
  }
#if (USE_GLX || USE_EGL)
  if (priv->options & GST_VAAPI_VIDEO_BUFFER_POOL_OPTION_GL_TEXTURE_UPLOAD) {
    GstMeta *tex_meta = gst_buffer_add_texture_upload_meta (buffer);
    if (tex_meta)
      GST_META_FLAG_SET (tex_meta, GST_META_FLAG_POOLED);
  }
#endif

  *out_buffer_ptr = buffer;
  return GST_FLOW_OK;

error_to_create:
  {
    GST_ERROR_OBJECT (base_pool, "failed to create video memory");
    if (mem)
      gst_memory_unref (mem);
    if (buffer)
      gst_buffer_unref (buffer);
    if (meta)
      gst_vaapi_video_meta_unref (meta);
    return GST_FLOW_ERROR;
  }
}

/* There is two manners to acquire_buffer()
   1. acquire_buffer() with no parameter. The returned buffer creates and
      holds the surface by itself. The surface will be cached until the buffer
      is freed or replaced by gst_vaapi_video_buffer_set_video_mem_surface_proxy().
   2. acquire_buffer() with
      GST_VAAPI_VIDEO_BUFFER_POOL_ACQUIRE_FLAG_EXTERNAL_SURFACE flag.
      The buffer uses the specified surface and will not allocate a surface
      internally. This mode is useful such as the decoder wants to create a
      gstbuffer with an already decoded picture. The buffer will not cache
      the surface when reset().
      For vaapi video memory, user can also change the underlying surface
      by gst_vaapi_video_buffer_set_video_mem_surface_proxy(). But for DMA
      kind memory, the underlying surface can not be changed.
   For vaapi video memory, the two kinds of buffers can be exchanged with each
   other. */
static GstFlowReturn
gst_vaapi_video_buffer_pool_acquire_buffer (GstBufferPool * pool,
    GstBuffer ** out_buffer_ptr, GstBufferPoolAcquireParams * params)
{
  GstVaapiVideoBufferPool *const base_pool = GST_VAAPI_VIDEO_BUFFER_POOL (pool);
  GstVaapiVideoBufferPoolPrivate *const priv = base_pool->priv;
  GstVaapiVideoBufferPoolAcquireParams *const priv_params =
      (GstVaapiVideoBufferPoolAcquireParams *) params;
  GstVaapiSurfaceProxy *const proxy = priv_params ? priv_params->proxy : NULL;
  gboolean is_external_surface = !params ? FALSE :
      (params->flags &
      GST_VAAPI_VIDEO_BUFFER_POOL_ACQUIRE_FLAG_EXTERNAL_SURFACE) != 0;
  GstFlowReturn ret;
  GstBuffer *buffer;
  GstMemory *mem;
  GstVaapiVideoMeta *meta;
  GstVaapiSurface *surface;

  if (!priv->allocator) {
    GST_ERROR_OBJECT (base_pool, "no GstAllocator in buffer pool");
    return GST_FLOW_ERROR;
  }

  if (priv->use_dmabuf_memory && is_external_surface && !proxy) {
    GST_ERROR ("When allocate DMA buffer with external surface, "
        "the surface can not be NULL");
    return GST_FLOW_ERROR;
  }

  ret =
      GST_BUFFER_POOL_CLASS
      (gst_vaapi_video_buffer_pool_parent_class)->acquire_buffer (pool, &buffer,
      params);

  if (ret != GST_FLOW_OK) {
    *out_buffer_ptr = NULL;
    return ret;
  }

  /* Not external surface, just return */
  if (!is_external_surface) {
    if (!priv->use_dmabuf_memory) {
      if (!gst_vaapi_video_memory_ensure_surface (GST_VAAPI_VIDEO_MEMORY_CAST
              (gst_buffer_peek_memory (buffer, 0)))) {
        gst_buffer_unref (buffer);
        *out_buffer_ptr = NULL;
        return GST_FLOW_ERROR;
      }
    }

    *out_buffer_ptr = buffer;
    return GST_FLOW_OK;
  }

  /* Video memory case with specified surface:
   *
   * Some pool users, such as decoders, need to acquire a buffer for
   * a specified surface (via surface proxy). So, just replace the
   * underlying surface proxy of buffer and mem.*/
  if (!priv->use_dmabuf_memory) {
    if (!gst_vaapi_video_buffer_set_video_mem_surface_proxy (buffer, proxy)) {
      gst_buffer_unref (buffer);
      *out_buffer_ptr = NULL;
    }

    *out_buffer_ptr = buffer;
    return GST_FLOW_OK;
  }

  meta = gst_buffer_get_vaapi_video_meta (buffer);
  g_assert (meta);

  /* DMABuf case with specified surface proxy, a little complicated:
   *
   * For DMABuf, GstMemory is-a GstFdMemory, which doesn't provide a
   * way to change its FD, thus once created it's bound to a
   * surface. On the other side, for performace reason, when the
   * buffer is released, the buffer and its memory are cached in the
   * buffer pool, and at next acquire_buffer() may still reuse a
   * buffer and its memory. But the pushed surface by the decoder may
   * be different from the one popped by the pool, so we need to
   * replace the buffer's memory with the correct one. */
  mem = vaapi_buffer_pool_lookup_dma_mem (base_pool, proxy);
  if (mem) {
    if (mem == gst_buffer_peek_memory (buffer, 0)) {
      gst_memory_unref (mem);
      *out_buffer_ptr = buffer;
      return GST_FLOW_OK;
    }
  } else {
    /* Should be an unexported surface */
    surface = GST_VAAPI_SURFACE_PROXY_SURFACE (proxy);
    g_assert (surface);
    g_assert (gst_vaapi_surface_peek_buffer_proxy (surface) == NULL);
    gst_vaapi_video_meta_set_surface_proxy (meta, proxy);
    mem = gst_vaapi_dmabuf_memory_new (priv->allocator, meta);
    if (mem)
      vaapi_buffer_pool_cache_dma_mem (base_pool, proxy, mem);
  }

  if (!mem) {
    gst_buffer_unref (buffer);
    *out_buffer_ptr = NULL;
    return GST_FLOW_ERROR;
  }

  gst_buffer_replace_memory (buffer, 0, mem);
  gst_buffer_unset_flags (buffer, GST_BUFFER_FLAG_TAG_MEMORY);
  *out_buffer_ptr = buffer;
  return GST_FLOW_OK;
}

static void
gst_vaapi_video_buffer_pool_reset_buffer (GstBufferPool * pool,
    GstBuffer * buffer)
{
  GstMemory *const mem = gst_buffer_peek_memory (buffer, 0);
  GstVaapiVideoMeta *meta;

  /* Release the underlying surface proxy */
  if (GST_VAAPI_IS_VIDEO_MEMORY (mem)) {
    gst_vaapi_video_memory_reset_surface (GST_VAAPI_VIDEO_MEMORY_CAST (mem));
  } else if (!gst_vaapi_dmabuf_memory_holds_surface (mem)) {
    /* If mem holds an internally created surface, don't reset it!
     * While surface is passed, we should clear it to avoid wrong
     * reference. */
    meta = gst_buffer_get_vaapi_video_meta (buffer);
    g_assert (meta);
    gst_vaapi_video_meta_set_surface_proxy (meta, NULL);
  }

  GST_BUFFER_POOL_CLASS (gst_vaapi_video_buffer_pool_parent_class)->reset_buffer
      (pool, buffer);
}

static void
gst_vaapi_video_buffer_pool_class_init (GstVaapiVideoBufferPoolClass * klass)
{
  GObjectClass *const object_class = G_OBJECT_CLASS (klass);
  GstBufferPoolClass *const pool_class = GST_BUFFER_POOL_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (gst_debug_vaapivideopool,
      "vaapivideopool", 0, "VA-API video pool");

  object_class->finalize = gst_vaapi_video_buffer_pool_finalize;
  object_class->set_property = gst_vaapi_video_buffer_pool_set_property;
  object_class->get_property = gst_vaapi_video_buffer_pool_get_property;
  pool_class->get_options = gst_vaapi_video_buffer_pool_get_options;
  pool_class->set_config = gst_vaapi_video_buffer_pool_set_config;
  pool_class->alloc_buffer = gst_vaapi_video_buffer_pool_alloc_buffer;
  pool_class->acquire_buffer = gst_vaapi_video_buffer_pool_acquire_buffer;
  pool_class->reset_buffer = gst_vaapi_video_buffer_pool_reset_buffer;

  /**
   * GstVaapiVideoBufferPool:display:
   *
   * The #GstVaapiDisplay this object is bound to.
   */
  g_object_class_install_property
      (object_class,
      PROP_DISPLAY,
      g_param_spec_object ("display", "Display",
          "The GstVaapiDisplay to use for this video pool",
          GST_TYPE_VAAPI_DISPLAY, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
gst_vaapi_video_buffer_pool_init (GstVaapiVideoBufferPool * pool)
{
  pool->priv = gst_vaapi_video_buffer_pool_get_instance_private (pool);
  pool->priv->dma_mem_map = NULL;
}

GstBufferPool *
gst_vaapi_video_buffer_pool_new (GstVaapiDisplay * display)
{
  return g_object_new (GST_VAAPI_TYPE_VIDEO_BUFFER_POOL,
      "display", display, NULL);
}

/**
 * gst_vaapi_video_buffer_pool_copy_buffer:
 * @pool: a #GstVaapiVideoBufferPool
 *
 * Returns if the @pool force set of #GstVideoMeta. If so, the element
 * should copy the generated buffer by the pool to a system allocated
 * buffer. Otherwise, downstream could not display correctly the
 * frame.
 *
 * Returns: %TRUE if #GstVideoMeta is forced.
 **/
gboolean
gst_vaapi_video_buffer_pool_copy_buffer (GstBufferPool * pool)
{
  GstVaapiVideoBufferPool *va_pool = (GstVaapiVideoBufferPool *) pool;

  g_return_val_if_fail (GST_VAAPI_IS_VIDEO_BUFFER_POOL (pool), FALSE);

  return va_pool->priv->forced_video_meta;
}
