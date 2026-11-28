#include "provider.h"

#include <gtest/gtest.h>

#include <string>

namespace {

config::Values OpenAiSettings() {
  return {{"openai_model", "gpt-x"}, {"openai_api_key", "sk-1"}, {"openai_base_url", "https://api.example/v1"}};
}

config::Values AnthropicSettings() {
  return {{"anthropic_model", "claude-x"}, {"anthropic_api_key", "ak-1"}, {"anthropic_base_url", "https://anthropic.example"}};
}

config::Values AllSettings() {
  config::Values values = OpenAiSettings();
  for (const auto& entry : AnthropicSettings()) values.insert(entry);
  return values;
}

std::string MessageOf(const config::Values& settings, const std::string& name) {
  try {
    llm::MakeProvider(settings, name);
  } catch (const llm::Error& e) {
    return e.what();
  }
  return "";
}

}  // namespace

TEST(Provider, OpenAiChatComesFromTheOpenAiSettings) {
  const llm::Provider provider = llm::MakeProvider(OpenAiSettings(), "openai-chat");
  EXPECT_EQ(provider.name, "openai-chat");
  EXPECT_EQ(provider.protocol, llm::Protocol::kOpenAIChat);
  EXPECT_EQ(provider.model, "gpt-x");
  EXPECT_EQ(provider.endpoint.base_url, "https://api.example/v1");
  EXPECT_EQ(provider.endpoint.api_key, "sk-1");
}

TEST(Provider, OpenAiResponsesSharesTheOpenAiSettingsWithChat) {
  const llm::Provider provider = llm::MakeProvider(OpenAiSettings(), "openai-responses");
  EXPECT_EQ(provider.name, "openai-responses");
  EXPECT_EQ(provider.protocol, llm::Protocol::kOpenAIResponses);
  EXPECT_EQ(provider.model, "gpt-x");
  EXPECT_EQ(provider.endpoint.api_key, "sk-1");
}

TEST(Provider, AnthropicComesFromTheAnthropicSettings) {
  const llm::Provider provider = llm::MakeProvider(AnthropicSettings(), "anthropic");
  EXPECT_EQ(provider.protocol, llm::Protocol::kAnthropic);
  EXPECT_EQ(provider.model, "claude-x");
  EXPECT_EQ(provider.endpoint.base_url, "https://anthropic.example");
  EXPECT_EQ(provider.endpoint.api_key, "ak-1");
}

TEST(Provider, AMissingSettingIsNamedTogetherWithItsEnvironmentVariable) {
  config::Values settings = AnthropicSettings();
  settings.erase("anthropic_model");
  const std::string message = MessageOf(settings, "anthropic");
  EXPECT_NE(message.find("anthropic_model"), std::string::npos) << message;
  EXPECT_NE(message.find(config::EnvName("anthropic_model")), std::string::npos) << message;
}

TEST(Provider, AnUnknownNameIsAnError) {
  EXPECT_THROW(llm::MakeProvider(AllSettings(), "gemini"), llm::Error);
}

TEST(Provider, OnlyTheConfiguredProvidersAreListed) {
  const auto only_anthropic = llm::Providers(AnthropicSettings());
  ASSERT_EQ(only_anthropic.size(), 1u);
  EXPECT_EQ(only_anthropic[0].name, "anthropic");
  EXPECT_TRUE(llm::Providers({}).empty());
}

TEST(Provider, TheListIsInAFixedOrder) {
  const auto providers = llm::Providers(AllSettings());
  ASSERT_EQ(providers.size(), 3u);
  EXPECT_EQ(providers[0].name, "openai-chat");
  EXPECT_EQ(providers[1].name, "openai-responses");
  EXPECT_EQ(providers[2].name, "anthropic");
}

TEST(Provider, TheSettingsLlmProviderAndOpenAiApiChooseTheDefaultName) {
  EXPECT_EQ(llm::DefaultProviderName({{"llm_provider", "openai"}, {"openai_api", "chat"}}), "openai-chat");
  EXPECT_EQ(llm::DefaultProviderName({{"llm_provider", "openai"}, {"openai_api", "responses"}}), "openai-responses");
  // Chat Completions is the default, also when the setting is empty or missing.
  EXPECT_EQ(llm::DefaultProviderName({{"llm_provider", "openai"}, {"openai_api", ""}}), "openai-chat");
  EXPECT_EQ(llm::DefaultProviderName({{"llm_provider", "openai"}}), "openai-chat");
  // openai_api is not looked at when the provider is anthropic.
  EXPECT_EQ(llm::DefaultProviderName({{"llm_provider", "anthropic"}, {"openai_api", "nonsense"}}), "anthropic");
}

TEST(Provider, AnUnknownProviderOrOpenAiApiIsAnError) {
  EXPECT_THROW(llm::DefaultProviderName({{"llm_provider", "gemini"}}), llm::Error);
  EXPECT_THROW(llm::DefaultProviderName({{"llm_provider", ""}}), llm::Error);
  EXPECT_THROW(llm::DefaultProviderName({{"llm_provider", "openai"}, {"openai_api", "completions"}}), llm::Error);
}

TEST(Provider, OnlyChatCannotKeepReasoningBlocks) {
  const auto providers = llm::Providers(AllSettings());
  ASSERT_EQ(providers.size(), 3u);
  EXPECT_FALSE(providers[0].capabilities.keeps_reasoning_blocks);
  EXPECT_TRUE(providers[1].capabilities.keeps_reasoning_blocks);
  EXPECT_TRUE(providers[2].capabilities.keeps_reasoning_blocks);
  for (const llm::Provider& provider : providers) {
    EXPECT_GT(provider.capabilities.max_output_tokens, 0) << provider.name;
    EXPECT_GT(provider.capabilities.context_window, 0) << provider.name;
  }
}

TEST(Options, TheReasoningLevelComesFromTheSettingLlmReasoning) {
  EXPECT_EQ(llm::OptionsFromSettings({{"llm_reasoning", "off"}}).reasoning, llm::Reasoning::kOff);
  EXPECT_EQ(llm::OptionsFromSettings({{"llm_reasoning", "low"}}).reasoning, llm::Reasoning::kLow);
  EXPECT_EQ(llm::OptionsFromSettings({{"llm_reasoning", "medium"}}).reasoning, llm::Reasoning::kMedium);
  EXPECT_EQ(llm::OptionsFromSettings({{"llm_reasoning", "high"}}).reasoning, llm::Reasoning::kHigh);
  EXPECT_EQ(llm::OptionsFromSettings({{"llm_reasoning", ""}}).reasoning, llm::Reasoning::kOff);
  EXPECT_EQ(llm::OptionsFromSettings({}).reasoning, llm::Reasoning::kOff);
  EXPECT_EQ(llm::OptionsFromSettings({}).max_tokens, 1024);
}

TEST(Options, PromptCachingIsOnUnlessTheSettingPromptCacheSaysOff) {
  EXPECT_TRUE(llm::OptionsFromSettings({}).cache);
  EXPECT_TRUE(llm::OptionsFromSettings({{"prompt_cache", ""}}).cache);
  EXPECT_TRUE(llm::OptionsFromSettings({{"prompt_cache", "on"}}).cache);
  EXPECT_FALSE(llm::OptionsFromSettings({{"prompt_cache", "off"}}).cache);
  EXPECT_THROW(llm::OptionsFromSettings({{"prompt_cache", "maybe"}}), llm::Error);
}

TEST(Options, AnUnknownReasoningLevelIsAnError) {
  EXPECT_THROW(llm::OptionsFromSettings({{"llm_reasoning", "loud"}}), llm::Error);
}
