#include "PacketSerializer.h"

bool CPacketSerializer::SerializeCharacterSpawns(const std::vector<SpawnCharacterData>& InDatas, char* OutBuffer, int& OutSize)
{
	Packet_Type Type = Packet_Type::Spawn;
	int Stride = 0;
	size_t DataCnt = InDatas.size();
	memcpy(OutBuffer, &Type, sizeof(UINT8));//패킷 처음에 패킷 타입 정보를 담는다.
	Stride += sizeof(UINT8);
	memcpy(OutBuffer + Stride, &DataCnt, sizeof(int));
	Stride += sizeof(int);

	for (int i = 0; i < DataCnt; ++i)
	{
		memcpy(OutBuffer + Stride, &InDatas[i], sizeof(SpawnCharacterData));
		Stride += sizeof(SpawnCharacterData);
	}

	OutSize = Stride;

	return true;
}

bool CPacketSerializer::DeserializeCharacterSpawns(const char* InBuffer, int InSize, std::vector<SpawnCharacterData>& OutDatas)
{
	int Stride = sizeof(Packet_Type);

	int DataCnt = 0;
	memcpy(&DataCnt, InBuffer + Stride, sizeof(int));
	Stride += sizeof(int);

	OutDatas.resize(DataCnt);

	for (int i = 0; i < DataCnt; ++i)
	{
		memcpy(&OutDatas[i], InBuffer + Stride, sizeof(SpawnCharacterData));
		Stride += sizeof(SpawnCharacterData);
	}

	return true;
}

bool CPacketSerializer::SerializeCharacterMoves(const std::vector<CharacterMoveData>& InDatas, char* OutBuffer, int& OutSize)
{
	Packet_Type Type = Packet_Type::CharacterMove;

	size_t DataCnt = InDatas.size();
	int Stride = 0;
	memcpy(OutBuffer, &Type, sizeof(UINT8));//패킷 처음에 패킷 타입 정보를 담는다.
	Stride += sizeof(UINT8);
	memcpy(OutBuffer + Stride, &DataCnt, sizeof(int));
	Stride += sizeof(int);

	for (int i = 0; i < DataCnt; ++i)
	{
		memcpy(OutBuffer + Stride, &InDatas[i], sizeof(CharacterMoveData));
		Stride += sizeof(CharacterMoveData);
	}

	OutSize = Stride;

	return true;
}

bool CPacketSerializer::DeserializeCharacterMoves(const char* InBuffer, const int InSize, std::vector<CharacterMoveData>& OutDatas)
{
	int Stride = sizeof(Packet_Type);

	int DataCnt = 0;
	memcpy(&DataCnt, InBuffer + Stride, sizeof(int));
	Stride += sizeof(int);

	OutDatas.resize(DataCnt);

	for (int i = 0; i < DataCnt; ++i)
	{
		memcpy(&OutDatas[i], InBuffer + Stride, sizeof(CharacterMoveData));
		Stride += sizeof(CharacterMoveData);
	}

	return true;
}
