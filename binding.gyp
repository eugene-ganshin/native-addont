{
    "targets": [
        {
            "target_name": "swap-nums",
            "cflags!": ["-fno-excpetions"],
            "cflags_cc!": ["-fno-excpetions"],
            "sources": [
                "./src/swap-nums.c",
                "./src/index.cpp"
            ],
            "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")"
            ],
            'defines': ['NAPI_DISABLE_CPP_EXCEPTIONS'],
        }
    ]
}
