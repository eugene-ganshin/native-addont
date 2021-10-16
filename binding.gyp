{
    "targets": [
        {
            "target_name": "ltApi",
            "cflags!": ["-fno-excpetions"],
            "cflags_cc!": ["-fno-excpetions"],
            "sources": [
                "./lib/ltCmn.c",
                "./lib/ltMsg.c",
                "./lib/ltClntU.c",
                "./src/index.cpp"
            ],
            "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")"
            ],
            'defines': ['NAPI_DISABLE_CPP_EXCEPTIONS'],
        }
    ]
}
