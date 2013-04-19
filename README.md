protobuf-nginx - Google Protocol Buffers for nginx
==================================================

This is protobuf-nginx, a code generator and supporting library that aim to
help [nginx](http://nginx.com) module developers work with 
[Google Protocol Buffers](http://code.google.com/p/protobuf) messages.

Background and motivation
-------------------------

The Google Protocol Buffers (protobuf for short) serialization framework, with
its many language implementations, is a practical and efficient way to transfer
structured data between applications using language-neutral *message definitions*
that can be translated into native code in a variety of programming languages.
Google open-sourced protobuf in 2008.  Meanwhile, the nginx HTTP server (which
was originally released to the public in 2004) has become one of the most popular
HTTP servers in use today, thanks to its exceptional performance and stability.
In a single-threaded event-driven program such as nginx, efficiency is a key
concern, and nginx module developers are encouraged to make use of the nginx
framework (data structures such as arrays and rbtrees, memory pools, etc) in order
to do things the *nginx way*: not one byte of memory nor one CPU cycle shall be
wasted.  Embrace the nginx way, and you embrace all that is good and right in
the world.

The protobuf-nginx project aims to bring protobuf to nginx, the nginx way: with
generated code that uses nginx types, memory pools, and modules in order to give
nginx C module developers the power of Google's serialization framework within
the nginx environment.  The generated code (along with a core protobuf module
that is distributed as part of protobuf-nginx) can be compiled directly into the 
nginx binary without any other dependencies.

Supported and unsupported features
----------------------------------

protobuf-nginx should be able to generate code for any protobuf message definition,
including enums, nested messages, and extensions.  The list of currently unsupported
features is as follows:

* [RPC services](https://developers.google.com/protocol-buffers/docs/proto#services).
* Retention of unknown fields.  Unknown fields are currently dropped, although they do not break the parsing.
* [Default values](https://developers.google.com/protocol-buffers/docs/proto#optional) for missing optional fields.

Support for all of these features, in at least some form, is planned.  See the
[TODO](https://github.com/dbcode/protobuf-nginx/blob/master/TODO) file for more
details on these and other planned improvements to protobuf-nginx.

A simple example
----------------

We can write a header filter module (similar to the [User ID module](http://wiki.nginx.org/HttpUseridModule))
that creates, reads, and updates a cookie, the value of which is a base64 encoded
serialized protobuf message.  Let's start with a simple message definition:

    package cookie;
    
    message User {
      message Channel {
        required bytes  name      = 1;
        required uint64 timestamp = 2;
        optional bytes  metadata  = 3;
      }
      required uint64  created  = 1;
      required uint64  updated  = 2;
      required uint64  counter  = 3;
      optional uint64  timegap  = 4;
      repeated Channel channels = 5;
    }

This message includes a creation timestamp, an update timestamp, a counter, a
numeric value that gives the elapsed time between the previous update timestamp
and the most recent update, and a list of Channel submessages, each of which
contains a string name, a timestamp, and optional string metadata.

If this file is saved as cookie.proto, the nginx code generator can be invoked
like so:

    protongx --out=. cookie.proto

The --out option allows you to specify an output directory for the generated code.
The generated code will be a complete nginx module that lives in a subdirectory
of the --out directory.  In this example, the nginx module generated from cookie.proto
is created in the ngx_cookie_proto subdirectory of the --out directory, and includes
the following files:

    config
    ngx_cookie_proto.c
    ngx_cookie_proto.h

To build your nginx with this module, you will need to add the core ngx_protobuf
module and this generated module to your configure arguments for nginx:

    ./configure \
      --add-module=/usr/local/share/protobuf-nginx \
      --add-module=/path/to/ngx_cookie_proto

The core ngx_protobuf module is installed in /usr/local/share (or $PREFIX/share)
in the protobuf-nginx subdirectory, and consists of the following files:

    config
    ngx_protobuf.c
    ngx_protobuf.h

The generated module calls some functions from the core module, so it's important
that both the core and the generated module be included in your nginx build.

The struct typedefs in ngx_cookie_proto.h show the nginx representation of the
cookie.User message and its nested message (cookie.User.Channel):

````c
/* cookie.User.Channel */

typedef struct {
    ngx_str_t name;
    uint64_t  timestamp;
    ngx_str_t metadata;
    uint32_t  __has_name      : 1;
    uint32_t  __has_timestamp : 1;
    uint32_t  __has_metadata  : 1;
} ngx_cookie_user_channel_t;

/* cookie.User */

typedef struct {
    uint64_t     created;
    uint64_t     updated;
    uint64_t     counter;
    uint64_t     timegap;
    /* ngx_cookie_user_channel_t */
    ngx_array_t *channels;
    uint32_t     __has_created  : 1;
    uint32_t     __has_updated  : 1;
    uint32_t     __has_counter  : 1;
    uint32_t     __has_timegap  : 1;
    uint32_t     __has_channels : 1;
} ngx_cookie_user_t;
````

Now let's say you want to write a filter module that reads and sets cookies with these
(base64 encoded) messages in them.  Ignoring the base64 encoding/decoding part, you'll
unpack your cookie using a *protobuf context* that includes pointers to the start and
end of your serialized data, and optional flags to control some aspects of the parsing.
Your parsing code will look something like this:

````c
ngx_cookie_user_t *
unpack_user(ngx_str_t *val, ngx_pool_t *pool)
{
  ngx_protobuf_context_t   ctx;
  ngx_cookie_user_t       *user;
  
  user = ngx_cookie_user__alloc(pool);
  if (user == NULL) {
    return NULL;
  }
  
  ctx.pool = pool;
  ctx.buffer.start = val->data;
  ctx.buffer.pos = ctx.buffer.start;
  ctx.buffer.last = val->data + val->len;
  ctx.reuse_strings = 1;
  
  if (ngx_cookie_user__unpack(user, &ctx) != NGX_OK) {
    return NULL;
  }
  
  return user;
}
````

The protobuf context needs a memory pool in order to unpack the message
contents, including possible string fields, repeated fields, and nested
message fields.  All of these fields require allocation of memory (with
the possible exception of string fields - see the information on the
**reuse_strings** flag below), so a pool is often required.  Note that if
you are unpacking a message that consists solely of non-repeated primitive
fields such as integer types, a pool is not necessary.

The protobuf context also contains a simple memory buffer:

````c
typedef struct {
  u_char  *start;
  u_char  *pos;
  u_char  *last;
} ngx_protobuf_buffer_t;
````

When unpacking a serialized object, the *start* and *pos* pointers should 
point to the beginning of the serialized data, and the *last* pointer should
point to the end.  When packing an object, the *start* and *pos* pointers
should point to the beginning of the output buffer, and the *last* pointer
should point to the end of that buffer's available space.  Once an object
has been packed, the buffer's *pos* pointer will point to the end of the
serialized output.  You can then use the *pos* pointer's offset from the
*start* pointer to calculate the length of your output (which should be the
same as its size):

    size_t size = ngx_cookie_user__size(user);

The **reuse_strings** flag lets you specify whether string fields should be
allocated into their own separate memory from the protobuf context's pool,
or can reuse (point to) memory in the underlying input.  If you want extra
efficiency and can guarantee that your object will not outlive the raw data
from which it was unpacked, the **reuse_strings** flag lets you avoid unnecessary
allocation of memory from the context pool.

Now you can modify the object in whatever way you like:

````c
void
update_user(ngx_cookie_user_t *user)
{
  ngx_time_t  *tp;
  uint64_t     now;
  
  tp = ngx_timeofday();
  now = tp->sec * 1000 + tp->msec;

  user->counter += 1;
  user->timegap = now - user->updated;
  user->updated = now;
}
````

Now it's time to repack the modified object:

````c
ngx_int_t
pack_user(ngx_cookie_user_t *user, ngx_str_t *output, ngx_pool_t *pool)
{
  ngx_protobuf_context_t  ctx;
  size_t                  size;
  ngx_int_t               rc;
  
  size = ngx_cookie_user__size(user);
  if (size == 0) {
    return NGX_DECLINED;
  }
  
  output->data = ngx_palloc(pool, size);
  if (output->data == NULL) {
    return NGX_ERROR;
  }
  
  ctx.pool = pool;
  ctx.buffer.start = output->data;
  ctx.buffer.pos = ctx.buffer.start;
  ctx.buffer.last = ctx.buffer.start + size;

  rc = ngx_cookie_user__pack(user, &ctx);
  output->len = ctx.buffer.pos - ctx.buffer.start;
  
  return rc;
}
````

How it all works
----------------

At nginx startup time, the core ngx_protobuf module scans the list of nginx modules
for any that match the NGX_PROTOBUF_MODULE (0x50425546, or "PBUF") tag.  Each of
these modules corresponds to a .proto file that may contain any number of enums,
messages, and extensions to other messages.  In most cases, nothing special needs
to be done in order to for these modules to function.  However, a .proto file that
includes extensions must register its extensions with the extendee (the type that
is being extended).  This is done by calling a "register_extensions" method that
is defined in any protobuf module that has extensions defined:

````c
  /* register extensions */
  for (i = 0; ngx_modules[i]; i++) {
    if (ngx_modules[i]->type == NGX_PROTOBUF_MODULE) {
      module = ngx_modules[i]->ctx;
      if (module != NULL && module->register_extensions != NULL) {
        module->register_extensions(cycle);
      }
    }
  }
````

At the present time, the registration of extnsions is the only thing that needs
to be done explicitly at nginx startup.  Translating .proto files into nginx
modules lets us do this in a natural way.

As was mentioned earlier, each .proto file is translated to a complete nginx
module in its own subdirectory.  You can define messages in as many .proto files
as you like, and invoke protongx on all of them at once:

    protongx --out=/path/to/protobuf/modules /path/to/protos/*.proto

The end result will be that /path/to/protobuf/modules contains a subdirectory
for each input .proto file.  Each of these subdirectories can then be added to
the configure arguments to nginx, as shown above for the simple case of the 
cookie.proto file.

19 April 2013
Dave Bailey <dave@daveb.net>
