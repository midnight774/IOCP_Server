#pragma once

enum class Packet_Type : UINT8
{
	Login = 0,
	Logout,
	Register,
	Spawn,
	Despawn,
	CharacterMove,
	CharacterAttack,
	AttackBox,
	Endpoint
};