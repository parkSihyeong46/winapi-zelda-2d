﻿#include "pch.h"
#include "WorldMapManager.h"
#include "InteractionManager.h"
#include "MapEdittor.h"
#include "GameManager.h"
#include "ItemManager.h"
#include <fstream>
#include <string>
#include <stdio.h>
#include <time.h>

constexpr const POINT RETOUCH_WOOD_DOOR_POS{ 2,4 };
constexpr const POINT SPAWN_PLAYER_STAGE1_POS{ 12,13 };

InteractionManager* InteractionManager::instance = nullptr;

InteractionManager::InteractionManager()
{
	srand(static_cast<unsigned int>(time(NULL)));
}

InteractionManager::~InteractionManager()
{

}

InteractionManager* InteractionManager::GetInstance()
{
	if (nullptr == instance)
		instance = new InteractionManager();

	return instance;
}

void InteractionManager::ReleaseInstance()
{
	delete instance;
	instance = nullptr;
}

void InteractionManager::ChangeMapData(POINT pos)
{
	WorldMap* worldMap = WorldMapManager::GetInstance()->GetWorldMap();

	// 맵 기준 좌표 이탈 시 무시
	if (!(0 <= pos.x && MAP_MAX_X > pos.x))
		return;
	if (!(0 <= pos.y && MAP_MAX_Y > pos.y))
		return;

	switch (worldMap->GetData(MapEdittorSelectState::OBJECT, pos))
	{
	case TextureName::lever_off:		// 레버 on 상태로 변경
		worldMap->SetData(MapEdittorSelectState::OBJECT, pos, TextureName::lever_on);
		break;
	case TextureName::lever_on:			// 레버 off 상태로 변경
		worldMap->SetData(MapEdittorSelectState::OBJECT, pos, TextureName::lever_off);
		break;
	case TextureName::box_off:			// 상자 on 상태로 변경
		worldMap->SetData(MapEdittorSelectState::OBJECT, pos, TextureName::box_on);
		break;
	default:
		return;
	}
}

void InteractionManager::ActionEvent(const POINT pos)
{
	WorldMap * worldMap = WorldMapManager::GetInstance()->GetWorldMap();

	switch (worldMap->GetData(MapEdittorSelectState::EVENT, pos))
	{
	case Event::OPEN_WOOD_HOUSE_DOOR:			// 오두막 문 열기,
		OpenWoodHouseDoor(pos);
		break;
	case Event::CLOSE_WOOD_HOUSE_DOOR:			// 오두막 문 닫기,
		CloseWoodHouseDoor(pos);
		break;
	case Event::OPEN_BOX:						// 아이템 드랍,
		DropItem(pos);
		break;
	default:
		break;
	}
}

void InteractionManager::OpenWoodHouseDoor(const POINT pos)
{
	for (int y = 0; y < MAP_MAX_Y; y++)
	{
		for (int x = 0; x < MAP_MAX_X; x++)
		{
			if (TextureName::wood_house_close == WorldMapManager::GetInstance()->GetWorldMap()->GetData(MapEdittorSelectState::OBJECT, { x,y }))
			{
				WorldMapManager::GetInstance()->GetWorldMap()->SetData(MapEdittorSelectState::OBJECT, { x,y }, TextureName::wood_house);		// 오두막 문 열기
				WorldMapManager::GetInstance()->GetWorldMap()->SetData(MapEdittorSelectState::EVENT, pos, Event::CLOSE_WOOD_HOUSE_DOOR);		// 오두막 문닫히는 이벤트 등록

				WorldMapManager::GetInstance()->GetWorldMap()->SetData(MapEdittorSelectState::COLLIDER,		// 문 쪽 콜라이더 제거
					{ x + RETOUCH_WOOD_DOOR_POS.x, y + RETOUCH_WOOD_DOOR_POS.y }, 0);

				WorldMapManager::GetInstance()->GetWorldMap()->SetData(MapEdittorSelectState::COLLIDER,		// 문 뒤쪽 콜라이더 생성
					{ x + RETOUCH_WOOD_DOOR_POS.x, y + RETOUCH_WOOD_DOOR_POS.y - 1 }, 1);

				Portal protal;
				protal.pos = { x+ RETOUCH_WOOD_DOOR_POS.x,y + RETOUCH_WOOD_DOOR_POS.y};
				protal.stage = 1;
				protal.spawnPos = { SPAWN_PLAYER_STAGE1_POS.x,SPAWN_PLAYER_STAGE1_POS.y };
				WorldMapManager::GetInstance()->AddProtalData(protal);
				return;
			}
		}
	}
}

void InteractionManager::CloseWoodHouseDoor(const POINT pos)
{
	for (int y = 0; y < MAP_MAX_Y; y++)
	{
		for (int x = 0; x < MAP_MAX_X; x++)
		{
			if (TextureName::wood_house == WorldMapManager::GetInstance()->GetWorldMap()->GetData(MapEdittorSelectState::OBJECT, { x,y }))
			{
				WorldMapManager::GetInstance()->GetWorldMap()->SetData(MapEdittorSelectState::OBJECT, { x,y }, TextureName::wood_house_close);	// 오두막 문 닫기
				WorldMapManager::GetInstance()->GetWorldMap()->SetData(MapEdittorSelectState::EVENT, pos, Event::OPEN_WOOD_HOUSE_DOOR);	// 오두막 문열리는 이벤트 등록

				int count = 0;
				for (const auto& iterator : WorldMapManager::GetInstance()->GetProtalData())
				{
					if (iterator.stage == 1 &&
						iterator.pos.x == x + RETOUCH_WOOD_DOOR_POS.x &&
						iterator.pos.y == y + RETOUCH_WOOD_DOOR_POS.y)
					{
						WorldMapManager::GetInstance()->DeleteProtalData(count);
						return;
					}
					count++;
				} 

				return;
			}
		}
	}
}

void InteractionManager::DropItem(const POINT pos)
{
	// 위 부터 반시계 방향으로 돌아가면서 확인, 상하좌우 검사 한 경우 -> 1사분면,2사분면,3사분면,4사분면 순으로 검사
	POINT tempPos;
	for (int i = 0; i < 8; i++)
	{
		tempPos = pos;
		switch (i)
		{
		case 0:				// 북쪽
			tempPos.y-=1;
			break;
		case 1:				// 서
			tempPos.x -= 1;
			break;
		case 2:				// 남
			tempPos.y += 1;
			break;
		case 3:				// 동
			tempPos.x += 1;
			break;
		case 4:				// 1사분면
			tempPos.x += 1;
			tempPos.y -= 1;
			break;
		case 5:				// 2사분면
			tempPos.x -= 1;
			tempPos.y -= 1;
			break;
		case 6:				// 3사분면
			tempPos.x -= 1;
			tempPos.y += 1;
			break;
		case 7:				// 4사분면
			tempPos.x += 1;
			tempPos.y += 1;
			break;
		}

		if (0 == WorldMapManager::GetInstance()->GetWorldMap()->GetData(MapEdittorSelectState::OBJECT, { tempPos.x,tempPos.y}) &&
			0 == WorldMapManager::GetInstance()->GetWorldMap()->GetData(MapEdittorSelectState::COLLIDER, { tempPos.x,tempPos.y}))
		{
			for (const auto& iterator : (*ItemManager::GetInstance()->GetFieldItem()))
			{
				if (iterator.pos.x == tempPos.x && iterator.pos.y == tempPos.y)
					return;
			}
			// 드랍 이벤트 삭제
			WorldMapManager::GetInstance()->GetWorldMap()->SetData(MapEdittorSelectState::EVENT, pos, Event::NONE);		

			// 아이템 스폰
			ItemManager::GetInstance()->AddFieldItem({ tempPos.x,tempPos.y},
				static_cast<int>(rand() % (ItemManager::GetInstance()->GetItemData()->size()+1)));
			return;
		}
	}
}