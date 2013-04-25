#include <ngx_generator.h>
#include <google/protobuf/descriptor.pb.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace nginx {

void
Generator::GenerateEnumDecl(const EnumDescriptor* desc, io::Printer& printer)
{
  std::string type(EnumType(desc->full_name()));
  unsigned int maxname = 0;
  unsigned int maxnumb = 0;

  // if we don't align the '=', what good are we?
  for (int i = 0; i < desc->value_count(); ++i) {
    const EnumValueDescriptor *value = desc->value(i);
    std::string name = EnumValue(value->full_name());
    std::string numb = Number(value->number());

    if (name.length() > maxname) {
      maxname = name.length();
    } 
    if (numb.length() > maxnumb) {
      maxnumb = numb.length();
    } 
  }

  printer.Print("/* $type$ */\n"
                "\n"
                "typedef enum {\n",
                "type", desc->full_name());
  Indent(printer);

  for (int i = 0; i < desc->value_count(); ++i) {
    const EnumValueDescriptor *value = desc->value(i);
    std::string name = EnumValue(value->full_name());
    std::string numb = Number(value->number());
    std::map<std::string, std::string> vars;

    vars["name"] = name;
    vars["nspace"] = Spaces(maxname - name.length());
    vars["vspace"] = Spaces(maxnumb - numb.length());
    vars["value"] = Number(value->number());
    printer.Print(vars, "$name$$nspace$ = $vspace$$value$");
    if (i < desc->value_count() - 1) {
      printer.Print(",");
    }
    printer.Print("\n");
  }

  Outdent(printer);
  printer.Print("} $type$;\n"
                "\n",
                "type", type);
}

void
Generator::GenerateTypedef(const Descriptor* desc, io::Printer& printer)
{
  for (int i = 0; i < desc->enum_type_count(); ++i) {
    GenerateEnumDecl(desc->enum_type(i), printer);
  }

  for (int i = 0; i < desc->nested_type_count(); ++i) {
    GenerateTypedef(desc->nested_type(i), printer);
  }

  // now we can generate this typedef

  std::string type(StructType(desc->full_name()));
  std::string root(TypedefRoot(desc->full_name()));

  unsigned int maxtype = 0;
  unsigned int maxname = 0;
  bool hasptr = false;
  bool hasself = false;
  bool unknown = HasUnknownFields(desc);

  if (desc->extension_range_count() > 0) {
    maxtype = sizeof("ngx_rbtree_t") - 1;
    hasptr = true;
  } else if (unknown) {
    maxtype = sizeof("ngx_array_t") - 1;
    hasptr = true;
  } else {
    maxtype = sizeof("uint32_t") - 1;
  }

  for (int i = 0; i < desc->field_count(); ++i) {
    const FieldDescriptor *field = desc->field(i);
    std::string ftype = FieldTypeName(field);

    if (field->is_optional() && FieldRealType(field) == type) {
      // type contains a pointer to itself
      hasself = true;
    }

    if (ftype.length() > maxtype) {
      maxtype = ftype.length();
    }
    if (field->name().length() > maxname) {
      maxname = field->name().length();
    }
    if (hasptr == false && FieldIsPointer(field)) {
      hasptr = true;
    }
  }

  printer.Print("/* $type$ */\n"
                "\n",
                "type", desc->full_name());

  if (hasself) {
    printer.Print("typedef struct $root$_s $type$;\n"
                  "\n"
                  "struct $root$_s {\n",
                  "root", root, "type", type);
  } else {
    printer.Print("typedef struct {\n");
  }
  Indent(printer);

  for (int i = 0; i < desc->field_count(); ++i) {
    const FieldDescriptor *field = desc->field(i);
    std::string ftype = FieldTypeName(field);
    std::string fname = field->name();
    std::string star;
    std::map<std::string, std::string> vars;

    if (hasptr) {
      if (FieldIsPointer(field)) {
        star = "*";
      } else {
        star = " ";
      }
    } else {
      star = "";
    }

    vars["type"] = ftype;
    vars["tspace"] = Spaces(maxtype - ftype.length());
    vars["fname"] = fname;
    vars["star"] = star;

    if (field->is_repeated()) {
      printer.Print("/* $rtype$ */\n", "rtype", FieldRealType(field));
    }
    printer.Print(vars, "$type$$tspace$ $star$$fname$;\n");
  }

  // extension tree

  if (desc->extension_range_count() > 0) {
    std::map<std::string, std::string> vars;
    std::string ftype = "ngx_rbtree_t";

    vars["type"] = ftype;
    vars["tspace"] = Spaces(maxtype - ftype.length());
    printer.Print(vars, "$type$$tspace$ *__extensions;\n");
  }

  // unknown fields (if applicable)

  if (unknown) {
    std::map<std::string, std::string> vars;
    std::string ftype = "ngx_array_t";

    vars["type"] = ftype;
    vars["tspace"] = Spaces(maxtype - ftype.length());
    printer.Print(vars,
		  "/* ngx_protobuf_unknown_field_t */\n"
		  "$type$$tspace$ *__unknown;\n");
  }

  // the "has" bits are last

  for (int i = 0; i < desc->field_count(); ++i) {
    const FieldDescriptor *field = desc->field(i);
    std::string ftype = "uint32_t";
    std::string fname = field->name();
    std::map<std::string, std::string> vars;

    vars["type"] = ftype;
    vars["tspace"] = Spaces(maxtype - ftype.length());
    vars["fname"] = "__has_" + field->name();
    vars["nspace"] = Spaces(maxname - field->name().length());
    vars["star"] = (hasptr) ? " " : "";

    printer.Print(vars, "$type$$tspace$ $star$$fname$$nspace$ : 1;\n");
  }

  Outdent(printer);

  if (hasself) {
    printer.Print("};\n"
                  "\n");
  } else {
    printer.Print("} $type$;\n"
                  "\n",
                  "type", type);
  }
}

} // namespace nginx
} // namespace compiler
} // namespace protobuf
} // namespace google
