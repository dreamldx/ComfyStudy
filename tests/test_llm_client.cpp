#include "llm_client.h"

#include <gtest/gtest.h>

#include <mutex>
#include <string>

#include "json.h"

#include "curl_transport.h"
#include "scripted_transport.h"
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
  EXPECT_EQ(reply.Text(), "Hello from chat");
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
  EXPECT_EQ(reply.Text(), "Hello from responses");
  EXPECT_EQ(seen.authorization, "Bearer sk-openai");
  EXPECT_EQ(seen.body["model"], "openai-model");
  EXPECT_EQ(seen.body["instructions"], "Be brief.");
  EXPECT_EQ(seen.body["input"], (json::Array({{{"role", "user"}, {"content", "Say hello."}}})));
}

TEST(LlmAsk, AnthropicSendsTheRequestAndReturnsTheText) {
  TestServer server;
  Seen seen;
  Serve(server, "/v1/messages", seen, 200, kAnthropicAnswer);
  server.Start();

  const llm::Reply reply = Ask(Settings("anthropic", server), "Say hello.", "");
  EXPECT_EQ(reply.Text(), "Hello from anthropic");
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

// ---- Generate, with a scripted transport --------------------------------------------------------

llm::Provider ProviderOf(llm::Protocol protocol) {
  const char* name = protocol == llm::Protocol::kAnthropic ? "anthropic"
                     : protocol == llm::Protocol::kOpenAIChat ? "openai-chat" : "openai-responses";
  return llm::Provider{name, protocol, llm::Endpoint{"https://example.test", "key"}, "m", {true, 1000, 100000}};
}

llm::Reply Generate(http::Transport& transport, const llm::Provider& provider, const llm::Conversation& conversation,
                    const llm::Options& options = {}) {
  boost::asio::io_context io;
  auto answer = boost::asio::co_spawn(io, llm::Generate(transport, provider, conversation, options),
                                      boost::asio::use_future);
  io.run();
  return answer.get();
}

llm::Message User(const std::string& text) { return {llm::Role::kUser, {llm::TextBlock{text}}}; }

const Json kThinking = {{"type", "thinking"}, {"thinking", "Plan"}, {"signature", "SIG=="}};

llm::Reply AnthropicAnswer(const std::string& text, bool with_thinking) {
  llm::Reply reply;
  reply.message.role = llm::Role::kAssistant;
  if (with_thinking) reply.message.blocks.push_back(llm::ReasoningBlock{llm::Protocol::kAnthropic, kThinking, "Plan"});
  reply.message.blocks.push_back(llm::TextBlock{text});
  reply.stop_reason = "end_turn";
  reply.input_tokens = 7;
  reply.output_tokens = 3;
  return reply;
}

TEST(LlmGenerate, TheWholeConversationIsSentAndTheReasoningBlockComesBackAsItWas) {
  ScriptedTransport transport({{200, AnthropicBody(AnthropicAnswer("Hi", true))}, {200, AnthropicBody(AnthropicAnswer("Bye", false))}});
  const llm::Provider provider = ProviderOf(llm::Protocol::kAnthropic);

  llm::Conversation conversation;
  conversation.messages.push_back(User("hello"));
  const llm::Reply first = Generate(transport, provider, conversation);
  ASSERT_EQ(first.message.blocks.size(), 2u);
  EXPECT_EQ(first.Text(), "Hi");

  conversation.messages.push_back(first.message);
  conversation.messages.push_back(User("again"));
  const llm::Reply second = Generate(transport, provider, conversation);
  EXPECT_EQ(second.Text(), "Bye");

  const auto seen = transport.seen();
  ASSERT_EQ(seen.size(), 2u);
  EXPECT_EQ(seen[0].url, "https://example.test/v1/messages");
  const Json body = json::Parse(seen[1].body);
  ASSERT_EQ(body["messages"].size(), 3u);
  EXPECT_EQ(body["messages"][1]["content"][0], kThinking);
  EXPECT_EQ(body["messages"][1]["content"][1], (Json{{"type", "text"}, {"text", "Hi"}}));
  EXPECT_EQ(body["messages"][2]["content"][0]["text"], "again");
}

TEST(LlmGenerate, AReasoningBlockOfAnotherProtocolIsNotSentButStaysInTheConversation) {
  llm::Conversation conversation;
  conversation.messages.push_back(User("hello"));
  conversation.messages.push_back(AnthropicAnswer("Hi", true).message);
  conversation.messages.push_back(User("again"));

  ScriptedTransport to_responses({{200, ResponsesBody(AnthropicAnswer("Bye", false))}});
  Generate(to_responses, ProviderOf(llm::Protocol::kOpenAIResponses), conversation);
  const Json body = json::Parse(to_responses.seen()[0].body);
  ASSERT_EQ(body["input"].size(), 3u);  // user, assistant text, user: no thinking item
  EXPECT_EQ(body["input"][1], (Json{{"role", "assistant"}, {"content", "Hi"}}));
  EXPECT_EQ(conversation.messages[1].blocks.size(), 2u);  // the conversation is unchanged
  EXPECT_EQ(llm::CountForeignReasoning(conversation, llm::Protocol::kOpenAIResponses), 1u);

  ScriptedTransport back({{200, AnthropicBody(AnthropicAnswer("Bye", false))}});
  Generate(back, ProviderOf(llm::Protocol::kAnthropic), conversation);
  EXPECT_EQ(json::Parse(back.seen()[0].body)["messages"][1]["content"][0], kThinking);  // and it is sent again
}

TEST(LlmGenerate, AnErrorStatusBecomesAnErrorWithTheServicesMessageAndAHintForA401) {
  ScriptedTransport transport({{401, R"({"type": "error", "error": {"type": "authentication_error", "message": "invalid x-api-key"}})"}});
  llm::Conversation conversation;
  conversation.messages.push_back(User("hello"));
  try {
    Generate(transport, ProviderOf(llm::Protocol::kAnthropic), conversation);
    FAIL() << "expected llm::Error";
  } catch (const llm::Error& e) {
    const std::string message = e.what();
    EXPECT_NE(message.find("401"), std::string::npos) << message;
    EXPECT_NE(message.find("invalid x-api-key"), std::string::npos) << message;
    EXPECT_NE(message.find("check the API key"), std::string::npos) << message;
  }
}

TEST(LlmGenerate, ScriptedBodiesAreDecodedBackIntoTheReplyTheyWereMadeFrom) {
  const Json responses_item = {{"type", "reasoning"}, {"id", "rs_1"}, {"summary", json::Array()}, {"encrypted_content", "E=="}};
  llm::Reply reply;
  reply.message.role = llm::Role::kAssistant;
  reply.message.blocks.push_back(llm::ReasoningBlock{llm::Protocol::kOpenAIResponses, responses_item, ""});
  reply.message.blocks.push_back(llm::TextBlock{"Hello"});
  reply.stop_reason = "completed";
  reply.input_tokens = 7;
  reply.output_tokens = 3;

  const llm::Reply from_responses = llm::openai_responses::ParseReply(ResponsesBody(reply));
  ASSERT_EQ(from_responses.message.blocks.size(), 2u);
  EXPECT_EQ(std::get<llm::ReasoningBlock>(from_responses.message.blocks[0]).payload, responses_item);
  EXPECT_EQ(from_responses.Text(), "Hello");
  EXPECT_EQ(from_responses.stop_reason, "completed");
  EXPECT_EQ(from_responses.input_tokens, 7);
  EXPECT_EQ(from_responses.output_tokens, 3);

  llm::Reply anthropic_reply = AnthropicAnswer("Hello", true);
  const llm::Reply from_anthropic = llm::anthropic::ParseReply(AnthropicBody(anthropic_reply));
  ASSERT_EQ(from_anthropic.message.blocks.size(), 2u);
  EXPECT_EQ(std::get<llm::ReasoningBlock>(from_anthropic.message.blocks[0]).payload, kThinking);
  EXPECT_EQ(from_anthropic.stop_reason, "end_turn");

  // Chat Completions has no reasoning blocks: only the text is rendered
  const llm::Reply from_chat = llm::openai_chat::ParseReply(ChatBody(anthropic_reply));
  ASSERT_EQ(from_chat.message.blocks.size(), 1u);
  EXPECT_EQ(from_chat.Text(), "Hello");
}

TEST(LlmGenerate, ARequestBeyondTheScriptIsAFailureOfTheTest) {
  ScriptedTransport transport({});
  llm::Conversation conversation;
  conversation.messages.push_back(User("hello"));
  EXPECT_THROW(Generate(transport, ProviderOf(llm::Protocol::kOpenAIChat), conversation), std::logic_error);
}

}  // namespace
