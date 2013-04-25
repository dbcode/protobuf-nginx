#include <ngx_flags.h>
#include <ngx_generator.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace nginx {

void
Generator::GenerateIsInitialized(const Descriptor *desc, io::Printer& printer)
{
  Flags flags(desc);

  std::map<std::string, std::string> vars;

  vars["name"] = desc->full_name();
  vars["root"] = TypedefRoot(desc->full_name());
  vars["type"] = StructType(desc->full_name());

  printer.Print(vars,
		"ngx_int_t\n"
		"$root$__is_initialized(\n"
		"    $type$ *obj)\n");
  OpenBrace(printer);

  if (flags.has_required()) {
    int rcnt = 0;

    printer.Print("return (");
    for (int i = 0; i < desc->field_count(); ++i) {
      const FieldDescriptor *field = desc->field(i);

      if (field->is_required()) {
	vars["fname"] = field->name();

	if (rcnt++ > 0) {
	  printer.Print("\n    && ");
	}
	printer.Print(vars, "$root$__has_$fname$(obj)");
      }
    }
    printer.Print(");\n");
  } else {
    printer.Print("return 1; /* no required fields */\n");
  }

  CloseBrace(printer);
  printer.Print("\n");
}


} // namespace nginx
} // namespace compiler
} // namespace protobuf
} // namespace google
