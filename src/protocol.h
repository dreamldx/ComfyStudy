#pragma once

namespace llm {

// The request formats we speak. A service can speak more than one (OpenAI has two), and several
// services can speak the same one (OpenRouter and OpenAI both take Chat Completions).
enum class Protocol { kOpenAIChat, kOpenAIResponses, kAnthropic };

}  // namespace llm
