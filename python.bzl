BUILD_FILE_CONTENTS = """package(default_visibility = ["//visibility:public"])

cc_library(
  name = "include",
  srcs = glob(["src/libs/python37.lib"]),
  hdrs = glob(["src/include/*"]),
  includes = ["src/include"],
)
"""

def _impl(repository_ctx):
  repository_ctx.file(
    "WORKSPACE", "workspace(name = \"{name}\")\n".format(
      name=repository_ctx.name,
    )
  )
  python_dir = repository_ctx.which("python").dirname
  repository_ctx.symlink(python_dir, "src")
  repository_ctx.file("BUILD", BUILD_FILE_CONTENTS)


python_repository = repository_rule(
  implementation = _impl,
)