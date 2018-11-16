BUILD_FILE_CONTENTS = """package(default_visibility = ["//visibility:public"])

cc_library(
  name = "include",
  srcs = glob(["libs/python36.lib"]),
  hdrs = glob(["include/*"]),
  includes = ["include"],
)
"""

def _find_include_path(repository_ctx):
  python3_result = repository_ctx.execute([
    "python3",
    "-c",
    "from sysconfig import get_paths; print(get_paths()[\"include\"], end='')",
  ])

  python_result = repository_ctx.execute([
    "python",
    "-c",
    "from sysconfig import get_paths; print(get_paths()[\"include\"], end='')",
  ])

  return (python3_result.stdout or python_result.stdout).strip()


def _find_libs_path(repository_ctx):
  python_dir = str(repository_ctx.which("python").dirname)
  return python_dir + "/libs" if python_dir else None


def _impl(repository_ctx):
  repository_ctx.file(
    "WORKSPACE", "workspace(name = \"{name}\")\n".format(
      name=repository_ctx.name,
    )
  )

  # Symlink the Python includes directory.
  include_path = _find_include_path(repository_ctx)
  if not include_path:
    fail("Unable to locate python includes")
  repository_ctx.symlink(include_path, "include")

  # On windows, we also need to symlink the lib file.
  libs_path = _find_libs_path(repository_ctx)
  if libs_path:
    repository_ctx.symlink(libs_path, "libs")

  # Create the appropriate BUILD file.
  repository_ctx.file("BUILD", BUILD_FILE_CONTENTS)


python_repository = repository_rule(
  implementation = _impl,
)
