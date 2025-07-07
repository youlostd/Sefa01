#include "stdafx.h"
#include "config.h"
#include "xmas_event.h"
#include "desc.h"
#include "desc_manager.h"
#include "sectree_manager.h"
#include "char.h"
#include "char_manager.h"
#include "questmanager.h"

namespace xmas
{
	void ProcessEventFlag(const std::string& name, int prev_value, int value)
	{
		if (name == "xmas_snow" || name == "xmas_song" || name == "xmas_tree")
		{
			// 뿌려준다
			const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();

			for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
			{
				LPCHARACTER ch = (*it)->GetCharacter();

				if (!ch)
					continue;

				ch->ChatPacket(CHAT_TYPE_COMMAND, "%s %d", name.c_str(), value);
			}
			if (name == "xmas_tree")
			{
				if (value > 0 && prev_value == 0)
				{
					CharacterVectorInteractor i;

					// 없으면 만들어준다
					if (!CHARACTER_MANAGER::instance().GetCharactersByRaceNum(MOB_XMAS_TREE_VNUM, i))
						CHARACTER_MANAGER::instance().SpawnMob(MOB_XMAS_TREE_VNUM, 61, 76500 + 358400, 60900 + 153600, 0, false, -1);
				}
				else if (prev_value > 0 && value == 0)
				{
					// 있으면 지워준다
					CharacterVectorInteractor i;

					if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(MOB_XMAS_TREE_VNUM, i))
					{
						CharacterVectorInteractor::iterator it = i.begin();

						while (it != i.end())
							M2_DESTROY_CHARACTER(*it++);
					}
				}
			}
		}
	}
}