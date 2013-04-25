#include <ngx_generator.h>
#include <google/protobuf/descriptor.pb.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace nginx {

void
Generator::GenerateMethodDecls(const Descriptor *desc, io::Printer& printer)
{
  for (int i = 0; i < desc->nested_type_count(); ++i) {
    GenerateMethodDecls(desc->nested_type(i), printer);
  }

  std::map<std::string, std::string> vars;

  vars["name"] = desc->full_name();
  vars["root"] = TypedefRoot(desc->full_name());
  vars["type"] = StructType(desc->full_name());

  printer.Print(vars,
                "/* $name$ field methods */\n"
                "\n");

  for (int i = 0; i < desc->field_count(); ++i) {
    const FieldDescriptor *field = desc->field(i);

    vars["field"] = FieldRealType(field);
    vars["fname"] = field->name();

    // set

    if (field->is_repeated() ||
        field->type() == FieldDescriptor::TYPE_MESSAGE) {
      printer.Print(vars,
                    "#define $root$__set_$fname$(obj, val) \\\n"
                    "    NGX_PROTOBUF_SET_MEMBER(obj, $fname$, val)\n"
                    "\n");
    } else if (field->type() == FieldDescriptor::TYPE_BYTES ||
               field->type() == FieldDescriptor::TYPE_STRING) {
      printer.Print(vars,
                    "#define $root$__set_$fname$(obj, val) \\\n"
                    "    NGX_PROTOBUF_SET_STRING(obj, $fname$, val)\n"
                    "\n");
    } else {
      printer.Print(vars,
                    "#define $root$__set_$fname$(obj, val) \\\n"
                    "    NGX_PROTOBUF_SET_NUMBER(obj, $fname$, val)\n"
                    "\n");
    }

    // add (if repeated)

    if (field->is_repeated()) {
      printer.Print(vars,
                    "$field$ *\n"
                    "$root$__add__$fname$(\n"
                    "    $type$ *obj,\n"
                    "    ngx_pool_t *pool);\n"
                    "\n");
    }

    // clear

    if (field->is_repeated() ||
        field->type() == FieldDescriptor::TYPE_MESSAGE) {
      printer.Print(vars,
                    "#define $root$__clear_$fname$(obj) \\\n"
                    "    NGX_PROTOBUF_CLEAR_MEMBER(obj, $fname$)\n"
                    "\n");
    } else if (field->type() == FieldDescriptor::TYPE_BYTES ||
               field->type() == FieldDescriptor::TYPE_STRING) {
      printer.Print(vars,
                    "#define $root$__clear_$fname$(obj) \\\n"
                    "    NGX_PROTOBUF_CLEAR_STRING(obj, $fname$)\n"
                    "\n");
    } else {
      printer.Print(vars,
                    "#define $root$__clear_$fname$(obj) \\\n"
                    "    NGX_PROTOBUF_CLEAR_NUMBER(obj, $fname$)\n"
                    "\n");
    }

    // has

    if (field->is_repeated()) {
      printer.Print(vars,
                    "#define $root$__has_$fname$(obj) \\\n"
                    "    (NGX_PROTOBUF_HAS_FIELD(obj, $fname$) && \\\n"
                    "     (obj)->$fname$->nelts > 0)\n"
                    "\n");
    } else {
      printer.Print(vars,
                    "#define $root$__has_$fname$(obj) \\\n"
                    "    (NGX_PROTOBUF_HAS_FIELD(obj, $fname$))\n"
                    "\n");
    }
  }

  printer.Print(vars,
                "/* $name$ message methods */\n"
                "\n"
                "#define $root$__alloc(pool) \\\n"
                "    ngx_pcalloc(pool, \\\n"
                "    sizeof($type$))\n"
                "\n"
                "#define $root$__clear(obj) \\\n"
                "    ngx_memzero(obj, sizeof($type$))\n"
                "\n"
		"ngx_int_t $root$__is_initialized(\n"
		"    $type$ *obj);\n"
		"\n"
                "ngx_int_t $root$__unpack(\n"
                "    $type$ *obj,\n"
                "    ngx_protobuf_context_t *ctx);\n"
                "\n"
                "size_t $root$__size(\n"
                "    $type$ *obj);\n"
                "\n"
                "ngx_int_t $root$__pack(\n"
                "    $type$ *obj,\n"
                "    ngx_protobuf_context_t *ctx);\n"
                "\n");

  if (desc->extension_range_count() > 0) {
    GenerateExtendeeDecls(desc, printer);
  }
}

void
Generator::GenerateMethods(const Descriptor* desc, io::Printer& printer)
{
  for (int i = 0; i < desc->nested_type_count(); ++i) {
    GenerateMethods(desc->nested_type(i), printer);
  }

  // field-level methods for the message

  GenerateAdd(desc, printer);

  if (desc->extension_range_count() > 0) {
    std::string root(TypedefRoot(desc->full_name()));

    printer.Print("/* extension registry for $name$ */\n"
                  "\n"
                  "static ngx_rbtree_t *$root$__extensions = NULL;\n"
                  "\n", "name", desc->full_name(), "root", root);
  }

  // message-level methods

  printer.Print("/* $name$ message methods */\n"
                "\n", "name", desc->full_name());

  GenerateIsInitialized(desc, printer);
  GenerateUnpack(desc, printer);
  GenerateSize(desc, printer);
  GeneratePack(desc, printer);

  if (desc->extension_range_count() > 0) {
    printer.Print("/* $name$ extendee methods */\n"
                  "\n", "name", desc->full_name());

    GenerateExtendeeMethods(desc, printer);
  }

  if (HasExtensionFields(desc)) {
    printer.Print("/* $name$ extension field methods */\n"
                  "\n", "name", desc->full_name());

    GenerateExtensionMethods(desc, printer);
  }
}

} // namespace nginx
} // namespace compiler
} // namespace protobuf
} // namespace google
