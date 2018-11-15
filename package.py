import glob
import os
import shutil
import stat
import subprocess
import sys


def mirror_tree(src, dst, pattern):
  if not os.path.exists(dst):
    os.mkdir(dst)
  abs_pattern = os.path.abspath(os.path.join(src, pattern))
  print("Copying", abs_pattern, "to", dst)
  for src_filename in glob.iglob(abs_pattern, recursive = True):
    if os.path.isdir(src_filename):
      continue
    dst_filename = os.path.join(dst, os.path.relpath(src_filename, src))
    dst_dir = os.path.dirname(dst_filename)
    if not os.path.exists(dst_dir):
      os.makedirs(dst_dir)
    shutil.copy(src_filename, dst_filename)
    os.chmod(dst_filename, os.stat(dst_filename).st_mode | stat.S_IWUSR)


def main():
  # Build the binary files.
  copt = "/std:c++17" if os.name == "nt" else "-std=c++17"
  subprocess.check_call(
      [
          "bazel",
          "build",
          "--compilation_mode=opt",
          f"--copt={copt}",
          "//src:game",
      ],
      stdout = sys.stdout,
      stderr = sys.stdout,
  )

  # Copy the files into the bin directory.
  mirror_tree("bazel-bin/src/", "bin/", "*.exe")
  mirror_tree("bazel-bin/src/", "bin/", "*.pdb")
  mirror_tree("bazel-bin/src/", "bin/", "game")
  mirror_tree("bazel-bin/src/", "bin/", "*.dll")
  mirror_tree("bazel-bin/src/", "bin/", "configs/*.json")
  mirror_tree("bazel-bin/src/", "bin/", "data/*.db")
  mirror_tree("bazel-bin/src/", "bin/", "fonts/**/*.ttf")
  mirror_tree("bazel-bin/src/", "bin/", "images/**/*.png")
  mirror_tree("bazel-bin/src/", "bin/", "scripts/*.js")
  mirror_tree("bazel-bin/src/", "bin/", "scripts/*.lua")
  mirror_tree("bazel-bin/src/", "bin/", "shaders/*.glsl")


if __name__ == "__main__":
  main()
