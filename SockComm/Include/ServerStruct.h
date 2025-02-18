#pragma once

#pragma pack(1)
struct LoginData
{
	int		IdLength;
	char	Id[64];
	int		PwLength;
	char	Password[256];

	/*LoginData(int _IdLength, char* _Id, int _PwLength, char* _Pw) :
		IdLength(_IdLength),
		PwLength(_PwLength)
	{
		memset(Id, 0, IdLength);
		memcpy(Id, _Id, IdLength);

		memset(Password, 0, PwLength);
		memcpy(Password, _Pw, PwLength);
	}*/
};

struct RegisterData
{
	int		IdLength;
	char	Id[64];
	int		PwLength;
	char	Password[256];

	/*RegisterData(int _IdLength, char* _Id, int _PwLength, char* _Pw) :
		IdLength(_IdLength),
		PwLength(_PwLength)
	{
		memset(Id, 0, IdLength);
		memcpy(Id, _Id, IdLength);

		memset(Password, 0, PwLength);
		memcpy(Password, _Pw, PwLength);
	}*/
};

struct CharacterMoveData
{
	LARGE_INTEGER Time;
	UINT	ObjectID;
	UINT8	Dir;
	float	PosX;
	float	PosY;
	bool	IsEnd;

	CharacterMoveData() {};
	CharacterMoveData(LARGE_INTEGER _Time, UINT _ObjID, UINT8 _Dir, float _PosX, float _PosY, bool _IsEnd) :
		Time(_Time),
		ObjectID(_ObjID),
		Dir(_Dir),
		PosX(_PosX),
		PosY(_PosY),
		IsEnd(_IsEnd)
	{}
};

struct SpawnCharacterData
{
	UINT	ObjectID;
	UINT	CharacterType;
	bool	IsLocal;
	float	PosX;
	float	PosY;
	UINT8	ViewDir;

	SpawnCharacterData() {};
	SpawnCharacterData(UINT _ObjectID, UINT _CharacterType, bool _IsLocal, float _PosX, float _PosY, UINT8 _ViewDir) :
		ObjectID(_ObjectID),
		CharacterType(_CharacterType),
		IsLocal(_IsLocal),
		PosX(_PosX),
		PosY(_PosY),
		ViewDir(_ViewDir)
	{}
};

#pragma pack()

