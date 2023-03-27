#ifndef MODEL_H_
#define MODEL_H_

#include <string>

#include <onnxruntime_cxx_api.h>

using namespace std;

namespace piper {
const string instanceName{"piper"};

struct ModelSession {
  Ort::Session onnx;
  Ort::AllocatorWithDefaultOptions allocator;
  Ort::SessionOptions options;
  Ort::Env env;

  ModelSession() : onnx(nullptr){};
};

void loadModel(string modelPath, ModelSession &session) {

  session.env = Ort::Env(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING,
                         instanceName.c_str());
  session.env.DisableTelemetryEvents();

  // Slows down performance by ~2x
  // session.options.SetIntraOpNumThreads(1);

  // Roughly doubles load time for no visible inference benefit
  // session.options.SetGraphOptimizationLevel(
  //     GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

  session.options.SetGraphOptimizationLevel(
      GraphOptimizationLevel::ORT_DISABLE_ALL);

  // Slows down performance very slightly
  // session.options.SetExecutionMode(ExecutionMode::ORT_PARALLEL);

  session.options.DisableCpuMemArena();
  session.options.DisableMemPattern();
  session.options.DisableProfiling();

  auto startTime = chrono::steady_clock::now();
  session.onnx = Ort::Session(session.env, modelPath.c_str(), session.options);
  auto endTime = chrono::steady_clock::now();
  auto loadDuration = chrono::duration<double>(endTime - startTime);
}

} // namespace piper

#endif // MODEL_H_
