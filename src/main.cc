/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include <napi.h>
#include <speechapi_cxx.h>

#include <string>
#include <unordered_map>

using namespace Microsoft::CognitiveServices::Speech;
using namespace Microsoft::CognitiveServices::Speech::Audio;

static int workerIds = 0;
static std::unordered_map<int, std::promise<void>> runningWorkers;
static std::mutex runningWorkersMutex;

void StopWorker(int workerId)
{
  std::lock_guard<std::mutex> lock(runningWorkersMutex);
  auto runningWorker = runningWorkers.find(workerId);
  if (runningWorker != runningWorkers.end())
  {
    runningWorker->second.set_value();
  }
}

void ClearWorker(int workerId)
{
  std::lock_guard<std::mutex> lock(runningWorkersMutex);
  auto runningWorker = runningWorkers.find(workerId);
  if (runningWorker != runningWorkers.end())
  {
    runningWorkers.erase(runningWorker);
  }
}

enum StatusCode
{
  STARTED = 1,
  RECOGNIZING = 2,
  RECOGNIZED = 3,
  NOT_RECOGNIZED = 4,
  INITIAL_SILENCE_TIMEOUT = 5,
  END_SILENCE_TIMEOUT = 6,
  SPEECH_START_DETECTED = 7,
  SPEECH_END_DETECTED = 8,
  STOPPED = 9,
  ERROR = 10
};

struct WorkerCallbackResult
{
  StatusCode status;
  std::string data = "";
};

class Worker : public Napi::AsyncProgressQueueWorker<WorkerCallbackResult>
{
public:
  const int id;

  Worker(std::string &path, std::string &key, std::string &model, std::string &wavPath, Napi::Function &callback)
      : Napi::AsyncProgressQueueWorker<WorkerCallbackResult>(callback), id(workerIds++), path(path), key(key), model(model), wavPath(wavPath)
  {
    std::lock_guard<std::mutex> lock(runningWorkersMutex);
    runningWorkers[this->id] = std::promise<void>();
  }

  void Execute(const ExecutionProgress &progress)
  {
    try
    {
      auto speechConfig = EmbeddedSpeechConfig::FromPath(path);
      speechConfig->SetSpeechRecognitionModel(model, key);
      std::shared_ptr<AudioConfig> audioConfig;
      if (this->wavPath.empty())
      {
        audioConfig = AudioConfig::FromDefaultMicrophoneInput();
      }
      else
      {
        audioConfig = AudioConfig::FromWavFileInput(this->wavPath);
      }
      auto recognizer = SpeechRecognizer::FromConfig(speechConfig, audioConfig);

      auto phraseList = PhraseListGrammar::FromRecognizer(recognizer);
      phraseList->AddPhrase("VS Code");
      phraseList->AddPhrase("Visual Studio Code");
      phraseList->AddPhrase("GitHub");

      // Callback: intermediate transcription results
      recognizer->Recognizing += [progress](const SpeechRecognitionEventArgs &e)
      {
        if (e.Result->Reason == ResultReason::RecognizingSpeech)
        {
          auto result = WorkerCallbackResult{StatusCode::RECOGNIZING, e.Result->Text};
          progress.Send(&result, 1);
        }
      };

      // Callback: final transcription result (sentence)
      recognizer->Recognized += [progress](const SpeechRecognitionEventArgs &e)
      {
        if (e.Result->Reason == ResultReason::RecognizedSpeech)
        {
          auto result = WorkerCallbackResult{StatusCode::RECOGNIZED, e.Result->Text};
          progress.Send(&result, 1);
        }
        else if (e.Result->Reason == ResultReason::NoMatch)
        {
          auto reason = NoMatchDetails::FromResult(e.Result)->Reason;
          switch (reason)
          {
          // Input audio was not silent but contained no recognizable speech.
          case NoMatchReason::NotRecognized:
          {
            auto result = WorkerCallbackResult{StatusCode::NOT_RECOGNIZED};
            progress.Send(&result, 1);
            break;
          }

          // Input audio was silent and the initial silence timeout expired.
          // In continuous recognition this can happen multiple times during
          // a session, not just at the very beginning.
          case NoMatchReason::InitialSilenceTimeout:
          {
            auto result = WorkerCallbackResult{StatusCode::INITIAL_SILENCE_TIMEOUT};
            progress.Send(&result, 1);
            break;
          }

          // Input audio was silent and the end silence timeout expired.
          // This can happen in continuous recognition after a phrase is
          // recognized (a final result is generated) and it is followed
          // by silence. If the silence continues long enough there will
          // be InitialSilenceTimeout after this.
          case NoMatchReason::EndSilenceTimeout:
          {
            auto result = WorkerCallbackResult{StatusCode::END_SILENCE_TIMEOUT};
            progress.Send(&result, 1);
            break;
          }

          // Other reasons are not supported in embedded speech at the moment.
          default:
            break;
          }
        }
      };

      // Callback: errors
      recognizer->Canceled += [progress](const SpeechRecognitionCanceledEventArgs &e)
      {
        switch (e.Reason)
        {
        case CancellationReason::Error:
        {
          auto result = WorkerCallbackResult{StatusCode::ERROR, e.ErrorDetails};
          progress.Send(&result, 1);
          break;
        }

        default:
          break;
        }
      };

      // Callback: begin of recognition session
      recognizer->SessionStarted += [progress](const SessionEventArgs &e)
      {
        UNUSED(e);
        auto result = WorkerCallbackResult{StatusCode::STARTED};
        progress.Send(&result, 1);
      };

      // Callback: speech start detected
      recognizer->SpeechStartDetected += [progress](const RecognitionEventArgs &e)
      {
        UNUSED(e);
        auto result = WorkerCallbackResult{StatusCode::SPEECH_START_DETECTED};
        progress.Send(&result, 1);
      };

      // Callback: speech end detected
      recognizer->SpeechEndDetected += [progress](const RecognitionEventArgs &e)
      {
        UNUSED(e);
        auto result = WorkerCallbackResult{StatusCode::SPEECH_END_DETECTED};
        progress.Send(&result, 1);
      };

      // Callback: end of recognition session
      recognizer->SessionStopped += [this](const SessionEventArgs &e)
      {
        UNUSED(e);
        StopWorker(this->id);
      };

      // Starts continuous recognition and wait for end & stopping
      recognizer->StartContinuousRecognitionAsync().get();
      runningWorkers[this->id].get_future().get();
      recognizer->StopContinuousRecognitionAsync().get();
      ClearWorker(this->id);
    }
    catch (const std::exception &e)
    {
      auto result = WorkerCallbackResult{StatusCode::ERROR, e.what()};
      progress.Send(&result, 1);
    }
  }

  void OnProgress(const WorkerCallbackResult *result, size_t /* count */)
  {
    Napi::HandleScope scope(Env());

    Napi::Object jsResult = Napi::Object::New(Env());
    jsResult.Set("status", Napi::Number::New(Env(), result->status));
    if (!result->data.empty())
    {
      jsResult.Set("data", Napi::String::New(Env(), result->data));
    }

    Callback().Call({Env().Undefined(), jsResult});
  }

  void OnOK()
  {
    Napi::HandleScope scope(Env());

    Napi::Object jsResult = Napi::Object::New(Env());
    jsResult.Set("status", Napi::Number::New(Env(), StatusCode::STOPPED));

    Callback().Call({Env().Undefined(), jsResult});
  }

  void OnError(const Napi::Error &e)
  {
    Napi::HandleScope scope(Env());

    Callback().Call({Napi::String::New(Env(), e.Message())});
  }

private:
  std::string path;
  std::string key;
  std::string model;
  std::string wavPath;
};

Napi::Value Transcribe(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  // Validate args
  if (info.Length() != 5)
  {
    Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  else if (!info[0].IsString() || !info[1].IsString() || !info[2].IsString() || (!info[3].IsUndefined() && !info[3].IsString()) || !info[4].IsFunction())
  {
    Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  auto modelPath = info[0].As<Napi::String>().Utf8Value();
  auto modelName = info[1].As<Napi::String>().Utf8Value();
  auto modelKey = info[2].As<Napi::String>().Utf8Value();
  std::string wavPath;
  if (!info[3].IsUndefined())
  {
    wavPath = info[3].As<Napi::String>().Utf8Value();
  }
  auto callback = info[4].As<Napi::Function>();

  try
  {
    Worker *worker = new Worker(modelPath, modelKey, modelName, wavPath, callback);
    worker->Queue();

    return Napi::Number::New(env, worker->id);
  }
  catch (const std::exception &e)
  {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    return env.Undefined();
  }
}

Napi::Value Untranscribe(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  // Validate args
  if (info.Length() < 1)
  {
    Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  else if (!info[0].IsNumber())
  {
    Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  Napi::Number workerId = info[0].As<Napi::Number>();
  StopWorker(workerId.Int32Value());

  return env.Undefined();
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  exports.Set(Napi::String::New(env, "transcribe"), Napi::Function::New(env, Transcribe));
  exports.Set(Napi::String::New(env, "untranscribe"), Napi::Function::New(env, Untranscribe));

  return exports;
}

NODE_API_MODULE(speechapi, Init)