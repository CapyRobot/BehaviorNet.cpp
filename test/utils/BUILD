
cc_test(
    name = "utils_tests",
    srcs = glob(["**/*.cpp"]) + glob(["**/*.hpp"]),
    defines = ["CATCH_CONFIG_MAIN"],
    deps = [
        "@catch2//:catch2_main",
        "//src:behavior_net_lib"
    ],
)
