import argparse
import glob
import os
import shutil
import subprocess
import sys

# Figure out what dire
VCPKG_DIR = os.path.abspath(os.path.expanduser("~") + "/.vcpkg/")


def install_vcpkg():
  print("Installing vcpkg at directory: ", VCPKG_DIR)
  if not os.path.exists(VCPKG_DIR):
    os.mkdir(VCPKG_DIR)
  subprocess.call(
      ["git", "clone", "https://github.com/Microsoft/vcpkg.git", "."],
      cwd = VCPKG_DIR,
  )

  if os.name == "nt":
    subprocess.call(
        ["bootstrap-vcpkg.bat"],
        cwd = VCPKG_DIR,
        shell = True,
    )
  else:
    subprocess.call(
        ["./bootstrap-vcpkg.sh"],
        cwd = VCPKG_DIR,
        shell = True,
    )


def run_vcpkg(*args):
  if not any(
      [
          os.path.exists(os.path.join(VCPKG_DIR, "vcpkg.exe")),
          os.path.exists(os.path.join(VCPKG_DIR, "vcpkg")),
      ]
  ):
    install_vcpkg()

  print("Running vcpkg command: vcpkg", *args)
  if os.name == "nt":
    subprocess.call(
        ["vcpkg.exe"] + list(args),
        cwd = VCPKG_DIR,
        shell = True,
    )
  else:
    subprocess.call(
        ["./vcpkg"] + list(args),
        cwd = VCPKG_DIR,
    )


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


def main():
  # Parse the command-line flags.
  parser = argparse.ArgumentParser()
  parser.add_argument("triplet", type = str)
  parser.add_argument("--skip_install", default = False, action = 'store_true')
  flags = parser.parse_args()

  print("Using vcpkg directory: ", VCPKG_DIR)
  print("Using triplet: ", flags.triplet)

  tools_dir = os.path.dirname(os.path.realpath(__file__))

  if not flags.skip_install:
    with open(os.path.join(tools_dir, "vcpkg_deps")) as deps:
      for line in deps.readlines():
        pkg = f"{line.strip()}:{flags.triplet}"
        run_vcpkg("install", pkg)

  dst_dir = os.path.abspath(
      os.path.join(tools_dir, os.pardir, "third_party", flags.triplet)
  )
  src_dir = os.path.join(VCPKG_DIR, "installed", flags.triplet)

  # Copy the static libs into the third_party directory.
  mirror_tree(src_dir, dst_dir, "lib/**/*.lib")
  mirror_tree(src_dir, dst_dir, "lib/**/*.a")

  # Copy the shared libs into the third_party directory.
  mirror_tree(src_dir, dst_dir, "bin/**/*.dll")
  mirror_tree(src_dir, dst_dir, "bin/**/*.so")

  # Copy the includes into the third_party directory.
  mirror_tree(src_dir, dst_dir, "include/**")


if __name__ == "__main__":
  main()
