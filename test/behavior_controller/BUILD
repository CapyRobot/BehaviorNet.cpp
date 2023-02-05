
cc_test(
    name = "behavior_controller_test",
    srcs = ["ActionTests.cpp", "ControllerTests.cpp"],
    data = [
        "//config_samples:config_samples"
    ],
    defines = ["CATCH_CONFIG_MAIN"],
    deps = [
        "@catch2//:catch2_main",
        "//src:behavior_net_lib"
    ],
)
