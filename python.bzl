BUILD_FILE_CONTENTS = """package(default_visibility = ["//visibility:public"])

cc_library(
  name = "include",
  srcs = {srcs},
  hdrs = glob(["include/*"]),
  includes = ["include"],
)
"""

def _impl(repository_ctx):
  repository_ctx.file(
    "WORKSPACE", "workspace(name = \"{name}\")\n".format(
      name=repository_ctx.name,
    )
  )

  include_dir = repository_ctx.execute([
    "python3",
    "-c",
    "from sysconfig import get_paths; print(get_paths()[\"include\"], end='')",
  ]).stdout.strip()
  repository_ctx.symlink(include_dir, "include")
  repository_ctx.file(
    "BUILD",
    BUILD_FILE_CONTENTS.format(
      srcs=str([]),
    ),
  )


python_repository = repository_rule(
  implementation = _impl,
)
