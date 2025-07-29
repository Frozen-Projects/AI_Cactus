# AI Cactus
This is an Unreal Engine 5.6 and up third party library plugin for integrating Cactus AI framework to the engine.

## Description
This is a runtime plugin. So, you have to use it on PIE, Stand Alone or Packaged project. It won't work directly on editor.</br>
It uses actor classes. So, you can have multiple conversation for each instance (for example NPC) and give different models. But you have to consider your target machine's hardware resources. Because AI projects consume too much of that eventhough cactus is really lighweight.</br>

## ROADMAP
- Import and export system : It will allow us persistent conversation between level loads and after restarting project.
- Multimodal, VLM, TTS features.