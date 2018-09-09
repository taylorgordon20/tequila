SETUP_TEMPLATE = """
from setuptools import setup

setup(
  name = "{name}",
  version = "{version}",
  py_modules = {py_modules},
)
"""

def _module_path(ctx, module, module_file):
  module_base = ctx.attr.module_base
  if not module_file.short_path.startswith(module_base):
    fail("Encounted module file outside module base.")
  module_path = module_file.short_path[len(module_base):]
  if all([
    module_file.basename == module.label.name,
    module_path.endswith(".dll"),
  ]):
    module_path = module_path[:-3] + "pyd"
  return "{package}{module}".format(
    package = ctx.attr.name,
    module = module_path,
  )

def _python_path(module_path):
  return ".".join(module_path.split(".")[:-1]).replace("/", ".")

def _py_extension_impl(ctx):
  out_files = []
  manifest_paths = []

  # Process all dependencies and move them into the appropriate directories.
  for module in ctx.attr.modules:
    for module_file in module.files:
      module_path = _module_path(ctx, module, module_file)
      manifest_paths.append(module_path)

      # Copy the source file to the corresponding module file.
      out_file = ctx.actions.declare_file(module_path)
      out_files.append(out_file)
      ctx.actions.run_shell(
        outputs = [out_file],
        inputs = [module_file],
        command = "cp {} {}".format(module_file.path, out_file.path)
      )

  if ctx.attr.generate_setup:
    # Create the manifest file.
    manifest_file = ctx.actions.declare_file("MANIFEST.in")
    ctx.actions.write(
      manifest_file,
      "\n".join(["include " + path for path in manifest_paths]),
    )

    # Create a setup script.
    setup_file = ctx.actions.declare_file("setup.py")
    ctx.actions.write(
      setup_file,
      SETUP_TEMPLATE.format(
        name=ctx.attr.name,
        version=ctx.attr.version,
        py_modules=str([ctx.attr.name]),
      )
    )
    out_files.extend([manifest_file, setup_file])

  return DefaultInfo(files = depset(out_files))


py_extension = rule(
    implementation = _py_extension_impl,
    attrs = {
        "version": attr.string(default = "0.0"),
        "module_base": attr.string(default = ""),
        "modules": attr.label_list(allow_files = True),
        "generate_setup": attr.bool(default = False),
    },
)


def py_cpp_module(name, srcs=[], deps=[]):
    native.cc_binary(
      name = "{}.dll".format(name),
      srcs = srcs,
      linkshared = True,
      deps = deps,
    )

    native.alias(
      name = name,
      actual = select({
        "@bazel_tools//src/conditions:windows": "{}.dll".format(name),
        "//conditions:default": "{}.so".format(name),
      }),
    )