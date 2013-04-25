#include <ngx_flags.h>
#include <ngx_generator.h>
#include <google/protobuf/descriptor.pb.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace nginx {

void
Generator::GenerateSize(const Descriptor* desc, io::Printer& printer)
{
  std::map<std::string, std::string> vars;

  vars["name"] = desc->full_name();
  vars["root"] = TypedefRoot(desc->full_name());
  vars["type"] = StructType(desc->full_name());

  // statics for packed field size calcs.  this is independent of field
  // number and just calculates the payload size (of the actual data).

  for (int i = 0; i < desc->field_count(); ++i) {
    const FieldDescriptor *field = desc->field(i);

    if (field->is_packable() && field->options().packed()) {
      bool fixed = false;

      vars["ffull"] = field->full_name();
      vars["fname"] = field->name();
      vars["ftype"] = FieldRealType(field);

      printer.Print(vars,
                    "static size_t\n"
                    "$root$_$fname$__packed_size(ngx_array_t *a)\n"
                    "{\n");
      Indent(printer);
      
      switch (field->type()) {
      case FieldDescriptor::TYPE_FIXED32:
      case FieldDescriptor::TYPE_SFIXED32:
      case FieldDescriptor::TYPE_FLOAT:
        printer.Print("return (a && a->nelts > 0) ? a->nelts * 4 : 0;\n");
        fixed = true;
        break;
      case FieldDescriptor::TYPE_FIXED64:
      case FieldDescriptor::TYPE_SFIXED64:
      case FieldDescriptor::TYPE_DOUBLE:
        printer.Print("return (a && a->nelts > 0) ? a->nelts * 8 : 0;\n");
        fixed = true;
        break;
      default:
        // continue with the codegen
        break;
      }

      if (!fixed) {
        printer.Print(vars,
                      "size_t size = 0;\n"
                      "ngx_uint_t i;\n"
                      "$ftype$ *fptr;\n"
                      "\n");

        SimpleIf(printer, vars,
                 "a != NULL && a->nelts > 0");
        printer.Print(vars,
                      "fptr = a->elts;\n"
                      "for (i = 0; i < a->nelts; ++i) {\n");
        Indent(printer);

        switch (field->type()) {
        case FieldDescriptor::TYPE_BOOL:
        case FieldDescriptor::TYPE_ENUM:
        case FieldDescriptor::TYPE_UINT32:
        case FieldDescriptor::TYPE_INT32:
          printer.Print("size += ngx_protobuf_size_uint32(fptr[i]);\n");
          break;
        case FieldDescriptor::TYPE_SINT32:
          printer.Print("size += ngx_protobuf_size_sint32(fptr[i]);\n");
          break;
        case FieldDescriptor::TYPE_UINT64:
        case FieldDescriptor::TYPE_INT64:
          printer.Print("size += ngx_protobuf_size_uint32(fptr[i]);\n");
          break;
        case FieldDescriptor::TYPE_SINT64:
          printer.Print("size += ngx_protobuf_size_sint64(fptr[i]);\n");
          break;
        default:
          printer.Print(vars,
                        "#error $ffull$ cannot be a packed field");
          break;
        }

        CloseBrace(printer);
        CloseBrace(printer);
        printer.Print("\n"
                      "return size;\n");
      }
      CloseBrace(printer);
      printer.Print("\n");
    }
  }

  printer.Print(vars,
                "size_t\n"
                "$root$__size(\n"
                "     $type$ *obj)\n"
                "{\n");
  Indent(printer);

  Flags flags(desc);
  bool  iterates = false;

  for (int i = 0; i < desc->field_count(); ++i) {
    const FieldDescriptor *field = desc->field(i);

    if (field->is_repeated() && !IsFixedWidth(field) &&
        !(field->is_packable() && field->options().packed())) {
      iterates = true;
      break;
    }
  }

  printer.Print("size_t      size = 0;\n");
  if (flags.has_packed() || flags.has_message()) {
    printer.Print("size_t      n;\n");
  }
  if (iterates || HasUnknownFields(desc)) {
    // we need to iterate any repeated non-fixed field or unknown fields
    printer.Print("ngx_uint_t  i;\n");
  }
  printer.Print("\n");

  for (int i = 0; i < desc->field_count(); ++i) {
    const FieldDescriptor *field = desc->field(i);

    vars["fname"] = field->name();
    vars["ftype"] = FieldRealType(field);
    vars["fnum"] = Number(field->number());

    if (field->is_repeated()) {

      CuddledIf(printer, vars,
                "obj->__has_$fname$",
                "&& obj->$fname$ != NULL\n"
                "&& obj->$fname$->nelts > 0");

      if (field->is_packable() && field->options().packed()) {
        printer.Print(vars,
                      "n = $root$_$fname$__packed_size(\n");
        Indented(printer, vars, "obj->$fname$);\n");

        printer.Print("\n");

        printer.Print(vars,
                      "size += ngx_protobuf_size_message_field(n, $fnum$);\n");

      } else if (IsFixedWidth(field)) {

        // size calculation of a non-packed repeated fixed-width field

        switch (field->type()) {
        case FieldDescriptor::TYPE_FIXED32:
        case FieldDescriptor::TYPE_SFIXED32:
        case FieldDescriptor::TYPE_FLOAT:
          printer.Print(vars,
                        "size += obj->$fname$->nelts *\n"
                        "    ngx_protobuf_size_fixed32_field($fnum$);\n");
          break;
        case FieldDescriptor::TYPE_FIXED64:
        case FieldDescriptor::TYPE_SFIXED64:
        case FieldDescriptor::TYPE_DOUBLE:
          printer.Print(vars,
                        "size += obj->$fname$->nelts *\n"
                        "    ngx_protobuf_size_fixed64_field($fnum$);\n");
          break;
        default:
          break;
        }
      } else {

        // size calculation of a non-packed repeated non-fixed width field

        printer.Print(vars,
                      "$ftype$ *vals = obj->$fname$->elts;\n"
                      "\n"
                      "for (i = 0; i < obj->$fname$->nelts; ++i) {\n");
        Indent(printer);

        switch (field->type()) {
        case FieldDescriptor::TYPE_MESSAGE:
          vars["froot"] = TypedefRoot(field->message_type()->full_name());
          printer.Print(vars,
                        "n = $froot$__size(vals + i);\n"
                        "size += ngx_protobuf_size_message_field("
                        "n, $fnum$);\n");
          break;
        case FieldDescriptor::TYPE_BYTES:
        case FieldDescriptor::TYPE_STRING:
          printer.Print(vars,
                        "size += ngx_protobuf_size_string_field("
                        "vals + i, $fnum$);\n");
          break;
        case FieldDescriptor::TYPE_BOOL:
          printer.Print(vars,
                        "size += ngx_protobuf_size_uint32_field("
                        "(vals[i] != 0), $fnum$);\n");
          break;
        case FieldDescriptor::TYPE_ENUM:
        case FieldDescriptor::TYPE_UINT32:
        case FieldDescriptor::TYPE_INT32:
          printer.Print(vars,
                        "size += ngx_protobuf_size_uint32_field("
                        "vals[i], $fnum$);\n");
          break;
        case FieldDescriptor::TYPE_SINT32:
          printer.Print(vars,
                        "size += ngx_protobuf_size_sint32_field("
                        "vals[i], $fnum$);\n");
          break;
        case FieldDescriptor::TYPE_UINT64:
        case FieldDescriptor::TYPE_INT64:
          printer.Print(vars,
                        "size += ngx_protobuf_size_uint64_field("
                        "vals[i], $fnum$);\n");
          break;
        case FieldDescriptor::TYPE_SINT64:
          printer.Print(vars,
                        "size += ngx_protobuf_size_sint64_field("
                        "vals[i], $fnum$);\n");
          break;
        case FieldDescriptor::TYPE_FIXED32:
        case FieldDescriptor::TYPE_SFIXED32:
        case FieldDescriptor::TYPE_FLOAT:
          printer.Print(vars,
                        "size += ngx_protobuf_size_fixed32_field($fnum$);\n");
          break;
        case FieldDescriptor::TYPE_FIXED64:
        case FieldDescriptor::TYPE_SFIXED64:
        case FieldDescriptor::TYPE_DOUBLE:
          printer.Print(vars,
                        "size += ngx_protobuf_size_fixed64_field($fnum$);\n");
          break;
        default:
          printer.Print(vars,
                        "size += FIXME; /* size $ftype$ */\n");
          break;
        }

        Outdent(printer);
        printer.Print("}\n");
      }
      Outdent(printer);
      printer.Print("}\n");
    } else {
      switch (field->type()) {
      case FieldDescriptor::TYPE_MESSAGE:
        vars["froot"] = TypedefRoot(field->message_type()->full_name());
        FullCuddledIf(printer, vars,
                      "obj->__has_$fname$",
                      "&& obj->$fname$ != NULL",
                      "n = $froot$__size(obj->$fname$);\n"
                      "size += ngx_protobuf_size_message_field(n, $fnum$);");
        break;
      case FieldDescriptor::TYPE_BYTES:
      case FieldDescriptor::TYPE_STRING:
        FullSimpleIf(printer, vars,
                     "obj->__has_$fname$",
                     "size += ngx_protobuf_size_string_field("
                     "&obj->$fname$, $fnum$);");
        break;
      case FieldDescriptor::TYPE_BOOL:
        FullSimpleIf(printer, vars,
                     "obj->__has_$fname$",
                     "size += ngx_protobuf_size_uint32_field("
                     "(obj->$fname$ != 0), $fnum$);");
        break;
      case FieldDescriptor::TYPE_ENUM:
      case FieldDescriptor::TYPE_UINT32:
      case FieldDescriptor::TYPE_INT32:
        FullSimpleIf(printer, vars,
                     "obj->__has_$fname$",
                     "size += ngx_protobuf_size_uint32_field("
                     "obj->$fname$, $fnum$);");
        break;
      case FieldDescriptor::TYPE_SINT32:
        FullSimpleIf(printer, vars,
                     "obj->__has_$fname$",
                     "size += ngx_protobuf_size_sint32_field("
                     "obj->$fname$, $fnum$);");
        break;
      case FieldDescriptor::TYPE_UINT64:
      case FieldDescriptor::TYPE_INT64:
        FullSimpleIf(printer, vars,
                     "obj->__has_$fname$",
                     "size += ngx_protobuf_size_uint64_field("
                     "obj->$fname$, $fnum$);");
        break;
      case FieldDescriptor::TYPE_SINT64:
        FullSimpleIf(printer, vars,
                     "obj->__has_$fname$",
                     "size += ngx_protobuf_size_sint64_field("
                     "obj->$fname$, $fnum$);");
        break;
      case FieldDescriptor::TYPE_FIXED32:
      case FieldDescriptor::TYPE_SFIXED32:
      case FieldDescriptor::TYPE_FLOAT:
        FullSimpleIf(printer, vars,
                     "obj->__has_$fname$",
                     "size += ngx_protobuf_size_fixed32_field($fnum$);");
        break;
      case FieldDescriptor::TYPE_FIXED64:
      case FieldDescriptor::TYPE_SFIXED64:
      case FieldDescriptor::TYPE_DOUBLE:
        FullSimpleIf(printer, vars,
                     "obj->__has_$fname$",
                     "size += ngx_protobuf_size_fixed64_field($fnum$);");
        break;
      default:
        printer.Print(vars,
                      "size += FIXME; /* size $ftype$ */\n");
        break;
      }
    }
  }

  if (desc->extension_range_count() > 0) {
    FullSimpleIf(printer, vars,
                 "obj->__extensions != NULL",
                 "size += ngx_protobuf_size_extensions(obj->__extensions);");
  }

  if (HasUnknownFields(desc)) {
    printer.Print("\n");
    CuddledIf(printer, vars,
	      "obj->__unknown != NULL",
	      "&& obj->__unknown->elts != NULL\n"
	      "&& obj->__unknown->nelts > 0");
    printer.Print("ngx_protobuf_unknown_field_t *unk = "
		  "obj->__unknown->elts;\n"
		  "\n"
		  "for (i = 0; i < obj->__unknown->nelts; ++i) ");
    OpenBrace(printer);
    printer.Print("size += ngx_protobuf_size_unknown_field(unk + i);\n");
    CloseBrace(printer);
    CloseBrace(printer);
  }

  printer.Print("\n"
                "return size;\n");

  CloseBrace(printer);
  printer.Print("\n");
}

} // namespace nginx
} // namespace compiler
} // namespace protobuf
} // namespace google
