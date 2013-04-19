#include <ngx_flags.h>
#include <ngx_generator.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace nginx {

void
Generator::GenerateAdd(const Descriptor* desc, io::Printer& printer)
{
  Flags flags(desc);

  if (!flags.has_array()) {
    return;
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

    if (field->is_repeated()) {
      vars["field"] = FieldRealType(field);
      vars["fname"] = field->name();

      printer.Print(vars,
                    "$field$ *\n"
                    "$root$__add__$fname$(\n"
                    "    $type$ *obj,\n"
                    "    ngx_pool_t *pool)\n"
                    "{\n");
      Indent(printer);

      printer.Print(vars,
                    "$field$ *ret;\n"
                    "\n"
                    "ret = ngx_protobuf_push_array(&obj->$fname$, pool,\n"
                    "    sizeof($field$));\n"
                    "if (ret != NULL) {\n");
      Indent(printer);
      printer.Print(vars,
                    "obj->__has_$fname$ = 1;\n");
      Outdent(printer);
      printer.Print("}\n"
                    "\n"
                    "return ret;\n");
      
      Outdent(printer);
      printer.Print("}\n"
                    "\n");
    }
  }
}

} // namespace nginx
} // namespace compiler
} // namespace protobuf
} // namespace google
