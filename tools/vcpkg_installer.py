import argparse
import os
import shutil
import sys
import subprocess

# Figure out what dire
VCPKG_DIR = os.path.abspath(os.path.expanduser("~") + "/.vcpkg/")


def install_vcpkg():
  print("Installing vcpkg at directory: ", VCPKG_DIR)
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
        ["bootstrap-vcpkg.sh"],
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
        ["vcpkg"] + list(args),
        cwd = VCPKG_DIR,
        shell = True,
    )


def main():
  # Parse the command-line flags.
  parser = argparse.ArgumentParser()
  parser.add_argument("triplet", type = str)
  flags = parser.parse_args()

  print("Using vcpkg directory: ", VCPKG_DIR)
  print("Using triplet: ", flags.triplet)

  tools_dir = os.path.dirname(os.path.realpath(__file__))

  with open(os.path.join(tools_dir, "vcpkg_deps")) as deps:
    for line in deps.readlines():
      pkg = f"{line.strip()}:{flags.triplet}"
      run_vcpkg("install", pkg)

  dst_dir = os.path.abspath(
      os.path.join(tools_dir, os.pardir, "third_party", flags.triplet)
  )
  src_dir = os.path.join(VCPKG_DIR, "installed", flags.triplet)

  # Copy the libs into the third_party directory.
  dst_lib_dir = os.path.join(dst_dir, "lib")
  if os.path.exists(dst_lib_dir):
    shutil.rmtree(dst_lib_dir)
  print("Copying library files into: ", dst_lib_dir)
  shutil.copytree(os.path.join(src_dir, "lib"), dst_lib_dir)

  # Copy the libs into the third_party directory.
  dst_include_dir = os.path.join(dst_dir, "include")
  if os.path.exists(dst_include_dir):
    shutil.rmtree(dst_include_dir)
  print("Copying include files into: ", dst_include_dir)
  shutil.copytree(os.path.join(src_dir, "include"), dst_include_dir)


if __name__ == "__main__":
  main()
