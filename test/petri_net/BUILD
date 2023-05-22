
cc_test(
    name = "petri_net_tests",
    srcs = glob(["**/*.cpp"]),
    data = [
        "//config_samples:config_samples"
    ],
    defines = ["CATCH_CONFIG_MAIN"],
    deps = [
        "@catch2//:catch2_main",
        "//src:behavior_net_lib"
    ],
)
