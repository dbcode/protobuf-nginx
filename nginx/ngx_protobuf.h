#ifndef _NGX_PROTOBUF_H_INCLUDED_
#define _NGX_PROTOBUF_H_INCLUDED_

#include <stdint.h>
#include <endian.h>
#include <ngx_core.h>

/* wire types */

typedef enum {
  NGX_PROTOBUF_WIRETYPE_VARINT           = 0,
  NGX_PROTOBUF_WIRETYPE_FIXED64          = 1,
  NGX_PROTOBUF_WIRETYPE_LENGTH_DELIMITED = 2,
  NGX_PROTOBUF_WIRETYPE_START_GROUP      = 3,
  NGX_PROTOBUF_WIRETYPE_END_GROUP        = 4,
  NGX_PROTOBUF_WIRETYPE_FIXED32          = 5
} ngx_protobuf_wiretype_e;

/* labels for fields */

typedef enum {
  NGX_PROTOBUF_LABEL_OPTIONAL = 1,
  NGX_PROTOBUF_LABEL_REQUIRED = 2,
  NGX_PROTOBUF_LABEL_REPEATED = 3
} ngx_protobuf_label_e;

/* data types */

typedef enum {
  NGX_PROTOBUF_TYPE_DOUBLE   =  1,
  NGX_PROTOBUF_TYPE_FLOAT    =  2,
  NGX_PROTOBUF_TYPE_INT64    =  3,
  NGX_PROTOBUF_TYPE_UINT64   =  4,
  NGX_PROTOBUF_TYPE_INT32    =  5,
  NGX_PROTOBUF_TYPE_FIXED64  =  6,
  NGX_PROTOBUF_TYPE_FIXED32  =  7,
  NGX_PROTOBUF_TYPE_BOOL     =  8,
  NGX_PROTOBUF_TYPE_STRING   =  9,
  NGX_PROTOBUF_TYPE_GROUP    = 10,
  NGX_PROTOBUF_TYPE_MESSAGE  = 11,
  NGX_PROTOBUF_TYPE_BYTES    = 12,
  NGX_PROTOBUF_TYPE_UINT32   = 13,
  NGX_PROTOBUF_TYPE_ENUM     = 14,
  NGX_PROTOBUF_TYPE_SFIXED32 = 15,
  NGX_PROTOBUF_TYPE_SFIXED64 = 16,
  NGX_PROTOBUF_TYPE_SINT32   = 17,
  NGX_PROTOBUF_TYPE_SINT64   = 18,
} ngx_protobuf_type_e;

/* hook for protobuf modules to register extensions. */

typedef struct {
  ngx_int_t (*register_extensions)(ngx_cycle_t *cycle);
} ngx_protobuf_module_t;

/* protobuf module identifier */

#define NGX_PROTOBUF_MODULE 0x50425546  /* "PBUF" */

/* forward declaration of context object */

typedef struct ngx_protobuf_context_s ngx_protobuf_context_t;

/* method signatures for pack, unpack, and size calculation. */

typedef ngx_int_t (*ngx_protobuf_pack_pt)
  (void *obj, ngx_protobuf_context_t *ctx);
typedef ngx_int_t (*ngx_protobuf_unpack_pt)
  (void *obj, ngx_protobuf_context_t *ctx);
typedef size_t (*ngx_protobuf_size_pt)(void *obj);

/* protobuf pack/unpack buffer.  when packing a protobuf message into
 * a buffer, it is the caller's responsibility to ensure that there is
 * enough space in the buffer to fit the message.  when unpacking, the
 * caller must initialize the start and last pointers so the unpack
 * routines know where the input data begin and end.  the routines
 * will use the pos pointer internally.
 */

typedef struct {
  u_char  *start;
  u_char  *pos;
  u_char  *last;
} ngx_protobuf_buffer_t;

#if 0
/* protobuf context state object.  this object contains the internal
 * state of the pack or unpack routines as they execute, such that a
 * protobuf message can be packed or unpacked incrementally (reusing
 * the context from one call to the next).  so, for example, objects
 * that need to be unpacked from several non-contiguous input buffers
 * can be unpacked by calling the top-level unpack routine on each of
 * the input buffers in succession.  the same is true for objects that
 * need to be packed into multiple output buffers.
 */

typedef struct {
  ngx_int_t                status;
  uint32_t                 field;
  ngx_protobuf_wiretype_e  wire;
  uint64_t                 varint;
  u_char                   fixed[8];
  ngx_protobuf_buffer_t    buffer;
  ngx_array_t              opstack;  /* ngx_protobuf_op_pt */
} ngx_protobuf_state_t;
#endif

/* protobuf pack/unpack context.  the context contains a buffer for the
 * input or output binary data, a state object to support incremental 
 * serialization and deserialization, a set of flags to control certain
 * aspects of the data processing, a memory pool for any memory that
 * needs to be allocated along the way, and a log for error messages.
 */

struct ngx_protobuf_context_s {
  ngx_protobuf_buffer_t    buffer;
  // ngx_protobuf_state_t     state;
  uint32_t                 reuse_strings : 1;
  ngx_pool_t              *pool;
  ngx_log_t               *log;
};

/* field descriptor */

typedef struct {
  ngx_str_t                name;
  uint32_t                 number;
  ngx_protobuf_wiretype_e  wire_type;
  ngx_protobuf_label_e     label;
  ngx_protobuf_type_e      type;
  size_t                   width;
  ngx_flag_t               packed;
  ngx_protobuf_pack_pt     pack;
  ngx_protobuf_unpack_pt   unpack;
  ngx_protobuf_size_pt     size;
} ngx_protobuf_field_descriptor_t;

/* generic container for an unpacked value. */

typedef struct {
  ngx_protobuf_type_e  type;
  ngx_flag_t           exists;
  union {
    uint32_t           v_uint32;
    int32_t            v_int32;
    uint64_t           v_uint64;
    int64_t            v_int64;
    float              v_float;
    double             v_double;
    ngx_str_t          v_bytes;
    void              *v_message;
    ngx_array_t       *v_repeated;
  } u;
} ngx_protobuf_value_t;

/* container for an unknown field. */

typedef struct {
  uint32_t                 number;
  ngx_protobuf_wiretype_e  wire_type;
  ngx_protobuf_value_t     value;
} ngx_protobuf_unknown_field_t;

/* rbtree node for an extension descriptor */

typedef struct {
  ngx_rbtree_node_t                 node;
  ngx_protobuf_field_descriptor_t  *descriptor;
} ngx_protobuf_extension_descriptor_t;

/* rbtree node for an extension field. */

typedef struct {
  ngx_rbtree_node_t                 node;
  ngx_protobuf_field_descriptor_t  *descriptor;
  ngx_protobuf_value_t              value;
} ngx_protobuf_extension_field_t;

/* field prefix macros */

#define NGX_PROTOBUF_HEADER(field, wire)         \
  (((field) << 3) | NGX_PROTOBUF_WIRETYPE_##wire)

#define NGX_PROTOBUF_VARINT(field)               \
  NGX_PROTOBUF_HEADER(field, VARINT)

#define NGX_PROTOBUF_FIXED64(field)              \
  NGX_PROTOBUF_HEADER(field, FIXED64)

#define NGX_PROTOBUF_LENGTH_DELIMITED(field)     \
  NGX_PROTOBUF_HEADER(field, LENGTH_DELIMITED)

#define NGX_PROTOBUF_START_GROUP(field)          \
  NGX_PROTOBUF_HEADER(field, START_GROUP)

#define NGX_PROTOBUF_END_GROUP(field)            \
  NGX_PROTOBUF_HEADER(field, END_GROUP)

#define NGX_PROTOBUF_FIXED32(field)              \
  NGX_PROTOBUF_HEADER(field, FIXED32)

/* zigzag encoding and decoding */

#define NGX_PROTOBUF_Z32_ENCODE(val)             \
  ((val << 1) ^ (val >> 31))

#define NGX_PROTOBUF_Z32_DECODE(val)             \
  (int32_t)((val >> 1) ^ -(int32_t)(val & 1))

#define NGX_PROTOBUF_Z64_ENCODE(val)             \
  ((val << 1) ^ (val >> 63))

#define NGX_PROTOBUF_Z64_DECODE(val)             \
  (int64_t)((val >> 1) ^ -(int64_t)(val & 1))

/* accessor macros */

#define NGX_PROTOBUF_CLEAR_MEMBER(obj, field)    \
  do {                                           \
    (obj)->field = NULL;                         \
    (obj)->__has_##field = 0;                    \
  } while (0)

#define NGX_PROTOBUF_CLEAR_STRING(obj, field)    \
  do {                                           \
    (obj)->field.data = NULL;                    \
    (obj)->field.len = 0;                        \
    (obj)->__has_##field = 0;                    \
  } while (0)

#define NGX_PROTOBUF_CLEAR_NUMBER(obj, field)    \
  do {                                           \
    (obj)->field = 0;                            \
    (obj)->__has_##field = 0;                    \
  } while (0)

#define NGX_PROTOBUF_SET_MEMBER(obj, field, val) \
  do {                                           \
    (obj)->field = val;                          \
    if (val != NULL) {                           \
      (obj)->__has_##field = 1 ;                 \
    }                                            \
  } while (0)

#define NGX_PROTOBUF_SET_STRING(obj, field, str) \
  do {                                           \
    (obj)->field.data = str->data;               \
    (obj)->field.len = str->len;                 \
    (obj)->__has_##field = 1;                    \
  } while (0)

#define NGX_PROTOBUF_SET_NUMBER(obj, field, val) \
  do {                                           \
    (obj)->field = val;                          \
    (obj)->__has_##field = 1;                    \
  } while (0)  

#define NGX_PROTOBUF_SET_ARRAY(obj, field, val)  \
  do {                                           \
    (obj)->field = val;                          \
    if (val != NULL && val->nelts > 0) {         \
      (obj)->__has_##field = 1;                  \
    }                                            \
  } while (0)

#define NGX_PROTOBUF_HAS_FIELD(obj, field)       \
  (obj)->__has_##field

/* datatype size calculations */

#define NGX_PROTOBUF_SIZE_UINT                   \
  size_t s = 0;                                  \
                                                 \
  do { ++s; } while ((val >>= 7));               \
                                                 \
  return s

static ngx_inline size_t
ngx_protobuf_size_uint32(uint32_t val)
{
  NGX_PROTOBUF_SIZE_UINT;
}

static ngx_inline size_t
ngx_protobuf_size_uint64(uint64_t val)
{
  NGX_PROTOBUF_SIZE_UINT;
}

static ngx_inline size_t
ngx_protobuf_size_int32(int32_t val)
{
  return (val < 0) ? 10 : ngx_protobuf_size_uint32((uint32_t)val);
}

static ngx_inline size_t
ngx_protobuf_size_int64(int64_t val)
{
  return (val < 0) ? 10 : ngx_protobuf_size_uint64((uint64_t)val);
}

static ngx_inline size_t
ngx_protobuf_size_sint32(int32_t val)
{
  return ngx_protobuf_size_uint32(NGX_PROTOBUF_Z32_ENCODE(val));
}

static ngx_inline size_t
ngx_protobuf_size_sint64(int64_t val)
{
  return ngx_protobuf_size_uint64(NGX_PROTOBUF_Z64_ENCODE(val));
}

static ngx_inline size_t
ngx_protobuf_size_binary(size_t val)
{
  return ngx_protobuf_size_uint32(val) + val;
}

static ngx_inline size_t
ngx_protobuf_size_string(ngx_str_t *val)
{
  return ngx_protobuf_size_binary(val->len);
}

/* field size calculations */

#define ngx_protobuf_size_uint32_field(val, field)              \
  (ngx_protobuf_size_uint32(NGX_PROTOBUF_VARINT(field)) +       \
   ngx_protobuf_size_uint32(val))

#define ngx_protobuf_size_uint64_field(val, field)              \
  (ngx_protobuf_size_uint32(NGX_PROTOBUF_VARINT(field)) +       \
   ngx_protobuf_size_uint64(val))

#define ngx_protobuf_size_fixed32_field(field)                  \
  (ngx_protobuf_size_uint32(NGX_PROTOBUF_FIXED32(field)) + 4)

#define ngx_protobuf_size_fixed64_field(field)                  \
  (ngx_protobuf_size_uint32(NGX_PROTOBUF_FIXED64(field)) + 8)

#define ngx_protobuf_size_sint32_field(val, field)              \
  (ngx_protobuf_size_uint32(NGX_PROTOBUF_VARINT(field)) +       \
   ngx_protobuf_size_uint32(NGX_PROTOBUF_Z32_ENCODE(val)))

#define ngx_protobuf_size_sint64_field(val, field)              \
  (ngx_protobuf_size_uint32(NGX_PROTOBUF_VARINT(field)) +       \
   ngx_protobuf_size_uint64(NGX_PROTOBUF_Z64_ENCODE(val)))

#define ngx_protobuf_size_string_field(val, field)                      \
  (ngx_protobuf_size_uint32(NGX_PROTOBUF_LENGTH_DELIMITED(field)) +     \
   ngx_protobuf_size_string(val))

#define ngx_protobuf_size_message_field(len, field)                     \
  (ngx_protobuf_size_uint32(NGX_PROTOBUF_LENGTH_DELIMITED(field)) +     \
   ngx_protobuf_size_binary(len))

/* using fixed sizes for memcpy helps with optimization */

#define NGX_PROTOBUF_BIGENDIAN_COPY4(dst, src)                           \
  dst += 3;                                                              \
  *dst-- = *src++;  *dst-- = *src++;  *dst-- = *src++;  *dst   = *src

#define NGX_PROTOBUF_BIGENDIAN_COPY8(dst, src)                           \
  dst += 7;                                                              \
  *dst-- = *src++;  *dst-- = *src++;  *dst-- = *src++;  *dst-- = *src++; \
  *dst-- = *src++;  *dst-- = *src++;  *dst-- = *src++;  *dst   = *src;

static ngx_inline void
ngx_protobuf_copy_endian4(u_char *dst, u_char *src)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
  memcpy(dst, src, 4);
#else /* __BYTE_ORDER != __LITTLE_ENDIAN */
  NGX_PROTOBUF_BIGENDIAN_COPY4(dst, src);
#endif /* __BYTE_ORDER */
}

static ngx_inline void
ngx_protobuf_copy_endian8(u_char *dst, u_char *src)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
  memcpy(dst, src, 8);
#else /* __BYTE_ORDER != __LITTLE_ENDIAN */
  NGX_PROTOBUF_BIGENDIAN_COPY8(dst, src);
#endif /* __BYTE_ORDER */
}

static ngx_inline void
ngx_protobuf_copy_float4(u_char *dst, u_char *src)
{
#if __FLOAT_WORD_ORDER == __LITTLE_ENDIAN
  memcpy(dst, src, 4);
#else /* __FLOAT_WORD_ORDER != __LITTLE_ENDIAN */
  NGX_PROTOBUF_BIGENDIAN_COPY4(dst, src);
#endif /* __FLOAT_WORD_ORDER */
}

static ngx_inline void
ngx_protobuf_copy_float8(u_char *dst, u_char *src)
{
#if __FLOAT_WORD_ORDER == __LITTLE_ENDIAN
  memcpy(dst, src, 8);
#else /* __FLOAT_WORD_ORDER != __LITTLE_ENDIAN */
  NGX_PROTOBUF_BIGENDIAN_COPY8(dst, src);
#endif /* __FLOAT_WORD_ORDER */
}

/* reading values */

static ngx_inline ngx_int_t
ngx_protobuf_read_uint64(u_char **buf, u_char *end, uint64_t *val)
{
  uint32_t  s = 0;
  uint64_t  v = 0;
  u_char   *p = *buf;

  if (p >= end) {
    return NGX_ABORT;
  }

  do {
    v |= (uint64_t)(*p & 0x7f) << s;
    s += 7;
  } while ((*p++ & 0x80) && (p <= end) && (s < 64));

  *buf = p;

  if (*(p-1) & 0x80) {
    return NGX_ABORT;
  }

  *val = v;

  return NGX_OK;
}

static ngx_inline ngx_int_t
ngx_protobuf_read_uint32(u_char **buf, u_char *end, uint32_t *val)
{
  uint64_t v64;

  if (ngx_protobuf_read_uint64(buf, end, &v64) != NGX_OK) {
    return NGX_ABORT;
  }

  *val = (uint32_t)v64;

  return NGX_OK;
}

ngx_int_t ngx_protobuf_read_string(u_char **buf,
                                   u_char *end,
                                   ngx_str_t *val,
                                   ngx_pool_t *pool);

static ngx_inline ngx_int_t
ngx_protobuf_read_bool(u_char **buf, u_char *end, uint32_t *val)
{
  ngx_int_t rc;
  uint32_t  uval;

  rc = ngx_protobuf_read_uint32(buf, end, &uval);
  if (rc == NGX_OK) {
    *val = (uval != 0);
  }

  return rc;
}

#define NGX_PROTOBUF_READ_COPY(method, dst, size)               \
  if (*buf + size > end) {                                      \
    return NGX_ABORT;                                           \
  }                                                             \
                                                                \
  ngx_protobuf_copy_##method##size(*buf, (u_char *)dst);        \
  *buf += size

static ngx_inline ngx_int_t
ngx_protobuf_read_fixed32(u_char **buf, u_char *end, uint32_t *val)
{
  NGX_PROTOBUF_READ_COPY(endian, val, 4);

  return NGX_OK;
}

static ngx_inline ngx_int_t
ngx_protobuf_read_sfixed32(u_char **buf, u_char *end, int32_t *val)
{
  uint32_t uval;

  NGX_PROTOBUF_READ_COPY(endian, &uval, 4);
  *val = NGX_PROTOBUF_Z32_DECODE(uval);

  return NGX_OK;
}

static ngx_inline ngx_int_t
ngx_protobuf_read_fixed64(u_char **buf, u_char *end, uint64_t *val)
{
  NGX_PROTOBUF_READ_COPY(endian, val, 8);

  return NGX_OK;
}

static ngx_inline ngx_int_t
ngx_protobuf_read_sfixed64(u_char **buf, u_char *end, int64_t *val)
{
  uint64_t uval;
  
  NGX_PROTOBUF_READ_COPY(endian, &uval, 8);
  *val = NGX_PROTOBUF_Z64_DECODE(uval);

  return NGX_OK;
}

static ngx_inline ngx_int_t
ngx_protobuf_read_sint32(u_char **buf, u_char *end, int32_t *val)
{
  ngx_int_t rc;
  uint32_t  uval;

  rc = ngx_protobuf_read_uint32(buf, end, &uval);
  if (rc == NGX_OK) {
    *val = NGX_PROTOBUF_Z32_DECODE(uval);
  }

  return rc;
}

static ngx_inline ngx_int_t
ngx_protobuf_read_sint64(u_char **buf, u_char *end, int64_t *val)
{
  ngx_int_t rc;
  uint64_t  uval;

  rc = ngx_protobuf_read_uint64(buf, end, &uval);
  if (rc == NGX_OK) {
    *val = NGX_PROTOBUF_Z64_DECODE(uval);
  }

  return rc;
}

static ngx_inline ngx_int_t
ngx_protobuf_read_float(u_char **buf, u_char *end, float *val)
{
  NGX_PROTOBUF_READ_COPY(float, val, 4);

  return NGX_OK;
}

static ngx_inline ngx_int_t
ngx_protobuf_read_double(u_char **buf, u_char *end, double *val)
{
  NGX_PROTOBUF_READ_COPY(endian, val, 8);

  return NGX_OK;
}

/* writing values */

#define NGX_PROTOBUF_WRITE_UINT                                 \
  do { *buf++ = (uint8_t)(val | 0x80); } while ((val >>= 7));   \
  *(buf-1) &= 0x7f;                                             \
                                                                \
  return buf

static ngx_inline u_char *
ngx_protobuf_write_uint32(u_char *buf, uint32_t val)
{
  NGX_PROTOBUF_WRITE_UINT;
}

static ngx_inline u_char *
ngx_protobuf_write_uint64(u_char *buf, uint64_t val)
{
  NGX_PROTOBUF_WRITE_UINT;
}

static ngx_inline u_char *
ngx_protobuf_write_string(u_char *buf, ngx_str_t *val)
{
  buf = ngx_protobuf_write_uint32(buf, val->len);
  buf = ngx_cpymem(buf, val->data, val->len);

  return buf;
}

#define NGX_PROTOBUF_WRITE_COPY(method, size)             \
  ngx_protobuf_copy_##method##size(buf, (uint8_t *)&val); \
                                                          \
  return buf + size

static ngx_inline u_char *
ngx_protobuf_write_fixed32(u_char *buf, uint32_t val)
{
  NGX_PROTOBUF_WRITE_COPY(endian, 4);
}

static ngx_inline u_char *
ngx_protobuf_write_fixed64(u_char *buf, uint64_t val)
{
  NGX_PROTOBUF_WRITE_COPY(endian, 8);
}

#define ngx_protobuf_write_sfixed32(buf, val) \
  ngx_protobuf_write_fixed32(buf, NGX_PROTOBUF_Z32_ENCODE(val))

#define ngx_protobuf_write_sfixed64(buf, val) \
  ngx_protobuf_write_fixed64(buf, NGX_PROTOBUF_Z64_ENCODE(val))

#define ngx_protobuf_write_sint32(buf, val) \
  ngx_protobuf_write_uint32(buf, NGX_PROTOBUF_Z32_ENCODE(val))

#define ngx_protobuf_write_sint64(buf, val) \
  ngx_protobuf_write_uint64(buf, NGX_PROTOBUF_Z64_ENCODE(val))

static ngx_inline u_char *
ngx_protobuf_write_float(u_char *buf, float val)
{
  NGX_PROTOBUF_WRITE_COPY(float, 4);
}

static ngx_inline u_char *
ngx_protobuf_write_double(u_char *buf, double val)
{
  NGX_PROTOBUF_WRITE_COPY(float, 8);
}

/* writing fields */

static ngx_inline u_char *
ngx_protobuf_write_uint32_field(u_char *buf, uint32_t val, uint32_t field)
{
  buf = ngx_protobuf_write_uint32(buf, NGX_PROTOBUF_VARINT(field));
  buf = ngx_protobuf_write_uint32(buf, val);

  return buf;
}

static ngx_inline u_char *
ngx_protobuf_write_uint64_field(u_char *buf, uint64_t val, uint32_t field)
{
  buf = ngx_protobuf_write_uint32(buf, NGX_PROTOBUF_VARINT(field));
  buf = ngx_protobuf_write_uint64(buf, val);

  return buf;
}

static ngx_inline u_char *
ngx_protobuf_write_string_field(u_char *buf, ngx_str_t *val, uint32_t field)
{
  buf = ngx_protobuf_write_uint32(buf, NGX_PROTOBUF_LENGTH_DELIMITED(field));
  buf = ngx_protobuf_write_string(buf, val);

  return buf;
}

static ngx_inline u_char *
ngx_protobuf_write_fixed32_field(u_char *buf, uint32_t val, uint32_t field)
{
  buf = ngx_protobuf_write_uint32(buf, NGX_PROTOBUF_FIXED32(field));
  buf = ngx_protobuf_write_fixed32(buf, val);

  return buf;
}

static ngx_inline u_char *
ngx_protobuf_write_fixed64_field(u_char *buf, uint64_t val, uint32_t field)
{
  buf = ngx_protobuf_write_uint32(buf, NGX_PROTOBUF_FIXED64(field));
  buf = ngx_protobuf_write_fixed64(buf, val);

  return buf;
}

#define ngx_protobuf_write_sfixed32_field(buf, val, field) \
  ngx_protobuf_write_fixed32_field(buf, NGX_PROTOBUF_Z32_ENCODE(val), field)

#define ngx_protobuf_write_sfixed64_field(buf, val, field) \
  ngx_protobuf_write_fixed64_field(buf, NGX_PROTOBUF_Z64_ENCODE(val), field)

#define ngx_protobuf_write_sint32_field(buf, val, field) \
  ngx_protobuf_write_uint32_field(buf, NGX_PROTOBUF_Z32_ENCODE(val), field)

#define ngx_protobuf_write_sint64_field(buf, val, field) \
  ngx_protobuf_write_uint64_field(buf, NGX_PROTOBUF_Z64_ENCODE(val), field)

static ngx_inline u_char *
ngx_protobuf_write_float_field(u_char *buf, float val, uint32_t field)
{
  buf = ngx_protobuf_write_uint32(buf, NGX_PROTOBUF_FIXED32(field));
  buf = ngx_protobuf_write_float(buf, val);

  return buf;
}

static ngx_inline u_char *
ngx_protobuf_write_double_field(u_char *buf, float val, uint32_t field)
{
  buf = ngx_protobuf_write_uint32(buf, NGX_PROTOBUF_FIXED32(field));
  buf = ngx_protobuf_write_double(buf, val);

  return buf;
}

static ngx_inline u_char *
ngx_protobuf_write_message_header(u_char *buf, size_t len, uint32_t field)
{
  buf = ngx_protobuf_write_uint32(buf, NGX_PROTOBUF_LENGTH_DELIMITED(field));
  buf = ngx_protobuf_write_uint32(buf, len);

  return buf;
}

/* other non-inlined methods */

ngx_int_t ngx_protobuf_skip(u_char **buf, u_char *end, uint32_t wire);

void *ngx_protobuf_push_array(ngx_array_t **a, ngx_pool_t *p, size_t n);

ngx_int_t ngx_protobuf_import_extension(ngx_rbtree_t **registry,
                                        ngx_protobuf_field_descriptor_t *desc,
                                        ngx_cycle_t *cycle);

ngx_protobuf_value_t *ngx_protobuf_get_extension(ngx_rbtree_t *extensions,
                                                 uint32_t field);

ngx_int_t ngx_protobuf_set_extension(ngx_rbtree_t **extensions,
                                     ngx_rbtree_t *registry,
                                     uint32_t field,
                                     ngx_protobuf_value_t *value,
                                     ngx_pool_t *pool);

ngx_int_t ngx_protobuf_add_extension(ngx_rbtree_t **extensions,
                                     ngx_rbtree_t *registry,
                                     uint32_t field,
                                     void **ptr,
                                     ngx_pool_t *pool);

void ngx_protobuf_clear_extension(ngx_rbtree_t *extensions,
                                  uint32_t field);

ngx_int_t ngx_protobuf_unpack_extension(uint32_t field,
                                        ngx_protobuf_wiretype_e wire,
                                        ngx_protobuf_context_t *ctx,
                                        ngx_rbtree_t *registry,
                                        ngx_rbtree_t **extensions);

size_t ngx_protobuf_size_extensions(ngx_rbtree_t *extensions);

ngx_int_t ngx_protobuf_pack_extensions(ngx_rbtree_t *extensions,
                                       uint32_t lower,
                                       uint32_t upper,
                                       ngx_protobuf_context_t *ctx);

ngx_int_t ngx_protobuf_unpack_unknown_field(uint32_t field,
					    ngx_protobuf_wiretype_e wire,
					    ngx_protobuf_context_t *ctx,
					    ngx_array_t **unknown);

size_t ngx_protobuf_size_unknown_field(ngx_protobuf_unknown_field_t *field);

ngx_int_t ngx_protobuf_pack_unknown_field(ngx_protobuf_unknown_field_t *field,
					  ngx_protobuf_context_t *ctx);

#endif /* _NGX_PROTOBUF_H_INCLUDED_ */
