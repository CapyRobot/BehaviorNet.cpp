workspace(name = "behavior_net_cpp")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# bazel_skylib required by catch2
http_archive(
    name = "bazel_skylib",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.3.0/bazel-skylib-1.3.0.tar.gz",
        "https://github.com/bazelbuild/bazel-skylib/releases/download/1.3.0/bazel-skylib-1.3.0.tar.gz",
    ],
    sha256 = "74d544d96f4a5bb630d465ca8bbcfe231e3594e5aae57e1edbf17a6eb3ca2506",
)

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")
bazel_skylib_workspace()

http_archive(
    name = "catch2",
    strip_prefix = "Catch2-3.3.0",
    urls = ["https://github.com/catchorg/Catch2/archive/v3.3.0.tar.gz"],
)
