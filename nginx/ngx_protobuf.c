#include <ngx_core.h>
#include <ngx_protobuf.h>

static char *ngx_protobuf_init(ngx_cycle_t *cycle, void *conf);

static ngx_core_module_t ngx_protobuf_module_ctx = {
  ngx_string("protobuf"),
  NULL,
  ngx_protobuf_init
};

ngx_module_t ngx_protobuf_module = {
  NGX_MODULE_V1,
  &ngx_protobuf_module_ctx,  /* module context */
  NULL,                      /* module directives */
  NGX_CORE_MODULE,           /* module type */
  NULL,                      /* init master */
  NULL,                      /* init module */
  NULL,                      /* init process */
  NULL,                      /* init thread */
  NULL,                      /* exit thread */
  NULL,                      /* exit process */
  NULL,                      /* exit master */
  NGX_MODULE_V1_PADDING
};

static char *
ngx_protobuf_init(ngx_cycle_t *cycle, void *conf)
{
  ngx_protobuf_module_t  *module;
  ngx_uint_t              i;

  /* register extensions */
  for (i = 0; ngx_modules[i]; i++) {
    if (ngx_modules[i]->type == NGX_PROTOBUF_MODULE) {
      module = ngx_modules[i]->ctx;
      if (module != NULL && module->register_extensions != NULL) {
        module->register_extensions(cycle);
      }
    }
  }

  return NGX_CONF_OK;
}

ngx_int_t
ngx_protobuf_read_string(u_char **buf,
                         u_char *end,
                         ngx_str_t *val,
                         ngx_pool_t *pool)
{
  uint64_t  v = 0;

  if (*buf >= end) {
    return NGX_ABORT;
  }

  if (ngx_protobuf_read_uint64(buf, end, &v) != NGX_OK) {
    *buf = end;

    return NGX_ABORT;
  }

  if (*buf + v > end) {
    *buf = end;

    return NGX_ABORT;
  }

  if (pool != NULL) {
    val->data = ngx_palloc(pool, v);
    if (val->data == NULL) {
      *buf = end;

      return NGX_ERROR;
    }
    memcpy(val->data, *buf, v);
  } else {
    val->data = *buf;
  }

  val->len = v;
  *buf += v;

  return NGX_OK;
}

ngx_int_t
ngx_protobuf_skip(u_char **pos, u_char *end, uint32_t wire)
{
  uint64_t  v64;
  uint32_t  v32;
  ngx_str_t s;
  ngx_int_t rc = NGX_ABORT;

  switch (wire) {
  case NGX_PROTOBUF_WIRETYPE_VARINT:
    rc = ngx_protobuf_read_uint64(pos, end, &v64);
    break;
  case NGX_PROTOBUF_WIRETYPE_FIXED64:
    rc = ngx_protobuf_read_fixed64(pos, end, &v64);
    break;
  case NGX_PROTOBUF_WIRETYPE_LENGTH_DELIMITED:
    rc = ngx_protobuf_read_string(pos, end, &s, NULL);
    break;
  case NGX_PROTOBUF_WIRETYPE_FIXED32:
    rc = ngx_protobuf_read_fixed32(pos, end, &v32);
    break;
  default:
    rc = NGX_ABORT;
    break;
  }

  return rc;
}

void *
ngx_protobuf_push_array(ngx_array_t **a, ngx_pool_t *p, size_t n)
{
  void *ret = NULL;

  if (*a == NULL) {
    if (p != NULL) {
      *a = ngx_array_create(p, 1, n);
    }
  }

  if (*a != NULL) {
    ret = ngx_array_push(*a);
    if (ret != NULL) {
      ngx_memzero(ret, n);
    }
  }

  return ret;
}

static ngx_inline void *
ngx_protobuf_find_node(ngx_rbtree_t *tree, uint32_t field)
{
  ngx_rbtree_node_t *node;

  node = tree->root;
  while (node != tree->sentinel) {
    if (field < node->key) {
      node = node->left;
    } else if (field > node->key) {
      node = node->right;
    } else {
      return node;
    }
  }

  return NULL;
}

static ngx_int_t
ngx_protobuf_init_rbtree(ngx_rbtree_t **tree, ngx_pool_t *pool)
{
  ngx_rbtree_node_t *sentinel;

  if (*tree != NULL) {
    return NGX_OK;
  }

  sentinel = ngx_pcalloc(pool, sizeof(ngx_rbtree_node_t));
  if (sentinel == NULL) {
    return NGX_ERROR;
  }

  *tree = ngx_pcalloc(pool, sizeof(ngx_rbtree_t));
  if (*tree == NULL) {
    return NGX_ERROR;
  }

  ngx_rbtree_init(*tree, sentinel, ngx_rbtree_insert_value);

  return NGX_OK;
}

static void *
ngx_protobuf_rbtree_node(ngx_rbtree_t  *tree,
                         uint32_t       key,
                         ngx_pool_t    *pool,
                         size_t         size)
{
  ngx_rbtree_node_t *node;

  node = ngx_protobuf_find_node(tree, key);
  if (node == NULL) {
    /* allocate and initialize the node */
    node = ngx_pcalloc(pool, size);
    if (node != NULL) {
      node->key = key;
      ngx_rbtree_insert(tree, node);
    }
  }

  return node;
}

/* extension objects have the lifetime of the entire process, so the
 * tree and its nodes are allocated from the cycle pool.
 */

ngx_int_t
ngx_protobuf_import_extension(ngx_rbtree_t **registry,
                              ngx_protobuf_field_descriptor_t *desc,
                              ngx_cycle_t *cycle)
{
  ngx_protobuf_extension_descriptor_t *node;

  if (ngx_protobuf_init_rbtree(registry, cycle->pool) != NGX_OK) {
    return NGX_ERROR;
  }

  node = ngx_protobuf_rbtree_node(*registry,
                                  desc->number,
                                  cycle->pool,
                                  sizeof(ngx_protobuf_extension_descriptor_t));
  if (node == NULL) {
    return NGX_ERROR;
  }

  node->descriptor = desc;

  return NGX_OK;
}

/* get the value for an extension field (or NULL if not found) */

ngx_protobuf_value_t *
ngx_protobuf_get_extension(ngx_rbtree_t *extensions, uint32_t field)
{
  ngx_protobuf_extension_field_t *node;

  if (extensions == NULL) {
    return NULL;
  }

  node = ngx_protobuf_find_node(extensions, field);
  if (node == NULL) {
    return NULL;
  }

  return &node->value;
}

static ngx_int_t
ngx_protobuf_get_extension_node(ngx_rbtree_t **extensions,
                                ngx_rbtree_t *registry,
                                uint32_t field,
                                ngx_protobuf_extension_field_t **node,
                                ngx_pool_t *pool)
{
  ngx_protobuf_extension_descriptor_t *desc;

  *node = NULL;

  desc = ngx_protobuf_find_node(registry, field);
  if (desc == NULL) {
    /* trying to set an extension with an invalid field number */
    return NGX_ABORT;
  }

  /* allocate/init the extension rbtree if needed */
  if (ngx_protobuf_init_rbtree(extensions, pool) != NGX_OK) {
    /* could not allocate the extension rbtree */
    return NGX_ERROR;
  }

  /* find or create the extension's rbtree node */
  *node = ngx_protobuf_rbtree_node(*extensions, field, pool,
    sizeof(ngx_protobuf_extension_field_t));
  if (*node == NULL) {
    /* could not allocate the extension's rbtree node */
    return NGX_ERROR;
  }
  if ((*node)->descriptor == NULL) {
    (*node)->descriptor = desc->descriptor;
  }

  return NGX_OK;
}

static void *
ngx_protobuf_extension_field(ngx_protobuf_extension_field_t   *node,
                             ngx_pool_t                       *pool)
{
  void *obj;

  if (node->descriptor->label == NGX_PROTOBUF_LABEL_REPEATED) {
    obj = ngx_protobuf_push_array(&node->value.u.v_repeated,
                                  pool,
                                  node->descriptor->width);
  } else if (node->descriptor->type == NGX_PROTOBUF_TYPE_MESSAGE) {
    if (node->value.u.v_message == NULL) {
      node->value.u.v_message = ngx_pcalloc(pool, node->descriptor->width);
    }
    obj = node->value.u.v_message;
  } else {
    obj = &node->value.u;
  }

  return obj;
}

/* set an extension field's value */

ngx_int_t
ngx_protobuf_set_extension(ngx_rbtree_t **extensions,
                           ngx_rbtree_t *registry,
                           uint32_t field,
                           ngx_protobuf_value_t *value,
                           ngx_pool_t *pool)
{
  ngx_protobuf_extension_field_t  *n = NULL;
  ngx_int_t                        rc;

  rc = ngx_protobuf_get_extension_node(extensions, registry, field, &n, pool);
  if (rc != NGX_OK) {
    return rc;
  }

  if (value != NULL && value->exists) {
    /* check for type consistency */
    if (value->type != n->descriptor->type) {
      /* value type does not match descriptor type */
      return NGX_ABORT;
    } else {
      /* shallow copy the value */
      memcpy(&n->value, value, sizeof(ngx_protobuf_value_t));
    }
  } else {
    /* clear the value */
    ngx_memzero(&n->value, sizeof(ngx_protobuf_value_t));
  }

  return NGX_OK;
}

/* push an element onto a repeated extension field's array. */

ngx_int_t
ngx_protobuf_add_extension(ngx_rbtree_t **extensions,
                           ngx_rbtree_t *registry,
                           uint32_t field,
                           void **ptr,
                           ngx_pool_t *pool)
{
  ngx_protobuf_extension_field_t  *n = NULL;
  ngx_int_t                        rc;

  rc = ngx_protobuf_get_extension_node(extensions, registry, field, &n, pool);
  if (rc != NGX_OK) {
    return rc;
  }

  *ptr = ngx_protobuf_extension_field(n, pool);
  if (*ptr == NULL) {
    return NGX_ERROR;
  }

  return NGX_OK;
}

/* clears an extension field.  the node itself is kept around, but
 * the node's value is cleared.
 */

void ngx_protobuf_clear_extension(ngx_rbtree_t *extensions,
                                  uint32_t field)
{
  ngx_protobuf_value_t *value;

  value = ngx_protobuf_get_extension(extensions, field);
  if (value != NULL) {
    ngx_memzero(value, sizeof(ngx_protobuf_value_t));
  }
}

static ngx_int_t
ngx_protobuf_unpack_extension_value(ngx_protobuf_field_descriptor_t *field,
                                    void *obj,
                                    ngx_protobuf_context_t *ctx)
{
  u_char    **pos = &ctx->buffer.pos;
  u_char     *end = ctx->buffer.last;
  ngx_int_t   rc = NGX_OK;
  uint32_t    mlen;

  switch (field->type) {
  case NGX_PROTOBUF_TYPE_MESSAGE:
    rc = ngx_protobuf_read_uint32(pos, end, &mlen);
    if (rc == NGX_OK) {
      ctx->buffer.last = ctx->buffer.pos + mlen;
      rc = field->unpack(obj, ctx);
      ctx->buffer.last = end;
    }
    break;
  case NGX_PROTOBUF_TYPE_BYTES:
  case NGX_PROTOBUF_TYPE_STRING:
    rc = ngx_protobuf_read_string(pos, end, obj, ctx->pool);
    break;
  case NGX_PROTOBUF_TYPE_BOOL:
    rc = ngx_protobuf_read_bool(pos, end, obj);
    break;
  case NGX_PROTOBUF_TYPE_ENUM:    
  case NGX_PROTOBUF_TYPE_UINT32:
  case NGX_PROTOBUF_TYPE_INT32:
    rc = ngx_protobuf_read_uint32(pos, end, obj);
    break;
  case NGX_PROTOBUF_TYPE_SINT32:
    rc = ngx_protobuf_read_sint32(pos, end, obj);
    break;
  case NGX_PROTOBUF_TYPE_UINT64:
  case NGX_PROTOBUF_TYPE_INT64:
    rc = ngx_protobuf_read_uint64(pos, end, obj);
    break;
  case NGX_PROTOBUF_TYPE_SINT64:
    rc = ngx_protobuf_read_sint64(pos, end, obj);
    break;
  case NGX_PROTOBUF_TYPE_FIXED32:
    rc = ngx_protobuf_read_fixed32(pos, end, obj);
    break;
  case NGX_PROTOBUF_TYPE_SFIXED32:
    rc = ngx_protobuf_read_sfixed32(pos, end, obj);
    break;
  case NGX_PROTOBUF_TYPE_FLOAT:
    rc = ngx_protobuf_read_float(pos, end, obj);
    break;
  case NGX_PROTOBUF_TYPE_FIXED64:
    rc = ngx_protobuf_read_fixed64(pos, end, obj);
    break;
  case NGX_PROTOBUF_TYPE_SFIXED64:
    rc = ngx_protobuf_read_sfixed64(pos, end, obj);
    break;
  case NGX_PROTOBUF_TYPE_DOUBLE:
    rc = ngx_protobuf_read_double(pos, end, obj);
    break;
  default:
    break;
  }

  return rc;
}

ngx_int_t
ngx_protobuf_unpack_extension(uint32_t field,
                              ngx_protobuf_wiretype_e wire,
                              ngx_protobuf_context_t *ctx,
                              ngx_rbtree_t *registry,
                              ngx_rbtree_t **extensions)
{
  u_char                               **pos = &ctx->buffer.pos;
  u_char                                *end = ctx->buffer.last;
  ngx_flag_t                             packed = 0;
  ngx_protobuf_extension_descriptor_t   *rnode = NULL;
  ngx_protobuf_extension_field_t        *onode = NULL;
  ngx_protobuf_field_descriptor_t       *desc;
  void                                  *obj;
  uint32_t                               mlen;
  u_char                                *mend;
  ngx_int_t                              rc = NGX_OK;

  /* if the type allows extensions but none were registered, skip. */
  if (registry == NULL) {
    return ngx_protobuf_skip(pos, end, wire);
  }

  /* look for the extension based on field number */
  rnode = ngx_protobuf_find_node(registry, field);
  if (rnode == NULL) {
    return ngx_protobuf_skip(pos, end, wire);
  }

  desc = rnode->descriptor;

  /* the wire types should match, unless it's a packed field. */
  if (desc->wire_type != wire) {
    if (wire == NGX_PROTOBUF_WIRETYPE_LENGTH_DELIMITED && desc->packed) {
      packed = 1;
    } else {
      return NGX_ABORT;
    }
  }

  /* at this point, we need the pool */
  if (ctx->pool == NULL) {
    return NGX_ERROR;
  }

  if (ngx_protobuf_init_rbtree(extensions, ctx->pool) != NGX_OK) {
    return NGX_ERROR;
  }

  onode = ngx_protobuf_rbtree_node(*extensions, field, ctx->pool,
    sizeof(ngx_protobuf_extension_field_t));
  if (onode == NULL) {
    return NGX_ERROR;
  }
  if (onode->descriptor == NULL) {
    onode->descriptor = desc;
  }

  if (packed) {
    /* packed repeated field */
    if (ngx_protobuf_read_uint32(pos, end, &mlen) != NGX_OK) {
      return NGX_ABORT;
    }

    mend = *pos + mlen;
    if (mend > end) {
      return NGX_ABORT;
    }

    rc = NGX_OK;
    ctx->buffer.last = mend;
    while (*pos < mend) {
      obj = ngx_protobuf_extension_field(onode, ctx->pool);
      if (obj == NULL) {
        rc = NGX_ERROR;
        break;
      }
      rc = ngx_protobuf_unpack_extension_value(desc, obj, ctx);
      if (rc != NGX_OK) {
        break;
      } else {
        onode->value.exists = 1;
      }
    }
    ctx->buffer.last = end;
  } else {
    /* unpacked field */
    obj = ngx_protobuf_extension_field(onode, ctx->pool);
    if (obj == NULL) {
      return NGX_ERROR;
    }

    rc = ngx_protobuf_unpack_extension_value(desc, obj, ctx);
  }

  if (rc == NGX_OK) {
    onode->value.exists = 1;
    onode->value.type = desc->type;
  }

  return rc;
}

typedef ngx_int_t (*ngx_protobuf_rbtree_callback_pt)
  (ngx_rbtree_node_t *node, void *data);

static ngx_int_t
ngx_protobuf_rbtree_node_iterate(ngx_rbtree_node_t *node,
                                 ngx_rbtree_node_t *sentinel,
                                 ngx_rbtree_key_t lower,
                                 ngx_rbtree_key_t upper,
                                 ngx_protobuf_rbtree_callback_pt fptr,
                                 void *data);

static ngx_int_t
ngx_protobuf_rbtree_node_iterate(ngx_rbtree_node_t *node,
                                 ngx_rbtree_node_t *sentinel,
                                 ngx_rbtree_key_t lower,
                                 ngx_rbtree_key_t upper,
                                 ngx_protobuf_rbtree_callback_pt fptr,
                                 void *data)
{
  ngx_int_t rc;

  if (node->left != sentinel && node->left->key >= lower) {
    rc = ngx_protobuf_rbtree_node_iterate(node->left, sentinel,
                                          lower, upper,
                                          fptr, data);
    if (rc != NGX_OK) {
      return rc;
    }
  }

  if (node->key >= lower && node->key <= upper) {
    rc = fptr(node, data);
    if (rc != NGX_OK) {
      return rc;
    }
  }

  if (node->right != sentinel && node->right->key <= upper) {
    rc = ngx_protobuf_rbtree_node_iterate(node->right, sentinel,
                                          lower, upper,
                                          fptr, data);
    if (rc != NGX_OK) {
      return rc;
    }
  }

  return NGX_OK;
}

static ngx_int_t
ngx_protobuf_rbtree_iterate(ngx_rbtree_t *tree,
                            ngx_rbtree_key_t lower,
                            ngx_rbtree_key_t upper,
                            ngx_protobuf_rbtree_callback_pt fptr,
                            void *data)
{
  ngx_int_t rc = NGX_OK;

  if (tree->root != tree->sentinel) {
    rc = ngx_protobuf_rbtree_node_iterate(tree->root,
                                          tree->sentinel,
                                          lower,
                                          upper,
                                          fptr,
                                          data);
  }

  return rc;
}

static size_t
ngx_protobuf_packed_field_size(ngx_protobuf_field_descriptor_t *field,
                               ngx_array_t *data)
{
  void        *elts;
  ngx_uint_t   nelts;
  size_t       p = 0; 
  ngx_uint_t   i;

  if (data == NULL || data->elts == NULL || data->nelts == 0) {
    return NGX_OK;
  }

  elts = data->elts;
  nelts = data->nelts;

  switch (field->type) {
  case NGX_PROTOBUF_TYPE_BOOL:
    for (i = 0; i < nelts; ++i) {
      p += ngx_protobuf_size_uint32((((uint32_t *)elts)[i] != 0));
    }
    break;
  case NGX_PROTOBUF_TYPE_ENUM:
  case NGX_PROTOBUF_TYPE_UINT32:
  case NGX_PROTOBUF_TYPE_INT32:
    for (i = 0; i < nelts; ++i) {
      p += ngx_protobuf_size_uint32(((uint32_t *)elts)[i]);
    }
    break;
  case NGX_PROTOBUF_TYPE_SINT32:
    for (i = 0; i < nelts; ++i) {
      p += ngx_protobuf_size_sint32(((int32_t *)elts)[i]);
    }
    break;
  case NGX_PROTOBUF_TYPE_UINT64:
  case NGX_PROTOBUF_TYPE_INT64:
    for (i = 0; i < nelts; ++i) {
      p += ngx_protobuf_size_uint64(((uint64_t *)elts)[i]);
    }
    break;
  case NGX_PROTOBUF_TYPE_SINT64:
    for (i = 0; i < nelts; ++i) {
      p += ngx_protobuf_size_sint64(((int64_t *)elts)[i]);
    }
    break;
  case NGX_PROTOBUF_TYPE_FIXED32:
  case NGX_PROTOBUF_TYPE_SFIXED32:
  case NGX_PROTOBUF_TYPE_FLOAT:
    p = 4 * nelts;
    break;
  case NGX_PROTOBUF_TYPE_FIXED64:
  case NGX_PROTOBUF_TYPE_SFIXED64:
  case NGX_PROTOBUF_TYPE_DOUBLE:
    p = 8 * nelts;
    break;
  default:
    break;
  }

  return p;
}

static size_t
ngx_protobuf_repeated_field_size(ngx_protobuf_field_descriptor_t *field,
                                 ngx_array_t *data)
{
  uint32_t     fnum = field->number;
  void        *elts;
  ngx_uint_t   nelts;
  size_t       p = 0;
  size_t       n;
  ngx_uint_t   i;

  if (data == NULL || data->elts == NULL || data->nelts == 0) {
    return NGX_OK;
  }

  elts = data->elts;
  nelts = data->nelts;

  switch (field->type) {
  case NGX_PROTOBUF_TYPE_MESSAGE:
    for (i = 0; i < nelts; ++i) {
      n = field->size(((void **)elts) + i);
      p += ngx_protobuf_size_message_field(n, fnum);
    }
    break;
  case NGX_PROTOBUF_TYPE_BYTES:
  case NGX_PROTOBUF_TYPE_STRING:
    for (i = 0; i < nelts; ++i) {
      p += ngx_protobuf_size_string_field((ngx_str_t *)elts + i, fnum);
    }
    break;
  case NGX_PROTOBUF_TYPE_BOOL:
    for (i = 0; i < nelts; ++i) {
      p += ngx_protobuf_size_uint32_field((((uint32_t *)elts)[i] != 0), fnum);
    }
    break;
  case NGX_PROTOBUF_TYPE_ENUM:
  case NGX_PROTOBUF_TYPE_UINT32:
  case NGX_PROTOBUF_TYPE_INT32:
    for (i = 0; i < nelts; ++i) {
      p += ngx_protobuf_size_uint32_field(((uint32_t *)elts)[i], fnum);
    }
    break;
  case NGX_PROTOBUF_TYPE_SINT32:
    for (i = 0; i < nelts; ++i) {
      p += ngx_protobuf_size_sint32_field(((int32_t *)elts)[i], fnum);
    }
    break;
  case NGX_PROTOBUF_TYPE_UINT64:
  case NGX_PROTOBUF_TYPE_INT64:
    for (i = 0; i < nelts; ++i) {
      p += ngx_protobuf_size_uint64_field(((uint64_t *)elts)[i], fnum);
    }
    break;
  case NGX_PROTOBUF_TYPE_SINT64:
    for (i = 0; i < nelts; ++i) {
      p += ngx_protobuf_size_sint64_field(((int64_t *)elts)[i], fnum);
    }
    break;
  case NGX_PROTOBUF_TYPE_FIXED32:
  case NGX_PROTOBUF_TYPE_SFIXED32:
  case NGX_PROTOBUF_TYPE_FLOAT:
    p = nelts * ngx_protobuf_size_fixed32_field(fnum);
    break;
  case NGX_PROTOBUF_TYPE_FIXED64:
  case NGX_PROTOBUF_TYPE_SFIXED64:
  case NGX_PROTOBUF_TYPE_DOUBLE:
    p = nelts * ngx_protobuf_size_fixed64_field(fnum);
    break;
  default:
    break;
  }

  return p;
}

static size_t
ngx_protobuf_single_field_size(ngx_protobuf_field_descriptor_t *field,
                               ngx_protobuf_value_t *value)
{
  uint32_t fnum = field->number;
  size_t   p = 0;
  size_t   n;

  switch (field->type) {
  case NGX_PROTOBUF_TYPE_MESSAGE:
    n = field->size(value->u.v_message);
    p = ngx_protobuf_size_message_field(n, fnum);
    break;
  case NGX_PROTOBUF_TYPE_BYTES:
  case NGX_PROTOBUF_TYPE_STRING:
    p = ngx_protobuf_size_string_field(&value->u.v_bytes, fnum);
    break;
  case NGX_PROTOBUF_TYPE_BOOL:
    p = ngx_protobuf_size_uint32_field((value->u.v_uint32 != 0), fnum);
    break;
  case NGX_PROTOBUF_TYPE_ENUM:
  case NGX_PROTOBUF_TYPE_UINT32:
  case NGX_PROTOBUF_TYPE_INT32:
    p = ngx_protobuf_size_uint32_field(value->u.v_uint32, fnum);
    break;
  case NGX_PROTOBUF_TYPE_SINT32:
    p = ngx_protobuf_size_sint32_field(value->u.v_int32, fnum);
    break;
  case NGX_PROTOBUF_TYPE_UINT64:
  case NGX_PROTOBUF_TYPE_INT64:
    p = ngx_protobuf_size_uint64_field(value->u.v_uint64, fnum);
    break;
  case NGX_PROTOBUF_TYPE_SINT64:
    p = ngx_protobuf_size_sint64_field(value->u.v_int64, fnum);
    break;
  case NGX_PROTOBUF_TYPE_FIXED32:
  case NGX_PROTOBUF_TYPE_SFIXED32:
  case NGX_PROTOBUF_TYPE_FLOAT:
    p = ngx_protobuf_size_fixed32_field(fnum);
    break;
  case NGX_PROTOBUF_TYPE_FIXED64:
  case NGX_PROTOBUF_TYPE_SFIXED64:
  case NGX_PROTOBUF_TYPE_DOUBLE:
    p = ngx_protobuf_size_fixed64_field(fnum);
    break;
  default:
    break;
  }

  return p;
}

static ngx_int_t
ngx_protobuf_size_extension(ngx_rbtree_node_t *node, void *data)
{
  ngx_protobuf_extension_field_t   *field =
    (ngx_protobuf_extension_field_t *)node;
  ngx_protobuf_field_descriptor_t  *desc = field->descriptor;
  uint32_t                          fnum = desc->number;
  ngx_protobuf_value_t             *fval = &field->value;
  size_t                           *size = (size_t *)data;
  size_t                            n;

  if (!field->value.exists) {
    return NGX_OK;
  }

  if (desc->label == NGX_PROTOBUF_LABEL_REPEATED) {
    if (desc->packed) {
      n = ngx_protobuf_packed_field_size(desc, fval->u.v_repeated);
      *size += ngx_protobuf_size_message_field(n, fnum);
    } else {
      *size += ngx_protobuf_repeated_field_size(desc, fval->u.v_repeated);
    }
  } else {
    *size += ngx_protobuf_single_field_size(desc, fval);
  }

  return NGX_OK;
}

size_t
ngx_protobuf_size_extensions(ngx_rbtree_t *extensions)
{
  size_t size = 0;

  ngx_protobuf_rbtree_iterate(extensions,
                              0,
                              -1,
                              ngx_protobuf_size_extension,
                              &size);

  return size;
}

static ngx_int_t
ngx_protobuf_pack_packed_field(ngx_protobuf_field_descriptor_t *field,
                               ngx_protobuf_context_t *ctx,
                               ngx_array_t *data)
{
  uint32_t     fnum = field->number;
  void        *elts;
  ngx_uint_t   nelts;
  ngx_uint_t   i;
  size_t       n;

  if (data == NULL || data->elts == NULL || data->nelts == 0) {
    return NGX_OK;
  }

  elts = data->elts;
  nelts = data->nelts;

  n = ngx_protobuf_packed_field_size(field, data);
  ctx->buffer.pos =
    ngx_protobuf_write_message_header(ctx->buffer.pos, n, fnum);

  switch (field->type) {
  case NGX_PROTOBUF_TYPE_BOOL:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_uint32(ctx->buffer.pos,
                                  (((uint32_t *)elts)[i] != 0));
    }
    break;
  case NGX_PROTOBUF_TYPE_ENUM:
  case NGX_PROTOBUF_TYPE_UINT32:
  case NGX_PROTOBUF_TYPE_INT32:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_uint32(ctx->buffer.pos, ((uint32_t *)elts)[i]);
    }
    break;
  case NGX_PROTOBUF_TYPE_SINT32:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_sint32(ctx->buffer.pos, ((int32_t *)elts)[i]);
    }
    break;
  case NGX_PROTOBUF_TYPE_UINT64:
  case NGX_PROTOBUF_TYPE_INT64:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_uint64(ctx->buffer.pos, ((uint64_t *)elts)[i]);
    }
    break;
  case NGX_PROTOBUF_TYPE_SINT64:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_sint64(ctx->buffer.pos, ((int64_t *)elts)[i]);
    }
    break;
  case NGX_PROTOBUF_TYPE_FIXED32:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_fixed32(ctx->buffer.pos, ((uint32_t *)elts)[i]);
    }
    break;
  case NGX_PROTOBUF_TYPE_SFIXED32:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_sfixed32(ctx->buffer.pos, ((int32_t *)elts)[i]);
    }
    break;
  case NGX_PROTOBUF_TYPE_FLOAT:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_float(ctx->buffer.pos, ((float *)elts)[i]);
    }
    break;
  case NGX_PROTOBUF_TYPE_FIXED64:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_fixed64(ctx->buffer.pos, ((uint64_t *)elts)[i]);
    }
    break;
  case NGX_PROTOBUF_TYPE_SFIXED64:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_sfixed64(ctx->buffer.pos, ((int64_t *)elts)[i]);
    }
    break;
  case NGX_PROTOBUF_TYPE_DOUBLE:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_double(ctx->buffer.pos, ((double *)elts)[i]);
    }
    break;
  default:
    break;
  }

  return NGX_OK;
}

static ngx_int_t
ngx_protobuf_pack_repeated_field(ngx_protobuf_field_descriptor_t *field,
                                 ngx_protobuf_context_t *ctx,
                                 ngx_array_t *data)
{
  uint32_t     fnum = field->number;
  void        *elts;
  ngx_uint_t   nelts;
  ngx_uint_t   i;
  size_t       n;

  if (data == NULL || data->elts == NULL || data->nelts == 0) {
    return NGX_OK;
  }

  elts = data->elts;
  nelts = data->nelts;

  switch (field->type) {
  case NGX_PROTOBUF_TYPE_MESSAGE:
    for (i = 0; i < nelts; ++i) {
      n = field->size(((void **)elts) + i);
      ctx->buffer.pos =
        ngx_protobuf_write_message_header(ctx->buffer.pos, n, fnum);
      field->pack(((void **)elts) + i, ctx);
    }
    break;
  case NGX_PROTOBUF_TYPE_BYTES:
  case NGX_PROTOBUF_TYPE_STRING:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_string_field(ctx->buffer.pos,
                                        (ngx_str_t *)elts + i, fnum);
    }
    break;
  case NGX_PROTOBUF_TYPE_BOOL:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_uint32_field(ctx->buffer.pos,
                                        (((uint32_t *)elts)[i] != 0), fnum);
    }
    break;
  case NGX_PROTOBUF_TYPE_ENUM:
  case NGX_PROTOBUF_TYPE_UINT32:
  case NGX_PROTOBUF_TYPE_INT32:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_uint32_field(ctx->buffer.pos,
                                        ((uint32_t *)elts)[i], fnum);
    }
    break;
  case NGX_PROTOBUF_TYPE_SINT32:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_sint32_field(ctx->buffer.pos,
                                        ((int32_t *)elts)[i], fnum);
    }
    break;
  case NGX_PROTOBUF_TYPE_UINT64:
  case NGX_PROTOBUF_TYPE_INT64:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_uint64_field(ctx->buffer.pos,
                                        ((uint64_t *)elts)[i], fnum);
    }
    break;
  case NGX_PROTOBUF_TYPE_SINT64:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_sint64_field(ctx->buffer.pos,
                                        ((int64_t *)elts)[i], fnum);
    }
    break;
  case NGX_PROTOBUF_TYPE_FIXED32:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_fixed32_field(ctx->buffer.pos,
                                         ((uint32_t *)elts)[i], fnum);
    }
    break;
  case NGX_PROTOBUF_TYPE_SFIXED32:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_sfixed32_field(ctx->buffer.pos,
                                          ((int32_t *)elts)[i], fnum);
    }
    break;
  case NGX_PROTOBUF_TYPE_FLOAT:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_float_field(ctx->buffer.pos,
                                       ((float *)elts)[i], fnum);
    }
    break;
  case NGX_PROTOBUF_TYPE_FIXED64:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_fixed64_field(ctx->buffer.pos,
                                         ((uint64_t *)elts)[i], fnum);
    }
    break;
  case NGX_PROTOBUF_TYPE_SFIXED64:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_sfixed64_field(ctx->buffer.pos,
                                          ((int64_t *)elts)[i], fnum);
    }
    break;
  case NGX_PROTOBUF_TYPE_DOUBLE:
    for (i = 0; i < nelts; ++i) {
      ctx->buffer.pos =
        ngx_protobuf_write_double_field(ctx->buffer.pos,
                                        ((double *)elts)[i], fnum);
    }
    break;
  default:
    break;
  }

  return NGX_OK;
}

static ngx_int_t
ngx_protobuf_pack_single_field(ngx_protobuf_field_descriptor_t *field,
                               ngx_protobuf_context_t *ctx,
                               ngx_protobuf_value_t *value)
{
  uint32_t  fnum = field->number;
  size_t    n;

  switch (field->type) {
  case NGX_PROTOBUF_TYPE_MESSAGE:
    n = field->size(value->u.v_message);
    ctx->buffer.pos =
      ngx_protobuf_write_message_header(ctx->buffer.pos, n, fnum);
    field->pack(value->u.v_message, ctx);
    break;
  case NGX_PROTOBUF_TYPE_BYTES:
  case NGX_PROTOBUF_TYPE_STRING:
    ctx->buffer.pos =
      ngx_protobuf_write_string_field(ctx->buffer.pos,
                                      &value->u.v_bytes, fnum);
    break;
  case NGX_PROTOBUF_TYPE_BOOL:
    ctx->buffer.pos =
      ngx_protobuf_write_uint32_field(ctx->buffer.pos,
                                      (value->u.v_uint32 != 0), fnum);
    break;
  case NGX_PROTOBUF_TYPE_ENUM:
  case NGX_PROTOBUF_TYPE_UINT32:
  case NGX_PROTOBUF_TYPE_INT32:
    ctx->buffer.pos =
      ngx_protobuf_write_uint32_field(ctx->buffer.pos,
                                      value->u.v_uint32, fnum);
    break;
  case NGX_PROTOBUF_TYPE_SINT32:
    ctx->buffer.pos =
      ngx_protobuf_write_sint32_field(ctx->buffer.pos,
                                      value->u.v_int32, fnum);
    break;
  case NGX_PROTOBUF_TYPE_UINT64:
  case NGX_PROTOBUF_TYPE_INT64:
    ctx->buffer.pos =
      ngx_protobuf_write_uint64_field(ctx->buffer.pos,
                                      value->u.v_uint64, fnum);
    break;
  case NGX_PROTOBUF_TYPE_SINT64:
    ctx->buffer.pos =
      ngx_protobuf_write_sint64_field(ctx->buffer.pos,
                                      value->u.v_int64, fnum);
    break;
  case NGX_PROTOBUF_TYPE_FIXED32:
    ctx->buffer.pos =
      ngx_protobuf_write_fixed32_field(ctx->buffer.pos,
                                       value->u.v_uint32, fnum);
    break;
  case NGX_PROTOBUF_TYPE_SFIXED32:
    ctx->buffer.pos =
      ngx_protobuf_write_sfixed32_field(ctx->buffer.pos,
                                        value->u.v_int32, fnum);
    break;
  case NGX_PROTOBUF_TYPE_FLOAT:
    ctx->buffer.pos =
      ngx_protobuf_write_float_field(ctx->buffer.pos,
                                     value->u.v_float, fnum);
    break;
  case NGX_PROTOBUF_TYPE_FIXED64:
    ctx->buffer.pos =
      ngx_protobuf_write_fixed64_field(ctx->buffer.pos,
                                       value->u.v_uint64, fnum);
    break;
  case NGX_PROTOBUF_TYPE_SFIXED64:
    ctx->buffer.pos =
      ngx_protobuf_write_sfixed64_field(ctx->buffer.pos,
                                        value->u.v_int64, fnum);
    break;
  case NGX_PROTOBUF_TYPE_DOUBLE:
    ctx->buffer.pos =
      ngx_protobuf_write_double_field(ctx->buffer.pos,
                                      value->u.v_double, fnum);
    break;
  default:
    break;
  }

  return NGX_OK;
}

static ngx_int_t
ngx_protobuf_pack_extension(ngx_rbtree_node_t *node, void *data)
{
  ngx_protobuf_extension_field_t   *field =
    (ngx_protobuf_extension_field_t *)node;
  ngx_protobuf_context_t           *ctx = data;
  ngx_protobuf_value_t             *fval = &field->value;
  ngx_protobuf_field_descriptor_t  *desc = field->descriptor;
  size_t                            size = 0;
  ngx_int_t                         rc = NGX_OK;

  if (!fval->exists) {
    return NGX_OK;
  }

  ngx_protobuf_size_extension(node, &size);
  if (ctx->buffer.pos + size > ctx->buffer.last) {
    return NGX_ABORT;
  }

  if (desc->label == NGX_PROTOBUF_LABEL_REPEATED) {
    if (desc->packed) {
      rc = ngx_protobuf_pack_packed_field(desc, ctx, fval->u.v_repeated);
    } else {
      rc = ngx_protobuf_pack_repeated_field(desc, ctx, fval->u.v_repeated);
    }
  } else {
    rc = ngx_protobuf_pack_single_field(desc, ctx, fval);
  }

  return rc;
}

ngx_int_t
ngx_protobuf_pack_extensions(ngx_rbtree_t *extensions,
                             uint32_t lower,
                             uint32_t upper,
                             ngx_protobuf_context_t *ctx)
{
  return ngx_protobuf_rbtree_iterate(extensions,
                                     lower,
                                     upper,
                                     ngx_protobuf_pack_extension,
                                     ctx);
}
