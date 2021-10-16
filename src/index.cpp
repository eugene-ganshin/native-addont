#include <napi.h>
#include "../lib/ltoapi.h"

Napi::Value openConnection(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    short initStatus = ltInit();
    if (initStatus < 0)
    {
        Napi::TypeError::New(env, "Error has occurred.")
            .ThrowAsJavaScriptException();

        return env.Null();
    }

    // GET ltOpen params
    // host
    // port
    // clientCode
    // short *session; // uninitialized session pointer

    // short openStatus = ltOpen();
    // if (openStatus)
    // {
    //     Napi::TypeError::New(env, "Error has occurred.")
    //         .ThrowAsJavaScriptException();

    //     return env.Null();
    // }

    return Napi::Number::New(env, 0);
}

Napi::Value query(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, 0);
}

Napi::Value closeConnection(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, 0);
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    exports.Set(Napi::String::New(env, "openConnection"), Napi::Function::New(env, openConnection));
    exports.Set(Napi::String::New(env, "query"), Napi::Function::New(env, query));
    exports.Set(Napi::String::New(env, "closeConnection"), Napi::Function::New(env, closeConnection));

    return exports;
}

NODE_API_MODULE(greet, Init)