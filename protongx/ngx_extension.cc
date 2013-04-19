#include <ngx_generator.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace nginx {

void
Generator::GenerateExtendeeDecls(const Descriptor *desc,
                                 io::Printer& printer)
{
  if (desc->extension_range_count() == 0) {
    return;
  }

  std::string root(TypedefRoot(desc->full_name()));

  printer.Print("/* $name$ extendee methods */\n"
                "\n", "name", desc->full_name());

  // import, set, add extension

  printer.Print("ngx_int_t $root$__import_extension(\n"
                "    ngx_protobuf_field_descriptor_t *desc,\n"
                "    ngx_cycle_t *cycle);\n"
                "\n"
                "ngx_int_t $root$__set_extension(\n"
                "    ngx_rbtree_t **extensions,\n"
                "    uint32_t field,\n"
                "    ngx_protobuf_value_t *val,\n"
                "    ngx_pool_t *pool);\n"
                "\n"
                "void *$root$__add_extension(\n"
                "    ngx_rbtree_t **extensions,\n"
                "    uint32_t field,\n"
                "    ngx_pool_t *pool);\n"
                "\n",
                "root", root);
}

void
Generator::GenerateExtendeeMethods(const Descriptor *desc,
                                   io::Printer& printer)
{
  if (desc->extension_range_count() == 0) {
    return;
  }

  std::string root(TypedefRoot(desc->full_name()));

  // import

  printer.Print("ngx_int_t\n"
                "$root$__import_extension(\n"
                "    ngx_protobuf_field_descriptor_t *desc,\n"
                "    ngx_cycle_t *cycle)\n"
                "{\n",
                "root", root);
  Indent(printer);
  printer.Print("return ngx_protobuf_import_extension(\n");
  Indent(printer);
  printer.Print("&$root$__extensions, desc, cycle);\n",
                "root", root);
  Outdent(printer);
  CloseBrace(printer);

  printer.Print("\n");

  // set_extension

  printer.Print("ngx_int_t\n"
                "$root$__set_extension(\n"
                "    ngx_rbtree_t **extensions,\n"
                "    uint32_t field,\n"
                "    ngx_protobuf_value_t *val,\n"
                "    ngx_pool_t *pool)\n",
                "root", root);
  OpenBrace(printer);
  printer.Print("return ngx_protobuf_set_extension(extensions,\n"
                "    $root$__extensions,\n"
                "    field, val, pool);\n",
                "root", root);
  CloseBrace(printer);
  printer.Print("\n");

  // add_extension

  printer.Print("void *\n"
                "$root$__add_extension(\n"
                "    ngx_rbtree_t **extensions,\n"
                "    uint32_t field,\n"
                "    ngx_pool_t *pool)\n",
                "root", root);
  OpenBrace(printer);
  printer.Print("void *val = NULL;\n"
                "\n"
                "ngx_protobuf_add_extension(extensions,\n"
                "    $root$__extensions,\n"
                "    field, &val, pool);\n"
                "\n"
                "return val;\n",
                "root", root);
  CloseBrace(printer);
  printer.Print("\n");
}

void
Generator::GenerateExtensionFieldDecls(const FieldDescriptor *field,
                                       io::Printer& printer)
{
  std::string mtype(TypedefRoot(field->containing_type()->full_name()));
  std::string ftype(FieldRealType(field));
  std::string name(ExtensionFieldName(field));

  std::map<std::string, std::string> vars;

  int primitive = 0;

  vars["mtype"] = mtype;
  vars["name"] = name;
  vars["fnum"] = Number(field->number());
  vars["ftype"] = ftype;
  vars["dtype"] = Type(field);
  vars["gtype"] = FieldTypeName(field);
  vars["stype"] = ExtensionSetterType(field, &primitive);
  vars["space"] = Spaces(primitive);

  printer.Print(vars,
                "#define $mtype$__has_$name$(obj) \\\n"
                "    (ngx_protobuf_get_extension(obj->__extensions, $fnum$) "
                "!= NULL)\n"
                "\n"
                "#define $mtype$__clear_$name$(obj) \\\n"
                "    ngx_protobuf_clear_extension(obj->__extensions, "
                "$fnum$);\n"
                "\n"
                "$stype$$space$$mtype$__get_$name$(\n"
                "    $mtype$_t *obj);\n"
                "\n"
                "ngx_int_t $mtype$__set_$name$(\n"
                "    $mtype$_t *obj,\n"
                "    $stype$$space$ext,\n"
                "    ngx_pool_t *pool);\n"
                "\n");

  if (primitive) {
    printer.Print(vars,
                  "$gtype$ *$mtype$__mutable_$name$(\n"
                  "    $mtype$_t *obj);\n"
                  "\n");
  }

  if (field->is_repeated()) {
    printer.Print(vars,
                  "$ftype$ *\n"
                  "$mtype$__add_$name$(\n"
                  "    $mtype$_t *obj,\n"
                  "    ngx_pool_t *pool);\n"
                  "\n");
  }
}

void
Generator::GenerateExtensionFieldMethods(const FieldDescriptor *field,
                                         io::Printer& printer)
{
  std::string mtype(TypedefRoot(field->containing_type()->full_name()));
  std::string ftype(FieldRealType(field));
  std::string name(ExtensionFieldName(field));

  std::map<std::string, std::string> vars;
  int primitive = 0;

  vars["mtype"] = mtype;
  vars["name"] = name;
  vars["fnum"] = Number(field->number());
  vars["ftype"] = ftype;
  vars["dtype"] = Type(field);
  vars["gtype"] = FieldTypeName(field);
  vars["stype"] = ExtensionSetterType(field, &primitive);
  vars["space"] = Spaces(primitive);

  // get extension

  printer.Print(vars,
                "$stype$\n"
                "$mtype$__get_$name$(\n"
                "    $mtype$_t *obj)\n");
  OpenBrace(printer);
  printer.Print(vars,
                "ngx_protobuf_value_t *val;\n"
                "\n"
                "val = ngx_protobuf_get_extension(obj->__extensions, "
                "$fnum$);\n");
  FullSimpleIf(printer, vars,
               "val == NULL || !val->exists",
               "return NULL;");

  printer.Print("\n");

  if (primitive) {
    FullSimpleIf(printer, vars,
                 "val->type != $dtype$",
                 "return 0;");
  } else {
    FullSimpleIf(printer, vars,
                 "val->type != $dtype$",
                 "return NULL;");
  }

  std::string rval;

  if (field->is_repeated()) {
    rval = "val->u.v_repeated";
  } else {
    switch (field->type()) {
    case FieldDescriptor::TYPE_MESSAGE:   rval = "val->u.v_message"; break;
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_STRING:    rval = "&val->u.v_bytes";  break;
    case FieldDescriptor::TYPE_BOOL:
    case FieldDescriptor::TYPE_ENUM:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_FIXED32:   rval = "val->u.v_uint32";  break;
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SFIXED32:  rval = "val->u.v_int32";   break;
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_FIXED64:   rval = "val->u.v_uint64";  break;
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_SFIXED64:  rval = "val->u.v_int64";   break;
    case FieldDescriptor::TYPE_FLOAT:     rval = "val->u.v_float";   break;
    case FieldDescriptor::TYPE_DOUBLE:    rval = "val->u.v_double";  break;
    default:                              rval = "FIXME";            break;
    }
  }

  printer.Print("\n"
                "return $rval$;\n",
                "rval", rval);

  CloseBrace(printer);
  printer.Print("\n");

  // set extension

  printer.Print(vars,
                "ngx_int_t\n"
                "$mtype$__set_$name$(\n"
                "    $mtype$_t *obj,\n"
                "    $stype$$space$ext,\n"
                "    ngx_pool_t *pool)\n");
  OpenBrace(printer);
  printer.Print(vars,
                "ngx_protobuf_value_t val;\n"
                "\n"
                "val.type = $dtype$;\n");

  if (field->is_repeated()) {
    printer.Print("val.u.v_repeated = ext;\n"
                  "val.exists = (ext != NULL);\n");
  } else if (field->type() == FieldDescriptor::TYPE_MESSAGE) {
    printer.Print("val.u.v_message = ext;\n"
                  "val.exists = (ext != NULL);\n");
  } else if (field->type() == FieldDescriptor::TYPE_BYTES ||
             field->type() == FieldDescriptor::TYPE_STRING) {
    SimpleIf(printer, vars, "ext != NULL");
    printer.Print("val.u.v_bytes.len = ext->len;\n"
                  "val.u.v_bytes.data = ext->data;\n"
                  "val.exists = 1;\n");
    Else(printer);
    printer.Print("val.exists = 0;\n");
    CloseBrace(printer);
  } else {
    printer.Print("val.exists = 1;\n");
  }

  printer.Print(vars,
                "\n"
                "return $mtype$__set_extension(\n"
                "    &obj->__extensions, $fnum$, &val, pool);\n");

  CloseBrace(printer);
  printer.Print("\n");

  // add extension (for repeated extension fields)

  if (field->is_repeated()) {
    printer.Print(vars,
                  "$ftype$ *\n"
                  "$mtype$__add_$name$(\n"
                  "    $mtype$_t *obj,\n"
                  "    ngx_pool_t *pool)\n");
    OpenBrace(printer);
    printer.Print(vars,
                  "return $mtype$__add_extension(\n"
                  "    &obj->__extensions, $fnum$, pool);\n");
    CloseBrace(printer);
    printer.Print("\n");
  }

  
}

void
Generator::GenerateExtensionMethods(const Descriptor *desc,
                                    io::Printer& printer)
{
  for (int i = 0; i < desc->field_count(); ++i) {
    const FieldDescriptor *field = desc->field(i);

    if (field->is_extension()) {
      GenerateExtensionFieldMethods(field, printer);
    }
  }
}

} // namespace nginx
} // namespace compiler
} // namespace protobuf
} // namespace google
