#ifndef __INC_REFINE_H
#define __INC_REFINE_H

#include "constants.h"

#include <google/protobuf/repeated_field.h>
#include "protobuf_data.h"

enum
{
	BLACKSMITH_MOB = 20016, // 확률 개량
	ALCHEMIST_MOB = 20001, // 100% 개량 성공 

	BLACKSMITH_WEAPON_MOB = 20044,
	BLACKSMITH_ARMOR_MOB = 20045,
	BLACKSMITH_ACCESSORY_MOB = 20046,

	DEVILTOWER_BLACKSMITH_WEAPON_MOB = 20074,
	DEVILTOWER_BLACKSMITH_ARMOR_MOB = 20075,
	DEVILTOWER_BLACKSMITH_ACCESSORY_MOB = 20076,

	BLACKSMITH2_MOB	= 20091,
	BLACKSMITH_STONE_MOB = 20011,

	ITEM_STONE_SCROLL_VNUM = 90105,
};

class CRefineManager : public singleton<CRefineManager>
{
	typedef std::map<DWORD, network::TRefineTable> TRefineRecipeMap;
	public:
	CRefineManager();
	virtual ~CRefineManager();

	bool	Initialize(const ::google::protobuf::RepeatedPtrField<network::TRefineTable>& table);
	const network::TRefineTable* GetRefineRecipe(DWORD id);

	private:
	TRefineRecipeMap	m_map_RefineRecipe;

};
#endif
