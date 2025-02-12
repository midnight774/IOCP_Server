#pragma once

#pragma pack(1)
struct LoginData
{
	int		IdLength;
	char	Id[64];
	int		PwLength;
	char	Password[256];

};

struct RegisterData
{
	int		IdLength;
	char	Id[64];
	int		PwLength;
	char	Password[256];
};

struct CharacterMoveData
{
	LARGE_INTEGER Time;
	UINT	ObjectID;
	UINT8	Dir;
	float	PosX;
	float	PosY;
	bool	IsEnd;
};

struct SpawnCharacterData
{
	UINT	ObjectID;
	UINT	CharacterType;
	bool	IsLocal;
	float	PosX;
	float	PosY;
	UINT8	ViewDir;
};

#pragma pack()

