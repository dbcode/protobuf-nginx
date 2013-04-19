#include <ngx_generator.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace nginx {

std::string
Generator::FileRoot(const std::string& input)
{
  std::string output;
  std::string::const_iterator i;

  output = "ngx_";
  for (i = input.begin(); i != input.end(); ++i) {
    char c = *i;

    if (isalnum(c)) {
      output += c;
    } else {
      output += '_';
    }
  }

  return output;
}

std::string
Generator::IncludeGuard(const std::string& input)
{
  std::string output;
  std::string::const_iterator i;

  output = "_";
  for (i = input.begin(); i != input.end(); ++i) {
    char c = *i;

    if (isalnum(c)) {
      output += toupper(c);
    } else {
      output += '_';
    }
  }
  output += "_INCLUDED_";

  return output;  
}

std::string
Generator::BareRoot(const std::string& input)
{
  std::string output;
  std::string::const_iterator i;
  bool lower = false;

  for (i = input.begin(); i != input.end(); ++i) {
    char c = *i;

    if (islower(c)) {
      lower = true;
    } else {
      if (lower && c != '.') {
        output += '_';
      }
      lower = false;
    }
    if (isalnum(c)) {
      output += tolower(c);
    } else {
      output += '_';
    }
  }

  return output;  
}

std::string
Generator::TypedefRoot(const std::string& input)
{
  return "ngx_" + BareRoot(input);
}

std::string
Generator::EnumType(const std::string& input)
{
  return TypedefRoot(input) + "_e";
}

std::string
Generator::EnumValue(const std::string& input)
{
  std::string output;
  std::string::const_iterator i;

  output = "NGX_";
  for (i = input.begin(); i != input.end(); ++i) {
    char c = *i;

    if (isalnum(c)) {
      output += toupper(c);
    } else {
      output += '_';
    }
  }

  return output;  
}

std::string
Generator::StructType(const std::string& input)
{
  return TypedefRoot(input) + "_t";
}

} // namespace nginx
} // namespace compiler
} // namespace protobuf
} // namespace google
