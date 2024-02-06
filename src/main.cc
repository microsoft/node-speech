/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include <napi.h>
#include <speechapi_cxx.h>

#include <string>
#include <unordered_map>
#include <thread>

using namespace Microsoft::CognitiveServices::Speech;
using namespace Microsoft::CognitiveServices::Speech::Audio;

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
  DISPOSED = 10,
  ERROR = 11
};

enum RuntimeStatus
{
  START = 1,
  STOP = 2,
  DISPOSE = 3
};

#pragma region Transcription

static int transcriptionWorkerIds = 0;
static std::unordered_map<int, RuntimeStatus> transcriptionWorkers;
static std::mutex transcriptionWorkersMutex;

void UpdateTranscriptionWorkerStatus(int workerId, RuntimeStatus status)
{
  std::lock_guard<std::mutex> lock(transcriptionWorkersMutex);
  transcriptionWorkers[workerId] = status;
}

void RemoveTranscriptionWorkerStatus(int workerId)
{
  std::lock_guard<std::mutex> lock(transcriptionWorkersMutex);
  transcriptionWorkers.erase(workerId);
}

RuntimeStatus GetTranscriptionWorkerStatus(int workerId)
{
  std::lock_guard<std::mutex> lock(transcriptionWorkersMutex);
  auto it = transcriptionWorkers.find(workerId);
  if (it != transcriptionWorkers.end())
  {
    return it->second;
  }
  else
  {
    return RuntimeStatus::DISPOSE;
  }
}

struct TranscriptionWorkerCallbackResult
{
  StatusCode status;
  std::string data = "";
};

class TranscriptionWorker : public Napi::AsyncProgressQueueWorker<TranscriptionWorkerCallbackResult>
{
public:
  const int id;

  TranscriptionWorker(std::string &path, std::string &key, std::string &model, std::string &logsPath, std::vector<std::string> &phrases, Napi::Function &callback)
      : Napi::AsyncProgressQueueWorker<TranscriptionWorkerCallbackResult>(callback), id(transcriptionWorkerIds++), path(path), key(key), model(model), logsPath(logsPath), phrases(phrases), started(false)
  {
    UpdateTranscriptionWorkerStatus(this->id, RuntimeStatus::STOP);
  }

  void Execute(const ExecutionProgress &progress)
  {
    try
    {
      auto speechConfig = EmbeddedSpeechConfig::FromPath(path);
      speechConfig->SetSpeechRecognitionModel(model, key);
      if (!this->logsPath.empty())
      {
        speechConfig->SetProperty(PropertyId::Speech_LogFilename, logsPath);
      }

      auto audioConfig = AudioConfig::FromDefaultMicrophoneInput();
      auto recognizer = SpeechRecognizer::FromConfig(speechConfig, audioConfig);

      auto phraseList = PhraseListGrammar::FromRecognizer(recognizer);
      for (auto phrase : this->phrases)
      {
        phraseList->AddPhrase(phrase);
      }

      // Callback: intermediate transcription results
      recognizer->Recognizing += [progress](const SpeechRecognitionEventArgs &e)
      {
        if (e.Result->Reason == ResultReason::RecognizingSpeech)
        {
          auto result = TranscriptionWorkerCallbackResult{StatusCode::RECOGNIZING, e.Result->Text};
          progress.Send(&result, 1);
        }
      };

      // Callback: final transcription result (sentence)
      recognizer->Recognized += [progress](const SpeechRecognitionEventArgs &e)
      {
        if (e.Result->Reason == ResultReason::RecognizedSpeech)
        {
          auto result = TranscriptionWorkerCallbackResult{StatusCode::RECOGNIZED, e.Result->Text};
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
            auto result = TranscriptionWorkerCallbackResult{StatusCode::NOT_RECOGNIZED};
            progress.Send(&result, 1);
            break;
          }

          // Input audio was silent and the initial silence timeout expired.
          // In continuous recognition this can happen multiple times during
          // a session, not just at the very beginning.
          case NoMatchReason::InitialSilenceTimeout:
          {
            auto result = TranscriptionWorkerCallbackResult{StatusCode::INITIAL_SILENCE_TIMEOUT};
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
            auto result = TranscriptionWorkerCallbackResult{StatusCode::END_SILENCE_TIMEOUT};
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
          auto result = TranscriptionWorkerCallbackResult{StatusCode::ERROR, e.ErrorDetails};
          progress.Send(&result, 1);
          break;
        }

        default:
          break;
        }
      };

      // Callback: begin of recognition session
      recognizer->SessionStarted += [this, progress](const SessionEventArgs &e)
      {
        this->started = true;

        UNUSED(e);
        auto result = TranscriptionWorkerCallbackResult{StatusCode::STARTED};
        progress.Send(&result, 1);
      };

      // Callback: speech start detected
      recognizer->SpeechStartDetected += [progress](const RecognitionEventArgs &e)
      {
        UNUSED(e);
        auto result = TranscriptionWorkerCallbackResult{StatusCode::SPEECH_START_DETECTED};
        progress.Send(&result, 1);
      };

      // Callback: speech end detected
      recognizer->SpeechEndDetected += [progress](const RecognitionEventArgs &e)
      {
        UNUSED(e);
        auto result = TranscriptionWorkerCallbackResult{StatusCode::SPEECH_END_DETECTED};
        progress.Send(&result, 1);
      };

      // Callback: end of recognition session
      recognizer->SessionStopped += [this, progress](const SessionEventArgs &e)
      {
        this->started = false;

        UNUSED(e);
        auto result = TranscriptionWorkerCallbackResult{StatusCode::STOPPED};
        progress.Send(&result, 1);
      };

      RuntimeStatus status;
      while ((status = GetTranscriptionWorkerStatus(this->id)) != RuntimeStatus::DISPOSE)
      {
        switch (status)
        {
        case RuntimeStatus::START:
          if (!this->started)
          {
            recognizer->StartContinuousRecognitionAsync().get();
          }
          break;
        case RuntimeStatus::STOP:
          if (this->started)
          {
            recognizer->StopContinuousRecognitionAsync().get();
          }
          break;
        case RuntimeStatus::DISPOSE:
          break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      if (this->started)
      {
        recognizer->StopContinuousRecognitionAsync().get();
      }

      RemoveTranscriptionWorkerStatus(this->id);
    }
    catch (const std::exception &e)
    {
      auto result = TranscriptionWorkerCallbackResult{StatusCode::ERROR, e.what()};
      progress.Send(&result, 1);
    }
  }

  void OnProgress(const TranscriptionWorkerCallbackResult *result, size_t /* count */)
  {
    Napi::HandleScope scope(Env());

    auto jsResult = Napi::Object::New(Env());
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

    auto jsResult = Napi::Object::New(Env());
    jsResult.Set("status", Napi::Number::New(Env(), StatusCode::DISPOSED));

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
  std::string logsPath;
  std::vector<std::string> phrases;
  bool started;
};

Napi::Value CreateTranscriber(const Napi::CallbackInfo &info)
{
  auto env = info.Env();

  // Validate args
  if (info.Length() != 6)
  {
    Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  else if (!info[0].IsString() || !info[1].IsString() || !info[2].IsString() || (!info[3].IsUndefined() && !info[3].IsString()) || !info[4].IsArray() || !info[5].IsFunction())
  {
    Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  auto modelPath = info[0].As<Napi::String>().Utf8Value();
  auto modelName = info[1].As<Napi::String>().Utf8Value();
  auto modelKey = info[2].As<Napi::String>().Utf8Value();
  std::string logsPath;
  if (!info[3].IsUndefined())
  {
    logsPath = info[3].As<Napi::String>().Utf8Value();
  }
  auto phrasesRaw = info[4].As<Napi::Array>();
  std::vector<std::string> phrases;
  for (uint32_t i = 0; i < static_cast<uint32_t>(phrasesRaw.Length()); i++)
  {
    phrases.push_back(phrasesRaw.Get(i).As<Napi::String>().Utf8Value());
  }
  auto callback = info[5].As<Napi::Function>();

  try
  {
    auto *worker = new TranscriptionWorker(modelPath, modelKey, modelName, logsPath, phrases, callback);
    worker->Queue();

    return Napi::Number::New(env, worker->id);
  }
  catch (const std::exception &e)
  {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    return env.Undefined();
  }
}

Napi::Value UpdateTranscriber(const Napi::CallbackInfo &info, RuntimeStatus status)
{
  auto env = info.Env();

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

  auto workerId = info[0].As<Napi::Number>();
  UpdateTranscriptionWorkerStatus(workerId.Int32Value(), status);

  return env.Undefined();
}

Napi::Value StartTranscriber(const Napi::CallbackInfo &info)
{
  return UpdateTranscriber(info, RuntimeStatus::START);
}

Napi::Value StopTranscriber(const Napi::CallbackInfo &info)
{
  return UpdateTranscriber(info, RuntimeStatus::STOP);
}

Napi::Value DisposeTranscriber(const Napi::CallbackInfo &info)
{
  return UpdateTranscriber(info, RuntimeStatus::DISPOSE);
}

#pragma endregion

#pragma region KeywordRecognition

static int keywordWorkerIds = 0;
static std::unordered_map<int, std::promise<void>> runningKeywordWorkers;
static std::mutex runningKeywordWorkersMutex;

void StopKeywordWorker(int workerId)
{
  std::lock_guard<std::mutex> lock(runningKeywordWorkersMutex);
  auto runningKeywordWorker = runningKeywordWorkers.find(workerId);
  if (runningKeywordWorker != runningKeywordWorkers.end())
  {
    runningKeywordWorker->second.set_value();
    runningKeywordWorkers.erase(runningKeywordWorker);
  }
}

struct KeywordWorkerCallbackResult
{
  StatusCode status;
  std::string data = "";
};

class KeywordWorker : public Napi::AsyncProgressQueueWorker<KeywordWorkerCallbackResult>
{
public:
  const int id;

  KeywordWorker(std::string &path, Napi::Function &callback)
      : Napi::AsyncProgressQueueWorker<KeywordWorkerCallbackResult>(callback), id(keywordWorkerIds++), path(path)
  {
    std::lock_guard<std::mutex> lock(runningKeywordWorkersMutex);
    runningKeywordWorkers[this->id] = std::promise<void>();

    this->waitingToStop = runningKeywordWorkers[this->id].get_future();
  }

  void Execute(const ExecutionProgress &progress)
  {
    try
    {
      auto keywordRecognitionConfig = KeywordRecognitionModel::FromFile(path);

      auto audioConfig = AudioConfig::FromDefaultMicrophoneInput();
      auto recognizer = KeywordRecognizer::FromConfig(audioConfig);

      // Callback: keyword recognized
      recognizer->Recognized += [this, progress](const KeywordRecognitionEventArgs &e)
      {
        auto result = KeywordWorkerCallbackResult{StatusCode::RECOGNIZED, e.Result->Text};
        progress.Send(&result, 1);

        StopKeywordWorker(this->id);
      };

      // Callback: errors
      recognizer->Canceled += [progress](const SpeechRecognitionCanceledEventArgs &e)
      {
        switch (e.Reason)
        {
        case CancellationReason::Error:
        {
          auto result = KeywordWorkerCallbackResult{StatusCode::ERROR, e.ErrorDetails};
          progress.Send(&result, 1);
          break;
        }

        default:
          break;
        }
      };

      // Starts keyword recognition and wait for end & stopping
      // We need to assign the future to a variable to avoid the
      // future automatically getting destroyed, which would block
      // the thread.
      //
      // Refs: https://github.com/Azure-Samples/cognitive-services-speech-sdk/issues/2229
      // https://stackoverflow.com/questions/23455104/why-is-the-destructor-of-a-future-returned-from-stdasync-blocking
      auto recognitionFuture = recognizer->RecognizeOnceAsync(keywordRecognitionConfig);
      this->waitingToStop.get();
      recognizer->StopRecognitionAsync().get();
    }
    catch (const std::exception &e)
    {
      auto result = KeywordWorkerCallbackResult{StatusCode::ERROR, e.what()};
      progress.Send(&result, 1);
    }
  }

  void OnProgress(const KeywordWorkerCallbackResult *result, size_t /* count */)
  {
    Napi::HandleScope scope(Env());

    auto jsResult = Napi::Object::New(Env());
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

    auto jsResult = Napi::Object::New(Env());
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
  std::future<void> waitingToStop;
};

Napi::Value Recognize(const Napi::CallbackInfo &info)
{
  auto env = info.Env();

  // Validate args
  if (info.Length() != 2)
  {
    Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  else if (!info[0].IsString() || !info[1].IsFunction())
  {
    Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  auto modelPath = info[0].As<Napi::String>().Utf8Value();
  auto callback = info[1].As<Napi::Function>();

  try
  {
    auto *worker = new KeywordWorker(modelPath, callback);
    worker->Queue();

    return Napi::Number::New(env, worker->id);
  }
  catch (const std::exception &e)
  {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    return env.Undefined();
  }
}

Napi::Value Unrecognize(const Napi::CallbackInfo &info)
{
  auto env = info.Env();

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

  auto workerId = info[0].As<Napi::Number>();
  StopKeywordWorker(workerId.Int32Value());

  return env.Undefined();
}

#pragma endregion

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  exports.Set(Napi::String::New(env, "createTranscriber"), Napi::Function::New(env, CreateTranscriber));
  exports.Set(Napi::String::New(env, "startTranscriber"), Napi::Function::New(env, StartTranscriber));
  exports.Set(Napi::String::New(env, "stopTranscriber"), Napi::Function::New(env, StopTranscriber));
  exports.Set(Napi::String::New(env, "disposeTranscriber"), Napi::Function::New(env, DisposeTranscriber));

  exports.Set(Napi::String::New(env, "recognize"), Napi::Function::New(env, Recognize));
  exports.Set(Napi::String::New(env, "unrecognize"), Napi::Function::New(env, Unrecognize));

  return exports;
}

NODE_API_MODULE(speechapi, Init)