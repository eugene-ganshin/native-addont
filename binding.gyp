{
    "targets": [
        {
            "target_name": "greet",
            "cflags!": ["-fno-excpetions"],
            "cflags_cc!": ["-fno-excpetions"],
            "sources": [
                "./src/greeting.cpp",
                "./src/index.cpp"
            ],
            "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")"
            ],
            'defines': ['NAPI_DISABLE_CPP_EXCEPTIONS'],
        }
    ]
}
