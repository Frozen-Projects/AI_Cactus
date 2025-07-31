# AI Cactus
This is an Unreal Engine 5.6 and up third party library plugin for integrating Cactus AI framework to the engine.

## Description
This is a runtime plugin. So, you have to use it on PIE, Stand Alone or Packaged project. It won't work directly on editor.</br>
It uses actor classes. So, you can have multiple conversation for each instance (for example NPC) and give different models. But you have to consider your target machine's hardware resources. Because AI projects consume too much of that eventhough cactus is really lighweight.</br>

## Current Features
- Generate Text : It will answer your question without storing it to conversation history. Think it as an temporary chat.
- Run Conversation : Each call of this function will record your question to conversation history.
- Clear Conversation
- Export and Import Conversation : This will allow to export conversation as "UE5 .sav" file. So, you can create persistent conversations. For example, level loads or NPCs can remember you after restarting your game.

## Roadmap
- Multimodal, VLM, TTS features.
- It currently works on Windows. I haven't tested it for packaging.

## Caution
- I don't have Apple and Linux devices. So, I can't offer support for other than Windows and Android.

## Sample Models
LLM Test : https://huggingface.co/QuantFactory/SmolLM-360M-Instruct-GGUF/resolve/main/SmolLM-360M-Instruct.Q6_K.gguf </br>
VLM Model : https://huggingface.co/ggml-org/SmolVLM-256M-Instruct-GGUF/resolve/main/SmolVLM-256M-Instruct-Q8_0.gguf </br>
VLM Proj : https://huggingface.co/ggml-org/SmolVLM-256M-Instruct-GGUF/resolve/main/mmproj-SmolVLM-256M-Instruct-Q8_0.gguf </br>