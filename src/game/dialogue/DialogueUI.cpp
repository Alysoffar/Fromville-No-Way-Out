#include "DialogueUI.h"

#include "engine/renderer/Shader.h"
#include "engine/renderer/TextRenderer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <iostream>

#include <glad/glad.h>

namespace {
struct OverlayResources {
	Shader shader{ "DialogueOverlay" };
	unsigned int vao = 0;
	unsigned int vbo = 0;
	unsigned int texture = 0;
	bool ready = false;
};

OverlayResources& GetOverlayResources() {
	static OverlayResources resources;
	return resources;
}

glm::vec3 EmotionColor(EmotionState emotion) {
	switch (emotion) {
		case EmotionState::Tense: return glm::vec3(0.90f, 0.86f, 0.66f);
		case EmotionState::Angry: return glm::vec3(0.96f, 0.58f, 0.52f);
		case EmotionState::Sad: return glm::vec3(0.74f, 0.82f, 0.92f);
		case EmotionState::Fearful: return glm::vec3(0.82f, 0.88f, 1.0f);
		case EmotionState::Calm: return glm::vec3(0.86f, 0.90f, 0.84f);
		case EmotionState::Neutral:
		default:
			return glm::vec3(0.93f, 0.91f, 0.86f);
	}
}

bool EndsWith(const std::string& text, char ch) {
	return !text.empty() && text.back() == ch;
}
}

DialogueUI::DialogueUI() = default;

void DialogueUI::OpenConversation(const Conversation& convo, const std::string& startNodeId) {
	activeConvo = &convo;
	currentNodeId = startNodeId;
	open = true;
	closing = false;
	overlayAlpha = 0.0f;
	std::cout << "[DialogueUI] Open conversation: " << convo.title << "\n";
	ResetLineState();
}

void DialogueUI::AdvanceToNode(const std::string& nodeId) {
	if (!open || !activeConvo) {
		return;
	}

	currentNodeId = nodeId;
	std::cout << "[DialogueUI] Advance to node: " << nodeId << "\n";
	ResetLineState();
}

void DialogueUI::Update(float dt) {
	if (!open) {
		return;
	}

	const float targetAlpha = closing ? 0.0f : 1.0f;
	const float fadeSpeed = 5.5f;
	overlayAlpha += (targetAlpha - overlayAlpha) * std::clamp(dt * fadeSpeed, 0.0f, 1.0f);

	if (closing && overlayAlpha <= 0.01f) {
		open = false;
		closing = false;
		activeConvo = nullptr;
		currentNodeId.clear();
		return;
	}

	if (!activeConvo) {
		return;
	}

	const DialogueNode* node = GetCurrentNode();
	if (!node) {
		closing = true;
		return;
	}

	if (preDelayRemaining > 0.0f) {
		preDelayRemaining = std::max(0.0f, preDelayRemaining - dt);
		return;
	}

	if (!fullyRevealed) {
		revealProgress += node->revealCharactersPerSecond * dt;
		const float textLength = static_cast<float>(node->text.size());
		if (revealProgress >= textLength) {
			revealProgress = textLength;
			fullyRevealed = true;
			postDelayRemaining = std::max(node->postDelaySeconds, GetPunctuationSilence(node->text));
		}
		return;
	}

	if (postDelayRemaining > 0.0f) {
		postDelayRemaining = std::max(0.0f, postDelayRemaining - dt);
	}
}

void DialogueUI::Render(TextRenderer* hud, int screenWidth, int screenHeight) const {
	if (!open || !activeConvo || overlayAlpha <= 0.0f) {
		return;
	}

	const DialogueNode* node = GetCurrentNode();
	if (!node) {
		return;
	}

	RenderOverlay(overlayAlpha, screenWidth, screenHeight);

	if (!hud) {
		return;
	}

	const float panelLeft = static_cast<float>(screenWidth) * 0.08f;
	const float panelTop = static_cast<float>(screenHeight) * 0.12f;
	const float panelHeight = static_cast<float>(screenHeight) * 0.34f;
	const float panelBottom = panelTop + panelHeight;
	const float readableLimit = std::max(40.0f, static_cast<float>(screenWidth) / 22.0f);

	const float visibleLength = std::clamp(revealProgress, 0.0f, static_cast<float>(node->text.size()));
	const std::string visibleText = node->text.substr(0, static_cast<std::size_t>(visibleLength));
	const std::string wrappedText = WrapText(visibleText, static_cast<std::size_t>(readableLimit));

	hud->RenderText(node->speaker, panelLeft + 10.0f, panelBottom - 52.0f, 0.62f, EmotionColor(node->emotion), screenWidth, screenHeight, true);
	hud->RenderText(wrappedText, panelLeft + 12.0f, panelBottom - 98.0f, 0.66f, glm::vec3(0.94f, 0.92f, 0.87f), screenWidth, screenHeight, false);

	if (node->choices.empty()) {
		return;
	}

	float choiceY = panelTop + 30.0f;
	for (std::size_t i = 0; i < node->choices.size(); ++i) {
		const bool selected = static_cast<int>(i) == selectedChoiceIndex;
		const glm::vec3 choiceColor = selected ? glm::vec3(0.98f, 0.92f, 0.68f) : glm::vec3(0.78f, 0.80f, 0.76f);
		const std::string prefix = selected ? "> " : "  ";
		const std::string choiceText = WrapText(prefix + node->choices[i].text, static_cast<std::size_t>(readableLimit * 0.88f));
		hud->RenderText(choiceText, panelLeft + 18.0f, choiceY, 0.50f, choiceColor, screenWidth, screenHeight, false);
		choiceY += 28.0f;
	}
}

void DialogueUI::Close() {
	closing = true;
	std::cout << "[DialogueUI] Close conversation\n";
}

void DialogueUI::MoveSelection(int delta) {
	const DialogueNode* node = GetCurrentNode();
	if (!node || node->choices.empty()) {
		selectedChoiceIndex = 0;
		return;
	}

	const int choiceCount = static_cast<int>(node->choices.size());
	selectedChoiceIndex = (selectedChoiceIndex + delta) % choiceCount;
	if (selectedChoiceIndex < 0) {
		selectedChoiceIndex += choiceCount;
	}
}

void DialogueUI::SetSelectedChoiceIndex(int index) {
	const DialogueNode* node = GetCurrentNode();
	if (!node || node->choices.empty()) {
		selectedChoiceIndex = 0;
		return;
	}

	const int choiceCount = static_cast<int>(node->choices.size());
	selectedChoiceIndex = std::clamp(index, 0, choiceCount - 1);
}

bool DialogueUI::IsWaitingForInput() const {
	return open && activeConvo && fullyRevealed && preDelayRemaining <= 0.0f && postDelayRemaining <= 0.0f;
}

void DialogueUI::RevealFully() {
	const DialogueNode* node = GetCurrentNode();
	if (!node) {
		return;
	}

	revealProgress = static_cast<float>(node->text.size());
	fullyRevealed = true;
	preDelayRemaining = 0.0f;
	postDelayRemaining = 0.0f;
}

const DialogueNode* DialogueUI::GetCurrentNode() const {
	if (!activeConvo) {
		return nullptr;
	}

	const auto it = activeConvo->nodes.find(currentNodeId);
	if (it == activeConvo->nodes.end()) {
		return nullptr;
	}

	return &it->second;
}

bool DialogueUI::HasChoices() const {
	const DialogueNode* node = GetCurrentNode();
	return node && !node->choices.empty();
}

void DialogueUI::ResetLineState() {
	selectedChoiceIndex = 0;
	revealProgress = 0.0f;
	fullyRevealed = false;
	postDelayRemaining = 0.0f;

	const DialogueNode* node = GetCurrentNode();
	if (!node) {
		preDelayRemaining = 0.0f;
		return;
	}

	switch (node->timing) {
		case DialogueTiming::Immediate: preDelayRemaining = node->preDelaySeconds; break;
		case DialogueTiming::PauseShort: preDelayRemaining = std::max(node->preDelaySeconds, 0.45f); break;
		case DialogueTiming::PauseLong: preDelayRemaining = std::max(node->preDelaySeconds, 1.05f); break;
		case DialogueTiming::TimedResponse: preDelayRemaining = std::max(node->preDelaySeconds, 0.25f); break;
	}
}

float DialogueUI::GetPunctuationSilence(const std::string& text) const {
	if (text.find("...") != std::string::npos) {
		return 1.0f;
	}

	if (EndsWith(text, '?') || EndsWith(text, '!')) {
		return 0.55f;
	}
	if (EndsWith(text, ',') || EndsWith(text, ';') || EndsWith(text, ':')) {
		return 0.35f;
	}
	if (EndsWith(text, '-')) {
		return 0.75f;
	}

	return 0.45f;
}

std::string DialogueUI::WrapText(const std::string& text, std::size_t maxLineChars) const {
	if (text.size() <= maxLineChars) {
		return text;
	}

	std::string wrapped;
	std::string line;

	std::size_t position = 0;
	while (position < text.size()) {
		const std::size_t nextSpace = text.find(' ', position);
		const std::string word = text.substr(position, nextSpace == std::string::npos ? std::string::npos : nextSpace - position);
		if (!word.empty()) {
			const std::size_t projected = line.empty() ? word.size() : line.size() + 1 + word.size();
			if (projected > maxLineChars && !line.empty()) {
				if (!wrapped.empty()) {
					wrapped.push_back('\n');
				}
				wrapped += line;
				line.clear();
			}

			if (!line.empty()) {
				line.push_back(' ');
			}
			line += word;
		}

		if (nextSpace == std::string::npos) {
			break;
		}
		position = nextSpace + 1;
	}

	if (!line.empty()) {
		if (!wrapped.empty()) {
			wrapped.push_back('\n');
		}
		wrapped += line;
	}

	return wrapped;
}

void DialogueUI::EnsureOverlayResources() const {
	OverlayResources& resources = GetOverlayResources();
	if (resources.ready) {
		return;
	}

	resources.shader.Load("assets/shaders/ui.vert", "assets/shaders/ui.frag");

	const float vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		 1.0f, -1.0f, 1.0f, 0.0f,
		 1.0f,  1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 1.0f, 1.0f,
		-1.0f,  1.0f, 0.0f, 1.0f,
	};

	glGenVertexArrays(1, &resources.vao);
	glGenBuffers(1, &resources.vbo);
	glBindVertexArray(resources.vao);
	glBindBuffer(GL_ARRAY_BUFFER, resources.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));

	glGenTextures(1, &resources.texture);
	glBindTexture(GL_TEXTURE_2D, resources.texture);
	const unsigned char whitePixel[4] = {255, 255, 255, 255};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	resources.ready = resources.vao != 0 && resources.vbo != 0 && resources.texture != 0;
}

void DialogueUI::RenderOverlay(float alpha, int screenWidth, int screenHeight) const {
	(void)screenWidth;
	(void)screenHeight;

	EnsureOverlayResources();
	OverlayResources& resources = GetOverlayResources();
	if (!resources.ready) {
		return;
	}

	const float dimAlpha = std::clamp(alpha * 0.42f, 0.0f, 0.42f);
	const float panelAlpha = std::clamp(alpha * 0.92f, 0.0f, 0.92f);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	resources.shader.Bind();
	resources.shader.SetInt("overlay", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, resources.texture);
	glBindVertexArray(resources.vao);

	resources.shader.SetVec4("tintColor", glm::vec4(0.02f, 0.02f, 0.03f, dimAlpha));
	glDrawArrays(GL_TRIANGLES, 0, 6);

	const float left = -0.92f;
	const float right = 0.92f;
	const float top = -0.72f;
	const float bottom = -0.98f;
	const float panelVertices[] = {
		left,  bottom, 0.0f, 0.0f,
		right, bottom, 1.0f, 0.0f,
		right, top,    1.0f, 1.0f,
		left,  bottom, 0.0f, 0.0f,
		right, top,    1.0f, 1.0f,
		left,  top,    0.0f, 1.0f,
	};
	glBindBuffer(GL_ARRAY_BUFFER, resources.vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(panelVertices), panelVertices);
	resources.shader.SetVec4("tintColor", glm::vec4(0.08f, 0.08f, 0.10f, panelAlpha));
	glDrawArrays(GL_TRIANGLES, 0, 6);

	const float lineVertices[] = {
		left, top + 0.02f, 0.0f, 0.0f,
		right, top + 0.02f, 1.0f, 0.0f,
		right, top + 0.04f, 1.0f, 1.0f,
		left, top + 0.02f, 0.0f, 0.0f,
		right, top + 0.04f, 1.0f, 1.0f,
		left, top + 0.04f, 0.0f, 1.0f,
	};
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lineVertices), lineVertices);
	resources.shader.SetVec4("tintColor", glm::vec4(0.68f, 0.64f, 0.50f, panelAlpha * 0.45f));
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	resources.shader.Unbind();
	glEnable(GL_DEPTH_TEST);
}