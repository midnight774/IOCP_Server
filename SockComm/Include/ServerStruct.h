#pragma once

#pragma pack(1)
struct LoginData
{
	int		IdLength;
	char	Id[64];
	int		PwLength;
	char	Password[256];

};
#pragma pack()

#pragma pack(1)
struct RegisterData
{
	int		IdLength;
	char	Id[64];
	int		PwLength;
	char	Password[256];
};
#pragma pack(1)

struct CharacterMoveData
{
	UINT8 Dir;
	float Velocity;
	float PosX;
	float PosY;
};
