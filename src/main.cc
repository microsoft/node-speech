/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include <napi.h>
#include <speechapi_cxx.h>

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include <openssl/evp.h>
#include <openssl/rand.h>

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

  Worker(std::string &path, std::string &key, std::string &model, Napi::Function &callback)
      : Napi::AsyncProgressQueueWorker<WorkerCallbackResult>(callback), id(workerIds++), path(path), key(key), model(model)
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
      auto audioConfig = AudioConfig::FromDefaultMicrophoneInput();
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
      auto result = WorkerCallbackResult{StatusCode::END_SILENCE_TIMEOUT, e.what()};
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
};

const int GCM_TAG_LEN = 16;
const int GCM_IV_LEN = 12;

bool encrypt(const std::string &plainText, const std::string &key,
             std::string &cipherText, std::string &iv, std::string &tag)
{
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  if (!ctx)
  {
    return false;
  }

  if (1 != EVP_EncryptInit(ctx, EVP_aes_256_gcm(),
                           reinterpret_cast<const unsigned char *>(key.data()), nullptr))
  {
    EVP_CIPHER_CTX_free(ctx);
    return false;
  }

  std::vector<unsigned char> iv_buf(GCM_IV_LEN);
  RAND_bytes(iv_buf.data(), GCM_IV_LEN);
  iv = std::string(reinterpret_cast<char *>(iv_buf.data()), GCM_IV_LEN);

  if (1 != EVP_EncryptInit_ex(ctx, nullptr, nullptr, nullptr, iv_buf.data()))
  {
    EVP_CIPHER_CTX_free(ctx);
    return false;
  }

  int len;
  std::vector<unsigned char> cipher_buf(plainText.size());

  if (1 != EVP_EncryptUpdate(ctx, cipher_buf.data(), &len,
                             reinterpret_cast<const unsigned char *>(plainText.data()), static_cast<int>(plainText.size())))
  {
    EVP_CIPHER_CTX_free(ctx);
    return false;
  }

  int cipher_len = len;

  if (1 != EVP_EncryptFinal_ex(ctx, cipher_buf.data() + len, &len))
  {
    EVP_CIPHER_CTX_free(ctx);
    return false;
  }

  cipher_len += len;
  cipherText = std::string(reinterpret_cast<char *>(cipher_buf.data()), cipher_len);

  std::vector<unsigned char> tag_buf(GCM_TAG_LEN);
  if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, GCM_TAG_LEN, tag_buf.data()))
  {
    EVP_CIPHER_CTX_free(ctx);
    return false;
  }
  tag = std::string(reinterpret_cast<char *>(tag_buf.data()), GCM_TAG_LEN);

  EVP_CIPHER_CTX_free(ctx);
  return true;
}

bool decrypt(const std::string &cipherText, const std::string &key, const std::string &iv,
             const std::string &tag, std::string &plainText)
{
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  if (!ctx)
  {
    return false;
  }

  if (!EVP_DecryptInit(ctx, EVP_aes_256_gcm(),
                       reinterpret_cast<const unsigned char *>(key.data()), nullptr))
  {
    EVP_CIPHER_CTX_free(ctx);
    return false;
  }

  if (!EVP_DecryptInit_ex(ctx, nullptr, nullptr, nullptr,
                          reinterpret_cast<const unsigned char *>(iv.data())))
  {
    EVP_CIPHER_CTX_free(ctx);
    return false;
  }

  if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, GCM_TAG_LEN,
                           reinterpret_cast<void *>(const_cast<char *>(tag.data()))))
  {
    EVP_CIPHER_CTX_free(ctx);
    return false;
  }

  int len;
  std::vector<unsigned char> plain_buf(cipherText.size());

  if (!EVP_DecryptUpdate(ctx, plain_buf.data(), &len,
                         reinterpret_cast<const unsigned char *>(cipherText.data()), static_cast<int>(cipherText.size())))
  {
    EVP_CIPHER_CTX_free(ctx);
    return false;
  }

  int plain_len = len;

  if (EVP_DecryptFinal_ex(ctx, plain_buf.data() + len, &len) <= 0)
  {
    EVP_CIPHER_CTX_free(ctx);
    return false;
  }

  plain_len += len;
  plainText = std::string(reinterpret_cast<char *>(plain_buf.data()), plain_len);

  EVP_CIPHER_CTX_free(ctx);
  return true;
}

std::string derive_key(const std::string &input)
{
  const EVP_MD *md = EVP_sha256();
  unsigned char hash[256 / 8]; // SHA-256 produces a 256-bit or 32-byte hash
  unsigned int len;

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  EVP_DigestInit_ex(ctx, md, nullptr);
  EVP_DigestUpdate(ctx, input.data(), input.size());
  EVP_DigestFinal_ex(ctx, hash, &len);
  EVP_MD_CTX_free(ctx);

  return std::string(reinterpret_cast<char *>(hash), len);
}

Napi::Value Transcribe(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  std::string plainText = "api-speech-key";
  std::string rawKey = "You may only use the C/C++ Extension for Visual Studio Code and C# Extension for Visual Studio Code with Visual Studio Code, Visual Studio or Xamarin Studio software to help you develop and test your applications. The software is licensed, not sold. This agreement only gives you some rights to use the software. Microsoft reserves all other rights. You may not work around any technical limitations in the software; reverse engineer, decompile or disassemble the software remove, minimize, block or modify any notices of Microsoft or its suppliers in the software share, publish, rent, or lease the software, or provide the software as a stand-alone hosted as solution for others to use.";
  std::string derivedKey = derive_key(rawKey);
  std::string cipherText, iv, tag;

  if (encrypt(plainText, derivedKey, cipherText, iv, tag))
  {
    std::cout << "Encryption successful!" << std::endl;

    std::cout << "cipherText: ";
    for (unsigned char c : cipherText)
    {
      printf("%02x", c);
    }
    std::cout << std::endl;

    std::cout << "iv: ";
    for (unsigned char c : iv)
    {
      printf("%02x", c);
    }
    std::cout << std::endl;

    std::cout << "tag: ";
    for (unsigned char c : tag)
    {
      printf("%02x", c);
    }
  }
  else
  {
    std::cerr << "Encryption failed!" << std::endl;
  }

  std::string decryptedText;
  if (decrypt(cipherText, derivedKey, iv, tag, decryptedText))
  {
    std::cout << "Decryption successful: " << decryptedText << std::endl;
  }
  else
  {
    std::cerr << "Decryption failed!" << std::endl;
  }

  // Validate args
  if (info.Length() < 4)
  {
    Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  else if (!info[0].IsString() || !info[1].IsString() || !info[2].IsString() || !info[3].IsFunction())
  {
    Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  auto path = info[0].As<Napi::String>().Utf8Value();
  auto key = info[1].As<Napi::String>().Utf8Value();
  auto model = info[2].As<Napi::String>().Utf8Value();
  auto callback = info[3].As<Napi::Function>();

  Worker *worker = new Worker(path, key, model, callback);
  worker->Queue();

  return Napi::Number::New(env, worker->id);
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