#include <napi.h>
#include "swap-nums.h"

Napi::Number swapNums(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    int swap1 = info[0].ToNumber();
    int swap2 = info[1].ToNumber();

    swapNumbers(&swap1, &swap2);

    return Napi::Number::New(env, swap1);
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    exports.Set(Napi::String::New(env, "swapNums"), Napi::Function::New(env, swapNums));

    return exports;
}

NODE_API_MODULE(greet, Init)