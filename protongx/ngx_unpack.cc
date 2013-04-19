#include <ngx_flags.h>
#include <ngx_generator.h>
#include <google/protobuf/descriptor.pb.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace nginx {

void
Generator::GenerateUnpack(const Descriptor* desc, io::Printer& printer)
{
  Flags flags(desc);

  std::map<std::string, std::string> vars;

  vars["name"] = desc->full_name();
  vars["root"] = TypedefRoot(desc->full_name());
  vars["type"] = StructType(desc->full_name());

  if (flags.has_message()) {

    // generate a static helper method to unpack each message field

    for (int i = 0; i < desc->field_count(); ++i) {
      const FieldDescriptor *field = desc->field(i);

      if (field->type() == FieldDescriptor::TYPE_MESSAGE) {
        std::string ftype(FieldRealType(field));
        size_t space;

        if (field->is_repeated() && ftype.length() >= sizeof("ngx_int_t")) {
          space = ftype.length() + 2 - sizeof("ngx_int_t");
        } else {
          space = 1;
        }

        vars["fname"] = field->name();
        vars["ffull"] = field->full_name();
        vars["ftype"] = ftype;
        vars["froot"] = TypedefRoot(field->message_type()->full_name());
        vars["space"] = Spaces(space);

        printer.Print(vars,
                      "static ngx_int_t\n"
                      "$root$__unpack_$fname$(\n"
                      "    $type$ *obj,\n"
                      "    ngx_protobuf_context_t *ctx,\n"
                      "    size_t len)\n");
        OpenBrace(printer);

        if (field->is_repeated()) {
          printer.Print(vars, "$ftype$  *fptr;\n");
        }

        printer.Print(vars,
                      "u_char    $space$*end0;\n"
                      "u_char    $space$*end;\n"
                      "ngx_int_t $space$ rc;\n"
                      "\n");

        FullSimpleIf(printer, vars,
                     "len == 0",
                     "return NGX_OK;");

        printer.Print("\n"
                      "end = ctx->buffer.pos + len;\n");

        FullSimpleIf(printer, vars,
                     "end > ctx->buffer.last",
                     "return NGX_ABORT;");

        printer.Print("\n");

        if (field->is_repeated()) {
          printer.Print(vars,
                        "fptr = $root$__add__$fname$(obj, ctx->pool);\n");
          FullSimpleIf(printer, vars,
                       "fptr == NULL",
                       "return NGX_ERROR;");
        } else {
          printer.Print(vars,
                        "if (obj->$fname$ != NULL) ");
          OpenBrace(printer);
          printer.Print(vars, "$froot$__clear(obj->$fname$);\n");
          Else(printer);
          FullSimpleIf(printer, vars,
                       "ctx->pool != NULL",
                       "obj->$fname$ = $froot$__alloc(ctx->pool);");
          FullSimpleIf(printer, vars,
                       "obj->$fname$ == NULL",
                       "return NGX_ERROR;");
          CloseBrace(printer);
        }

        printer.Print("\n"
                      "end0 = ctx->buffer.last;\n"
                      "ctx->buffer.last = end;\n");


        if (field->is_repeated()) {
          printer.Print(vars, "rc = $froot$__unpack(fptr, ctx);\n");
        } else {
          printer.Print(vars, "rc = $froot$__unpack(obj->$fname$, ctx);\n");
        }
                      
        printer.Print("ctx->buffer.last = end0;\n"
                      "\n");

        FullSimpleIf(printer, vars,
                     "rc == NGX_OK",
                     "obj->__has_$fname$ = 1;");

        printer.Print("\n"
                      "return rc;\n");

        CloseBrace(printer);
        printer.Print("\n");
      }
    }
  }

  // now for the actual unpack method itself

  printer.Print(vars,
                "ngx_int_t\n"
                "$root$__unpack(\n"
                "    $type$ *obj,\n"
                "    ngx_protobuf_context_t *ctx)\n"
                "{\n");

  Indent(printer);

  printer.Print(vars,
                "u_char      **pos = &ctx->buffer.pos;\n"
                "u_char       *end = ctx->buffer.last;\n");

  if (flags.has_string() || flags.has_repnm()) {
    printer.Print("ngx_pool_t   *pool = ctx->pool;\n");
  }

  printer.Print("uint32_t      header;\n"
                "uint32_t      field;\n"
                "uint32_t      wire;\n");

  if (flags.has_bool()) {
    printer.Print("uint32_t      flag;\n");
  }
  if (flags.has_int32()) {
    printer.Print("uint32_t      u32v;\n");
  }
  if (flags.has_int64()) {
    printer.Print("uint64_t      u64v;\n");
  }
  if (flags.has_message() || flags.has_packed()) {
    printer.Print("uint32_t      mlen;\n");
  }
  if (flags.has_packed()) {
    printer.Print("u_char       *mend;\n");
  }
  if (flags.has_message() || desc->extension_range_count() > 0) {
    printer.Print("ngx_int_t     rc;\n");
  }

  printer.Print("\n"
                "while (*pos < end) {\n");
  Indent(printer);
  FullSimpleIf(printer, vars,
               "ngx_protobuf_read_uint32(pos, end, &header) != NGX_OK",
               "return NGX_ABORT;");
  printer.Print("\n");
  FullSimpleIf(printer, vars, "*pos >= end", "return NGX_ABORT;");
  printer.Print("\n"
                "field = header >> 3;\n"
                "wire = header & 0x07;\n"
                "\n"
                "switch (field) {\n");
  
  for (int i = 0; i < desc->field_count(); ++i) {
    const FieldDescriptor *field = desc->field(i);

    vars["fname"] = field->name();
    vars["ffull"] = field->full_name();
    vars["ftype"] = FieldRealType(field);
    vars["fnum"] = Number(field->number());

    printer.Print(vars,
                  "case $fnum$: /* $ffull$ */\n");
    Indent(printer);

    switch (field->type()) {
    case FieldDescriptor::TYPE_BOOL:
    case FieldDescriptor::TYPE_ENUM:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64:
      printer.Print("if (wire == NGX_PROTOBUF_WIRETYPE_VARINT) {\n");
      break;
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_DOUBLE:
      printer.Print("if (wire == NGX_PROTOBUF_WIRETYPE_FIXED64) {\n");
      break;
    case FieldDescriptor::TYPE_MESSAGE:
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_STRING:
      printer.Print("if (wire == NGX_PROTOBUF_WIRETYPE_LENGTH_DELIMITED) {\n");
      break;
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_FLOAT:
      printer.Print("if (wire == NGX_PROTOBUF_WIRETYPE_FIXED32) {\n");
      break;
    default:
      // we don't support groups
      printer.Print("if (wire == FIXME) {\n");
      break;
    }

    Indent(printer);

    // the actual code to unpack this field

    if (field->type() == FieldDescriptor::TYPE_MESSAGE) {

      // messages have a helper function that we call, which takes
      // care of everything.

      FullSimpleIf(printer, vars,
                   "ngx_protobuf_read_uint32(pos, end, &mlen) != NGX_OK",
                   "return NGX_ABORT;");
      printer.Print(vars, "rc = $root$__unpack_$fname$(obj, ctx, mlen);\n");
      FullSimpleIf(printer, vars, "rc != NGX_OK", "return rc;");

    } else if (field->is_repeated()) {

      // repeated non-message field

      printer.Print(vars,
                    "$ftype$ *fptr;\n"
                    "\n"
                    "fptr = $root$__add__$fname$(obj, pool);\n");

      FullSimpleIf(printer, vars, "fptr == NULL", "return NGX_ERROR;");

      switch (field->type()) {
      case FieldDescriptor::TYPE_BYTES:
      case FieldDescriptor::TYPE_STRING:
        FullCuddledIf(printer, vars,
                      "ngx_protobuf_read_string(pos, end, fptr,",
                      "(ctx->reuse_strings) ? NULL : pool) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars, "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_BOOL:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_uint32(pos, end, &flag) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "*fptr = (flag != 0);\n"
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_UINT32:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_uint32(pos, end, fptr) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars, "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_ENUM:
      case FieldDescriptor::TYPE_INT32:
        FullCuddledIf(printer, vars,
                      "ngx_protobuf_read_uint32(pos, end,",
                      "(uint32_t *)fptr) != NGX_OK",
                      "return NGX_ABORT;");
        printer.Print(vars, "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_SINT32:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_uint32(pos, end, &u32v) "
                     "!= NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "*fptr = NGX_PROTOBUF_Z32_DECODE(u32v);\n"
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_UINT64:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_uint64(pos, end, fptr) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars, "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_INT64:
        FullCuddledIf(printer, vars,
                      "ngx_protobuf_read_uint64(pos, end,",
                      "(uint64_t *)fptr) != NGX_OK",
                      "return NGX_ABORT;");
        printer.Print(vars, "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_SINT64:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_uint64(pos, end, &u64v) "
                     "!= NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "*fptr = NGX_PROTOBUF_Z64_DECODE(u64v);\n"
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_FIXED32:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_fixed32(pos, end, fptr) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars, "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_SFIXED32:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_fixed32(pos, end, &u32v) "
                     "!= NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "*fptr = NGX_PROTOBUF_Z32_DECODE(u32v);\n"
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_FLOAT:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_float(pos, end, fptr) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars, "obj->__has_$fname$ = 1;\n"); 
        break;
      case FieldDescriptor::TYPE_FIXED64:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_fixed64(pos, end, fptr) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars, "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_SFIXED64:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_fixed64(pos, end, &u64v) "
                     "!= NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "*fptr = NGX_PROTOBUF_Z64_DECODE(u64v);\n"
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_DOUBLE:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_double(pos, end, fptr) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars, "obj->__has_$fname$ = 1;\n"); 
        break;
      default:
        printer.Print(vars, "#error cannot read $ftype$\n");
        break;
      }
    } else {

      // non-repeated non-message field

      switch (field->type()) {
      case FieldDescriptor::TYPE_BYTES:
      case FieldDescriptor::TYPE_STRING:
        FullCuddledIf(printer, vars,
                      "ngx_protobuf_read_string(pos, end,",
                      "&obj->$fname$,\n"
                      "(ctx->reuse_strings) ? NULL : pool) != NGX_OK",
                      "return NGX_ABORT;");
        printer.Print(vars, "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_BOOL:
        FullCuddledIf(printer, vars,
                      "ngx_protobuf_read_uint32(pos, end,",
                      "(uint32_t *)&flag) != NGX_OK",
                      "return NGX_ABORT;");
        printer.Print(vars,
                      "obj->$fname$ = (flag != 0);\n"
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_UINT32:
        FullCuddledIf(printer, vars,
                      "ngx_protobuf_read_uint32(pos, end,",
                      "&obj->$fname$) != NGX_OK",
                      "return NGX_ABORT;");
        printer.Print(vars, "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_ENUM:
      case FieldDescriptor::TYPE_INT32:
        FullCuddledIf(printer, vars,
                      "ngx_protobuf_read_uint32(pos, end,",
                      "(uint32_t *)&obj->$fname$) != NGX_OK",
                      "return NGX_ABORT;");
        printer.Print(vars, "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_SINT32:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_uint32(pos, end, &u32v) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "obj->$fname$ = NGX_PROTOBUF_Z32_DECODE(u32v);\n"
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_UINT64:
        FullCuddledIf(printer, vars,
                      "ngx_protobuf_read_uint64(pos, end,",
                      "&obj->$fname$) != NGX_OK",
                      "return NGX_ABORT;");
        printer.Print(vars, "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_INT64:
        FullCuddledIf(printer, vars,
                      "ngx_protobuf_read_uint64(pos, end,",
                      "(uint64_t *)&obj->$fname$) != NGX_OK",
                      "return NGX_ABORT;");
        printer.Print(vars, "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_SINT64:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_uint64(pos, end, &u64v) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "obj->$fname$ = NGX_PROTOBUF_Z64_DECODE(u64v);\n"
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_FIXED32:
        FullCuddledIf(printer, vars,
                      "ngx_protobuf_read_fixed32(pos, end,",
                      "&obj->$fname$) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_SFIXED32:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_fixed32(pos, end, &u32v) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "obj->$fname$ = NGX_PROTOBUF_Z32_DECODE(u32v);\n"
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_FLOAT:
        FullCuddledIf(printer, vars,
                      "ngx_protobuf_read_float(pos, end,",
                      "&obj->$fname$) != NGX_OK",
                      "return NGX_ABORT;");
        printer.Print(vars,
                      "obj->__has_$fname$ = 1;\n"); 
        break;
      case FieldDescriptor::TYPE_FIXED64:
        FullCuddledIf(printer, vars,
                      "ngx_protobuf_read_fixed64(pos, end,",
                      "&obj->$fname$) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "obj->__has_$fname$ = 1;\n");
      case FieldDescriptor::TYPE_SFIXED64:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_fixed64(pos, end, &u64v) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "obj->$fname$ = NGX_PROTOBUF_Z64_DECODE(u64v);\n"
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_DOUBLE:
        FullCuddledIf(printer, vars,
                      "ngx_protobuf_read_double(pos, end,",
                      "&obj->$fname$) != NGX_OK",
                      "return NGX_ABORT;");
        printer.Print(vars,
                      "obj->__has_$fname$ = 1;\n"); 
        break;
      default:
        printer.Print(vars,
                      "FIXME; /* read $ftype$ */\n");
        break;
      }
    }

    if (field->is_packable() && field->options().packed()) {

      // generate code to read the field if it's packed

      Outdent(printer);
      printer.Print("} else if (wire == "
                    "NGX_PROTOBUF_WIRETYPE_LENGTH_DELIMITED) {\n");
      Indent(printer);

      FullSimpleIf(printer, vars,
                   "ngx_protobuf_read_uint32(pos, end, &mlen) != NGX_OK",
                   "return NGX_ABORT;");

      printer.Print("mend = *pos + mlen;\n");
      FullSimpleIf(printer, vars, "mend > end", "return NGX_ABORT;");
      printer.Print("while (*pos < mend) {\n");
      Indent(printer);

      printer.Print(vars,
                    "$ftype$ *fptr;\n"
                    "\n"
                    "fptr = $root$__add__$fname$(obj, pool);\n");

      FullSimpleIf(printer, vars, "fptr == NULL", "return NGX_ERROR;");

      // now read the data type

      switch (field->type()) {
      case FieldDescriptor::TYPE_BOOL:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_uint32(pos, end, &flag) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "*fptr = (flag != 0);\n"
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_UINT32:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_uint32(pos, end, fptr) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_ENUM:
      case FieldDescriptor::TYPE_INT32:
        FullCuddledIf(printer, vars,
                      "ngx_protobuf_read_uint32(pos, end,",
                      "(uint32_t *)fptr) != NGX_OK",
                      "return NGX_ABORT;");
        printer.Print(vars,
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_SINT32:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_uint32(pos, end, &u32v) "
                     "!= NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "*fptr = NGX_PROTOBUF_Z32_DECODE(u32v);\n"
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_UINT64:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_uint64(pos, end, fptr) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_INT64:
        FullCuddledIf(printer, vars,
                      "ngx_protobuf_read_uint64(pos, end,",
                      "(uint64_t *)fptr) != NGX_OK",
                      "return NGX_ABORT;");
        printer.Print(vars,
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_SINT64:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_uint64(pos, end, &u64v) "
                     "!= NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "*fptr = NGX_PROTOBUF_Z64_DECODE(u64v);\n"
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_FIXED32:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_fixed32(pos, end, fptr) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_SFIXED32:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_fixed32(pos, end, &u32v) "
                     "!= NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "*fptr = NGX_PROTOBUF_Z32_DECODE(u32v);\n"
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_FLOAT:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_float(pos, end, fptr) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "obj->__has_$fname$ = 1;\n"); 
        break;
      case FieldDescriptor::TYPE_FIXED64:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_fixed64(pos, end, fptr) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "obj->__has_$fname$ = 1;\n");
      case FieldDescriptor::TYPE_SFIXED64:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_fixed64(pos, end, &u64v) "
                     "!= NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "*fptr = NGX_PROTOBUF_Z64_DECODE(u64v);\n"
                      "obj->__has_$fname$ = 1;\n");
        break;
      case FieldDescriptor::TYPE_DOUBLE:
        FullSimpleIf(printer, vars,
                     "ngx_protobuf_read_double(pos, end, fptr) != NGX_OK",
                     "return NGX_ABORT;");
        printer.Print(vars,
                      "obj->__has_$fname$ = 1;\n"); 
        break;
      default:
        printer.Print(vars,
                      "#error cannot read packed $ftype$\n");
        break;
      }

      CloseBrace(printer);
    }

    Else(printer);
    SkipUnknown(printer);
    CloseBrace(printer);

    printer.Print("break;\n");
    Outdent(printer);
  }

  printer.Print("default:\n");
  Indent(printer);

  if (desc->extension_range_count() > 0) {
    printer.Print(vars,
                  "rc = ngx_protobuf_unpack_extension(field, wire, ctx,\n"
                  "    $root$__extensions,\n"
                  "    &obj->__extensions);\n");
    FullSimpleIf(printer, vars, "rc != NGX_OK", "return rc;");
  } else {
    SkipUnknown(printer);
  }

  printer.Print("break;\n");
  CloseBrace(printer);
  CloseBrace(printer);
  printer.Print("\n"
                "return NGX_OK;\n");
  CloseBrace(printer);
  printer.Print("\n");
}

} // namespace nginx
} // namespace compiler
} // namespace protobuf
} // namespace google
