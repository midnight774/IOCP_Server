#pragma once
#include "SockComm.h"

class CPacket;

class CPacketSerializer
{
public:
	static bool SerializeCharacterSpawns(const std::vector<SpawnCharacterData>& InDatas, char* OutBuffer, int& OutSize);
	static bool DeserializeCharacterSpawns(const char* InBuffer, int InSize, std::vector<SpawnCharacterData>& OutDatas);

	static bool SerializeCharacterMoves(const std::vector<CharacterMoveData>& InDatas, char* OutBuffer, int& OutSize);
	static bool DeserializeCharacterMoves(const char* InBuffer, const int InSize, std::vector<CharacterMoveData>& OutDatas);

};

