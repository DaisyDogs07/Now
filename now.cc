#include "v8config.h"
#if defined(V8_OS_MACOSX)
#include <mach/mach_time.h>
#include <pthread.h>
#elif defined(V8_OS_POSIX)
#include <sys/time.h>
#include <unistd.h>
#elif defined(V8_OS_WIN)
double fr;
{
  LARGE_INTEGER li;
  if (!QueryPerformanceFrequency(&li))
    std::abort();
  fr = static_cast<double>(li.QuadPart) / 1000.0;
}
#elif defined(V8_OS_STARBOARD)
#include "starboard/time.h"
#endif
#include "node.h"

using namespace v8;

double GetNow() {
#if defined(V8_OS_MACOSX)
  static struct mach_timebase_info info;
  if (info.denom == 0) {
    kern_return_t result = mach_timebase_info(&info);
    if (result != KERN_SUCCESS)
      std::abort();
  }
  return (static_cast<double>(
    mach_absolute_time() * (info.numer / info.denom)
  ) / 1000000.0) + 1.0;
#elif defined(V8_OS_SOLARIS)
  return (static_cast<double>(gethrtime()) / 1000000.0) + 1.0;
#elif defined(V8_OS_POSIX)
#if (defined(_POSIX_MONOTONIC_CLOCK) && _POSIX_MONOTONIC_CLOCK >= 0) || \
  defined(V8_OS_BSD) || defined(V8_OS_ANDROID)
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    std::abort();
  return (static_cast<double>(ts.tv_sec) * 1000.0) +
    (static_cast<double>(ts.tv_nsec) / 1000000.0) + 1.0;
#else
  return 0.0;
#endif
#elif defined(V8_OS_WIN)
  LARGE_INTEGER pc;
  if (!QueryPerformanceCounter(&pc))
    std::abort();
  return (static_cast<double>(pc.QuadPart) / fr) + 1.0;
#elif defined(V8_OS_STARBOARD)
  return (static_cast<double>(SbTimeGetMonotonicNow()) / 1000.0) + 1.0;
#endif
}
double kInitialTime;
void Now(const FunctionCallbackInfo<Value>& args) {
  args.GetReturnValue().Set(
    Number::New(args.GetIsolate(), GetNow() - kInitialTime)
  );
}

NODE_MODULE_INIT() {
  Isolate* isolate = context->GetIsolate();
  Local<Function> fn = Function::New(
    context,
    Now,
    Local<Value>(),
    0,
    ConstructorBehavior::kAllow,
    SideEffectType::kHasNoSideEffect
  ).ToLocalChecked();
  fn->SetName(String::NewFromUtf8Literal(isolate, "now"));
  module.As<Object>()->Set(
    context,
    String::NewFromUtf8Literal(isolate, "exports"),
    fn
  ).Check();
  kInitialTime = GetNow();
}