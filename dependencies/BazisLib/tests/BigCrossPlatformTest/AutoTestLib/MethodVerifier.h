#pragma once

template<typename _Ty> char _MethodVerifyHelper(_Ty);
#define ENSURE_MEMBER_DECL(className, methodName, returnType, ...) typedef char __PROTOTYPE_VERIFIER__[sizeof(_MethodVerifyHelper<returnType (className::*)(__VA_ARGS__)>(&className::methodName))]
#define ENSURE_CONSTRUCTOR_DECL(className, ...) typedef char __PROTOTYPE_VERIFIER__[sizeof(_MethodVerifyHelper(className(__VA_ARGS__)))]
