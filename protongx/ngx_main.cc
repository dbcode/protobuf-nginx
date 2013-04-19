#include "config.h"
#include <google/protobuf/compiler/command_line_interface.h>
#include <google/protobuf/compiler/code_generator.h>
#include <ngx_generator.h>

int
main (int argc, char **argv)
{
  google::protobuf::compiler::CommandLineInterface cli;
  google::protobuf::compiler::nginx::Generator gen;

  cli.RegisterGenerator("--out", &gen, "Generate nginx source files.");
  cli.SetVersionInfo(PACKAGE " " VERSION);

  return cli.Run(argc, argv);
}
