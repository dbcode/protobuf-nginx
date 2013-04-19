#include <sstream>
#include <ngx_generator.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace nginx {

void
Generator::Indented(io::Printer& printer,
                    const std::map<std::string, std::string>& vars,
                    const char *body)
{
  Indent(printer);
  printer.Print(vars, body);
  Outdent(printer);
}

void
Generator::OpenBrace(io::Printer& printer)
{
  printer.Print("{\n");
  Indent(printer);
}

void
Generator::CloseBrace(io::Printer& printer)
{
  Outdent(printer);
  printer.Print("}\n");
}

void
Generator::SimpleIf(io::Printer& printer,
                    const std::map<std::string, std::string>& vars,
                    const char *cond)
{
  printer.Print("if (");
  printer.Print(vars, cond);
  printer.Print(") ");
  OpenBrace(printer);
}

void
Generator::CuddledIf(io::Printer& printer,
                     const std::map<std::string, std::string>& vars,
                     const char *cond1,
                     const char *cond2)
{
  printer.Print("if (");
  printer.Print(vars, cond1);
  printer.Print("\n");
  Indent(printer);
  printer.Print(vars, cond2);  
  printer.Print(")\n");
  Outdent(printer);
  OpenBrace(printer);
}

void
Generator::FullSimpleIf(io::Printer& printer,
                        const std::map<std::string, std::string>& vars,
                        const char *cond,
                        const char *body)
{
  SimpleIf(printer, vars, cond);
  printer.Print(vars, body);
  printer.Print("\n");
  CloseBrace(printer);
}

void
Generator::FullCuddledIf(io::Printer& printer,
                         const std::map<std::string, std::string>& vars,
                         const char *cond1,
                         const char *cond2,
                         const char *body)
{
  CuddledIf(printer, vars, cond1, cond2);
  printer.Print(vars, body);
  printer.Print("\n");
  CloseBrace(printer);
}

void
Generator::Else(io::Printer& printer)
{
  Outdent(printer);
  printer.Print("} else {\n");
  Indent(printer);
}

void
Generator::SkipUnknown(io::Printer& printer)
{
  printer.Print("if (ngx_protobuf_skip(pos, end, wire) != NGX_OK) ");
  BracedReturn(printer, "NGX_ABORT");
}

void
Generator::BracedReturn(io::Printer& printer, const char *value)
{
  OpenBrace(printer);
  printer.Print("return ");
  printer.Print(value);
  printer.Print(";\n");
  CloseBrace(printer);
}

std::string
Generator::Spaces(int count)
{
  std::string output;

  while (count-- > 0) {
    output += ' ';
  }

  return output;
}

std::string
Generator::Number(int number)
{
  std::ostringstream os;

  os << number;

  return os.str();
}

void
Generator::Indent(io::Printer& printer)
{
  printer.Indent();
  printer.Indent();
}

void
Generator::Outdent(io::Printer& printer)
{
  printer.Outdent();
  printer.Outdent();
}

} // namespace nginx
} // namespace compiler
} // namespace protobuf
} // namespace google
