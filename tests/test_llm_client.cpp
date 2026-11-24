#include "llm_client.h"

#include <gtest/gtest.h>

#include <mutex>
#include <string>

#include "json.h"

#include "curl_transport.h"
#include "test_server.h"

namespace {

using Json = json::Value;

// Asks through the real libcurl transport and waits for the answer. Throws what llm::Ask throws.
llm::Reply Ask(const config::Values& settings, const std::string& prompt, const std::string& system) {
  boost::asio::io_context io;
  const auto transport = http::MakeCurlTransport();
  auto answer = boost::asio::co_spawn(io, llm::Ask(*transport, settings, prompt, system), boost::asio::use_future);
  io.run();
  return answer.get();
}

// What the fake service saw in the last request.
struct Seen {
  std::mutex mutex;
  std::string path;
  std::string authorization;
  std::string api_key;
  std::string version;
  Json body;
};

const char* kChatAnswer =
    R"({"choices": [{"message": {"role": "assistant", "content": "Hello from chat"}, "finish_reason": "stop"}],
        "usage": {"prompt_tokens": 7, "completion_tokens": 3}})";
const char* kResponsesAnswer =
    R"({"status": "completed",
        "output": [{"type": "message", "content": [{"type": "output_text", "text": "Hello from responses"}]}],
        "usage": {"input_tokens": 7, "output_tokens": 3}})";
const char* kAnthropicAnswer =
    R"({"content": [{"type": "text", "text": "Hello from anthropic"}], "stop_reason": "end_turn",
        "usage": {"input_tokens": 7, "output_tokens": 3}})";

// Registers a handler on `path` that records the request and answers with `status` and `answer`.
void Serve(TestServer& server, const std::string& path, Seen& seen, int status, const char* answer) {
  server.server().Post(path, [&seen, path, status, answer](const httplib::Request& req, httplib::Response& res) {
    {
      std::lock_guard<std::mutex> lock(seen.mutex);
      seen.path = path;
      seen.authorization = req.get_header_value("Authorization");
      seen.api_key = req.get_header_value("x-api-key");
      seen.version = req.get_header_value("anthropic-version");
      seen.body = json::Parse(req.body);
    }
    res.status = status;
    res.set_content(answer, "application/json");
  });
}

config::Values Settings(const std::string& provider, const TestServer& server,
                        const std::string& openai_api = "chat") {
  return {{"llm_provider", provider},
          {"openai_api", openai_api},
          {"openai_model", "openai-model"},
          {"openai_api_key", "sk-openai"},
          {"openai_base_url", server.Url("/v1")},
          {"anthropic_model", "anthropic-model"},
          {"anthropic_api_key", "sk-anthropic"},
          {"anthropic_base_url", server.Url()}};
}

TEST(LlmAsk, OpenAiChatSendsTheRequestAndReturnsTheText) {
  TestServer server;
  Seen seen;
  Serve(server, "/v1/chat/completions", seen, 200, kChatAnswer);
  server.Start();

  const llm::Reply reply = Ask(Settings("openai", server), "Say hello.", "Be brief.");
  EXPECT_EQ(reply.text, "Hello from chat");
  EXPECT_EQ(reply.stop_reason, "stop");
  EXPECT_EQ(reply.input_tokens, 7);
  EXPECT_EQ(seen.authorization, "Bearer sk-openai");
  EXPECT_EQ(seen.body["model"], "openai-model");  // the model of the openai_* settings
  EXPECT_EQ(seen.body["messages"].size(), 2u);
}

TEST(LlmAsk, OpenAiResponsesSendsTheRequestAndReturnsTheText) {
  TestServer server;
  Seen seen;
  Serve(server, "/v1/responses", seen, 200, kResponsesAnswer);
  server.Start();

  const llm::Reply reply = Ask(Settings("openai", server, "responses"), "Say hello.", "Be brief.");
  EXPECT_EQ(reply.text, "Hello from responses");
  EXPECT_EQ(seen.authorization, "Bearer sk-openai");
  EXPECT_EQ(seen.body["model"], "openai-model");
  EXPECT_EQ(seen.body["instructions"], "Be brief.");
  EXPECT_EQ(seen.body["input"], "Say hello.");
}

TEST(LlmAsk, AnthropicSendsTheRequestAndReturnsTheText) {
  TestServer server;
  Seen seen;
  Serve(server, "/v1/messages", seen, 200, kAnthropicAnswer);
  server.Start();

  const llm::Reply reply = Ask(Settings("anthropic", server), "Say hello.", "");
  EXPECT_EQ(reply.text, "Hello from anthropic");
  EXPECT_EQ(reply.stop_reason, "end_turn");
  EXPECT_EQ(seen.api_key, "sk-anthropic");
  EXPECT_EQ(seen.authorization, "");
  EXPECT_FALSE(seen.version.empty());
  EXPECT_EQ(seen.body["model"], "anthropic-model");  // not the OpenAI one
  EXPECT_FALSE(seen.body.contains("system"));
}

TEST(LlmAsk, AnErrorStatusBecomesAnErrorWithTheServicesMessageAndAHintForA401) {
  TestServer server;
  Seen seen;
  Serve(server, "/v1/messages", seen, 401,
        R"({"type": "error", "error": {"type": "authentication_error", "message": "invalid x-api-key"}})");
  server.Start();

  try {
    Ask(Settings("anthropic", server), "Say hello.", "");
    FAIL() << "expected llm::Error";
  } catch (const llm::Error& e) {
    const std::string message = e.what();
    EXPECT_NE(message.find("HTTP 401"), std::string::npos) << message;
    EXPECT_NE(message.find("authentication_error: invalid x-api-key"), std::string::npos) << message;
    EXPECT_NE(message.find("check the API key"), std::string::npos) << message;
    EXPECT_EQ(message.find("sk-anthropic"), std::string::npos) << "the key must never be in a message";
  }
}

TEST(LlmAsk, ASuccessfulAnswerThatIsNotTheExpectedJsonIsAnError) {
  TestServer server;
  Seen seen;
  Serve(server, "/v1/chat/completions", seen, 200, R"({"unexpected": true})");
  server.Start();
  EXPECT_THROW(Ask(Settings("openai", server), "Say hello.", ""), llm::Error);
}

TEST(LlmAsk, MissingSettingsAreReportedByName) {
  TestServer server;
  server.Start();

  config::Values no_key = Settings("openai", server);
  no_key["openai_api_key"] = "";
  try {
    Ask(no_key, "Say hello.", "");
    FAIL() << "expected llm::Error";
  } catch (const llm::Error& e) {
    EXPECT_NE(std::string(e.what()).find("openai_api_key"), std::string::npos) << e.what();
    EXPECT_NE(std::string(e.what()).find("OPENAI_API_KEY"), std::string::npos) << e.what();
  }

  // Each service has its own model: a model set for OpenAI does not help the Anthropic call.
  config::Values no_model = Settings("anthropic", server);
  no_model["anthropic_model"] = "";
  try {
    Ask(no_model, "Say hello.", "");
    FAIL() << "expected llm::Error";
  } catch (const llm::Error& e) {
    EXPECT_NE(std::string(e.what()).find("anthropic_model"), std::string::npos) << e.what();
  }
}

TEST(LlmAsk, TheProviderAndTheOpenAiApiChooseTheProtocol) {
  EXPECT_EQ(llm::ChooseProtocol({{"llm_provider", "openai"}, {"openai_api", "chat"}}),
            llm::Protocol::kOpenAIChat);
  EXPECT_EQ(llm::ChooseProtocol({{"llm_provider", "openai"}, {"openai_api", "responses"}}),
            llm::Protocol::kOpenAIResponses);
  // Chat Completions is the default for OpenAI, also when openai_api is empty or missing
  EXPECT_EQ(llm::ChooseProtocol({{"llm_provider", "openai"}, {"openai_api", ""}}), llm::Protocol::kOpenAIChat);
  EXPECT_EQ(llm::ChooseProtocol({{"llm_provider", "openai"}}), llm::Protocol::kOpenAIChat);
  // openai_api is only looked at for OpenAI
  EXPECT_EQ(llm::ChooseProtocol({{"llm_provider", "anthropic"}, {"openai_api", "nonsense"}}),
            llm::Protocol::kAnthropic);
}

TEST(LlmAsk, AnUnknownProviderOrOpenAiApiIsAnError) {
  EXPECT_THROW(llm::ChooseProtocol({{"llm_provider", "gemini"}}), llm::Error);
  EXPECT_THROW(llm::ChooseProtocol({{"llm_provider", ""}}), llm::Error);
  EXPECT_THROW(llm::ChooseProtocol({{"llm_provider", "openai"}, {"openai_api", "completions"}}), llm::Error);
}

TEST(LlmAsk, ANonexistentServerThrowsAnHttpError) {
  int closed_port = 0;
  {
    TestServer server;
    server.Start();
    closed_port = server.port();
  }
  config::Values settings = {{"llm_provider", "openai"},
                             {"openai_api", "chat"},
                             {"openai_model", "m"},
                             {"openai_api_key", "k"},
                             {"openai_base_url", "http://127.0.0.1:" + std::to_string(closed_port) + "/v1"}};
  EXPECT_THROW(Ask(settings, "hi", ""), http::Error);
}

}  // namespace
