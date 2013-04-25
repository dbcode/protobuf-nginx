#include <ngx_flags.h>
#include <ngx_generator.h>
#include <google/protobuf/descriptor.pb.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace nginx {

struct ExtensionRangeSorter {
  bool operator()(const Descriptor::ExtensionRange *left,
                  const Descriptor::ExtensionRange *right) const
  {
    return (left->start < right->start);
  }
};

struct FieldDescriptorSorter {
  bool operator()(const FieldDescriptor *left,
                  const FieldDescriptor *right) const
  {
    return (left->number() < right->number());
  }
};

void
Generator::GeneratePackField(const FieldDescriptor *field,
                             io::Printer& printer)
{
  std::map<std::string, std::string> vars;

  vars["fname"] = field->name();
  vars["ftype"] = FieldRealType(field);
  vars["fnum"] = Number(field->number());

  if (field->is_repeated()) {
    CuddledIf(printer, vars,
              "obj->__has_$fname$",
              "&& obj->$fname$ != NULL\n"
              "&& obj->$fname$->nelts > 0");
  } else if (field->type() == FieldDescriptor::TYPE_MESSAGE) {
    CuddledIf(printer, vars,
              "obj->__has_$fname$",
              "&& obj->$fname$ != NULL");
  } else {
    SimpleIf(printer, vars, "obj->__has_$fname$");
  }

  if (field->is_repeated()) {
    printer.Print(vars,
                  "$ftype$ *vals = obj->$fname$->elts;\n"
                  "ngx_uint_t i;\n"
                  "\n");

    if (field->is_packable() && field->options().packed()) {
      vars["root"] = TypedefRoot(field->containing_type()->full_name());
      printer.Print(vars,
                    "n = $root$_$fname$__packed_size(obj->$fname$);\n"
                    "ctx->buffer.pos = ngx_protobuf_write_message_header(\n"
                    "    ctx->buffer.pos, n, $fnum$);\n"
                    "\n");
    }

    printer.Print(vars,
                  "for (i = 0; i < obj->$fname$->nelts; ++i) {\n");
    Indent(printer);
    switch (field->type()) {
    case FieldDescriptor::TYPE_MESSAGE:
      vars["froot"] = TypedefRoot(field->message_type()->full_name());
      printer.Print(vars,
                    "n = $froot$__size(vals + i);\n"
                    "ctx->buffer.pos = ngx_protobuf_write_message_header(\n"
                    "    ctx->buffer.pos, n, $fnum$);\n"
                    "$froot$__pack(vals + i, ctx);\n");
      break;
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_STRING:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_string_field(\n"
                    "    ctx->buffer.pos, vals + i, $fnum$);\n");
      break;
    case FieldDescriptor::TYPE_BOOL:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_uint32_field(\n"
                    "    ctx->buffer.pos, (vals[i] != 0), $fnum$);\n");
      break;
    case FieldDescriptor::TYPE_ENUM:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_INT32:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_uint32_field(\n"
                    "    ctx->buffer.pos, vals[i], $fnum$);\n");
      break;
    case FieldDescriptor::TYPE_SINT32:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_uint32_field(\n"
                    "    ctx->buffer.pos, "
                    "NGX_PROTOBUF_Z32_ENCODE(vals[i]), $fnum$);\n");
      break;
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_INT64:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_uint64_field(\n"
                    "    ctx->buffer.pos, vals[i], $fnum$);\n");
      break;
    case FieldDescriptor::TYPE_SINT64:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_uint64_field(\n"
                    "    ctx->buffer.pos, "
                    "NGX_PROTOBUF_Z64_ENCODE(vals[i]), $fnum$);\n");
      break;
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_fixed32_field(\n"
                    "    ctx->buffer.pos, vals[i], $fnum$);\n");
      break;
    case FieldDescriptor::TYPE_FLOAT:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_float_field(\n"
                    "    ctx->buffer.pos, vals[i], $fnum$);\n");
      break;
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_fixed64_field(\n"
                    "    ctx->buffer.pos, vals[i], $fnum$);\n");
      break;
    case FieldDescriptor::TYPE_DOUBLE:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_double_field(\n"
                    "    ctx->buffer.pos, vals[i], $fnum$);\n");
      break;
    default:
      printer.Print(vars,
                    "/* pack repeated $ftype$ */\n"
                    "ctx->buffer.pos = FIXME;\n");
      break;
    }
    Outdent(printer);
    printer.Print("}\n");
  } else {
    switch (field->type()) {
    case FieldDescriptor::TYPE_MESSAGE:
      vars["froot"] = TypedefRoot(field->message_type()->full_name());
      printer.Print(vars,
                    "n = $froot$__size(obj->$fname$);\n"
                    "ctx->buffer.pos = ngx_protobuf_write_message_header(\n"
                    "    ctx->buffer.pos, n, $fnum$);\n"
                    "$froot$__pack(obj->$fname$, ctx);\n");
      break;
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_STRING:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_string_field(\n"
                    "    ctx->buffer.pos,\n"
                    "    &obj->$fname$,\n"
                    "    $fnum$);\n");
      break;
    case FieldDescriptor::TYPE_BOOL:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_uint32_field(\n"
                    "    ctx->buffer.pos,\n"
                    "    (obj->$fname$ != 0),\n"
                    "    $fnum$);\n");
      break;
    case FieldDescriptor::TYPE_ENUM:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_INT32:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_uint32_field(\n"
                    "    ctx->buffer.pos,\n"
                    "    obj->$fname$,\n"
                    "    $fnum$);\n");
      break;
    case FieldDescriptor::TYPE_SINT32:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_uint32_field(\n"
                    "    ctx->buffer.pos,\n"
                    "    NGX_PROTOBUF_Z32_ENCODE(obj->$fname$),\n"
                    "    $fnum$);\n");
      break;
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_INT64:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_uint64_field(\n"
                    "    ctx->buffer.pos,\n"
                    "    obj->$fname$,\n"
                    "    $fnum$);\n");
      break;
    case FieldDescriptor::TYPE_SINT64:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_uint64_field(\n"
                    "    ctx->buffer.pos,\n"
                    "    NGX_PROTOBUF_Z64_ENCODE(obj->$fname$),\n"
                    "    $fnum$);\n");
      break;
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_fixed32_field(\n"
                    "    ctx->buffer.pos,\n"
                    "    obj->$fname$,\n"
                    "    $fnum$);\n");
      break;
    case FieldDescriptor::TYPE_FLOAT:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_float_field(\n"
                    "    ctx->buffer.pos,\n"
                    "    obj->$fname$,\n"
                    "    $fnum$);\n");
      break;
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_fixed64_field(\n"
                    "    ctx->buffer.pos,\n"
                    "    obj->$fname$,\n"
                    "    $fnum$);\n");
      break;
    case FieldDescriptor::TYPE_DOUBLE:
      printer.Print(vars,
                    "ctx->buffer.pos = ngx_protobuf_write_double_field(\n"
                    "    ctx->buffer.pos,\n"
                    "    obj->$fname$,\n"
                    "    $fnum$);\n");
      break;
    default:
      printer.Print(vars,
                    "ctx->buffer.pos = FIXME; /* pack $ftype$ */\n");
      break;
    }
  }

  Outdent(printer);
  printer.Print("}\n");
}

void
Generator::GeneratePackRange(const Descriptor::ExtensionRange *range,
                             io::Printer& printer)
{
  std::map<std::string, std::string> vars;

  vars["lower"] = Number(range->start);
  vars["upper"] = Number(range->end);

  SimpleIf(printer, vars, "obj->__extensions != NULL");
  printer.Print(vars,
                "rc = ngx_protobuf_pack_extensions(obj->__extensions,\n"
                "    $lower$, $upper$, ctx);\n");
  FullSimpleIf(printer, vars, "rc != NGX_OK", "return rc;");
  CloseBrace(printer);
}

void
Generator::GeneratePackUnknown(const Descriptor *desc, io::Printer& printer)
{
  printer.Print("\n"
		"if (obj->__unknown != NULL\n"
		"    && obj->__unknown->elts != NULL\n"
		"    && obj->__unknown->nelts > 0)\n");
  OpenBrace(printer);
  printer.Print("ngx_protobuf_unknown_field_t  *unk = obj->__unknown->elts;\n"
		"ngx_uint_t                     i;\n"
		"\n"
		"for (i = 0; i < obj->__unknown->nelts; ++i) ");  
  OpenBrace(printer);
  printer.Print("rc = ngx_protobuf_pack_unknown_field(unk + i, ctx);\n"
		"if (rc != NGX_OK) ");
  OpenBrace(printer);
  printer.Print("return rc;\n");
  CloseBrace(printer);
  CloseBrace(printer);
  CloseBrace(printer);
}
		
void
Generator::GeneratePack(const Descriptor* desc, io::Printer& printer)
{
  std::map<std::string, std::string> vars;

  vars["name"] = desc->full_name();
  vars["root"] = TypedefRoot(desc->full_name());
  vars["type"] = StructType(desc->full_name());

  printer.Print(vars,
                "ngx_int_t\n"
                "$root$__pack(\n"
                "    $type$ *obj,\n"
                "    ngx_protobuf_context_t *ctx)\n"
                "{\n");
  Indent(printer);

  printer.Print(vars,
                "size_t     size = $root$__size(obj);\n");

  Flags flags(desc);

  if (flags.has_message() || flags.has_packed()) {
    printer.Print("size_t     n;\n");
  }

  if (desc->extension_range_count() > 0 || HasUnknownFields(desc)) {
    printer.Print("ngx_int_t  rc;\n");
  }

  printer.Print("\n");
  FullSimpleIf(printer, vars, "size == 0", "return NGX_OK;");
  printer.Print("\n");
  FullSimpleIf(printer, vars,
               "ctx->buffer.pos + size > ctx->buffer.last",
               "return NGX_ABORT;");
  printer.Print("\n");

  // fields and extension ranges may be declared in any order,
  // but we should serialize in canonical order

  std::vector<const Descriptor::ExtensionRange *> extensions;
  std::vector<const FieldDescriptor *> fields;

  for (int i = 0; i < desc->field_count(); ++i) {
    fields.push_back(desc->field(i));
  }

  for (int i = 0; i < desc->extension_range_count(); ++i) {
    extensions.push_back(desc->extension_range(i));
  }

  std::sort(extensions.begin(), extensions.end(), ExtensionRangeSorter());
  std::sort(fields.begin(), fields.end(), FieldDescriptorSorter());
  
  std::vector<const Descriptor::ExtensionRange *>::const_iterator ei;
  std::vector<const FieldDescriptor *>::const_iterator fi;

  ei = extensions.begin();
  fi = fields.begin();

  while (ei != extensions.end() || fi != fields.end()) {
    if (ei == extensions.end()) {
      GeneratePackField(*fi++, printer);
    } else if (fi == fields.end()) {
      GeneratePackRange(*ei++, printer);
    } else if ((*fi)->number() < (*ei)->start) {
      GeneratePackField(*fi++, printer);
    } else {
      GeneratePackRange(*ei++, printer);
    }
  }

  if (HasUnknownFields(desc)) {
    GeneratePackUnknown(desc, printer);
  }

  printer.Print("\n"
                "return NGX_OK;\n");

  CloseBrace(printer);

  printer.Print("\n");
}

} // namespace nginx
} // namespace compiler
} // namespace protobuf
} // namespace google
