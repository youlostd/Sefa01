#pragma once

#include "stdafx.h"

#ifdef ENABLE_RUNE_SYSTEM
#include "packet.h"

#include <google/protobuf/repeated_field.h>
#include "protobuf_data.h"

#ifdef min
#undef min
#endif

class CRuneManager : public singleton<CRuneManager>
{
	public:
		CRuneManager();
		~CRuneManager();

		void	Initialize(const ::google::protobuf::RepeatedPtrField<network::TRuneProtoTable>& table);
		void	Initialize(const ::google::protobuf::RepeatedPtrField<network::TRunePointProtoTable>& table);
		void	Destroy();

		DWORD	FindPointProto(WORD point);

		network::TRuneProtoTable*	GetProto(DWORD dwVnum);
#ifdef ENABLE_LEVEL2_RUNES
		DWORD	GetIDByIndex(BYTE bGroup, BYTE bIndex);
#endif
		void CalculateRuneAffects(LPCHARACTER attacker, LPCHARACTER victim, int &damage, EDamageType damageType);
		void OnKill(LPCHARACTER killer, LPCHARACTER victim);
		void OnMeleeAttack(LPCHARACTER ch);
		void OnSkillUse(LPCHARACTER ch, WORD skillVnum);

	private:
		std::map<DWORD, network::TRuneProtoTable>	m_map_Proto;
		std::map<WORD, DWORD>		m_map_pointProto;
};

struct HealPartyMax
{
	LPCHARACTER except;
	int maxHP, currHP;
	inline void operator()(LPCHARACTER ch)
	{
		if (except != ch)
		{
			maxHP = ch->GetMaxHP();
			currHP = ch->GetHP();
			if (currHP < maxHP && !ch->IsDead())
				ch->PointChange(POINT_HP, maxHP - MAX(0, currHP));
		}
	}
};

struct ShieldParty
{
	LPCHARACTER except;
	inline void operator()(LPCHARACTER ch)
	{
		if (except != ch)
		{
			RuneCharData& chData = ch->GetRuneData();
			chData.shield.magicShield = std::min(chData.shield.magicShield + 500, DWORD(3000));
			if (test_server)
				ch->tchat("you received 500 magic shield (total magic shield: %u)", chData.shield.magicShield);

#ifdef ENABLE_RUNE_AFFECT_ICONS
			except->ChatPacket(CHAT_TYPE_COMMAND, "rune_affect_info %d", chData.shield.magicShield);
#endif
		}
	}
};

inline void CRuneManager::OnKill(LPCHARACTER killer, LPCHARACTER victim)
{
	if (victim->IsPC() && killer->GetPoint(POINT_RUNE_HEAL_ON_KILL))
	{
		RuneCharData& killerData = killer->GetRuneData();
		killerData.killCount = (killerData.killCount + 1) % killer->GetPoint(POINT_RUNE_HEAL_ON_KILL);

#ifdef ENABLE_RUNE_AFFECT_ICONS
		if (killer->GetParty() && (killerData.killCount + 1))
			killer->ChatPacket(CHAT_TYPE_COMMAND, "rune_affect_info %d", killerData.killCount);
#endif
		if (!killerData.killCount && killer->GetParty())
		{
			HealPartyMax f;
			f.except = killer;
			killer->GetParty()->ForEachNearMember(f);
			if (test_server)
				killer->tchat("healing the whole party to max");
		}
	}
	else if (killer->IsPC() && !victim->IsPC() && victim->GetMobRank() >= MOB_RANK_BOSS && victim->GetLevel() >= 90 && killer->GetPoint(POINT_RUNE_HARVEST))
	{
		RuneCharData& killerData = killer->GetRuneData();
		int32_t currHarvest = killer->GetQuestFlag("rune_manager.harvested_souls") + 1;
		killer->SetQuestFlag("rune_manager.harvested_souls", currHarvest);
		killerData.soulsHarvested = MIN(killer->GetPoint(POINT_RUNE_HARVEST), currHarvest);
	
#ifdef ENABLE_RUNE_AFFECT_ICONS
		killer->ChatPacket(CHAT_TYPE_COMMAND, "HarvestSould %d", currHarvest);
#endif
		
		if (test_server)
			killer->tchat("you harvested a boss's soul, current: %d", currHarvest);
	}
}

inline void CRuneManager::OnMeleeAttack(LPCHARACTER ch)
{
	if (ch->IsPC())
	{
		RuneCharData& chData = ch->GetRuneData();
		chData.isNewHit = true;
	}
}

inline void CRuneManager::OnSkillUse(LPCHARACTER ch, WORD skillVnum)
{
	if (ch->IsPC())
	{
		RuneCharData& chData = ch->GetRuneData();
		chData.isNewSkill = true;
		if (skillVnum != SKILL_MUYEONG)
			chData.lastSkillVnum = skillVnum;
	}
}

inline void CRuneManager::CalculateRuneAffects(LPCHARACTER attacker, LPCHARACTER victim, int &damage, EDamageType damageType)
{
	RuneCharData& attackerData = attacker->GetRuneData();
	RuneCharData& victimData = victim->GetRuneData();

	if (attacker->IsPC())
	{
		if (attacker->GetPoint(POINT_RUNE_BONUS_DAMAGE_AFTER_HIT) && (damageType == DAMAGE_TYPE_NORMAL || damageType == DAMAGE_TYPE_NORMAL_RANGE))
		{
			if (attackerData.isNewHit)
			{
				auto currentTime = std::chrono::high_resolution_clock::now();
				long long elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - attackerData.lastHitStart).count();
				if (elapsedTime > 10000)
				{
					attackerData.lastHitStart = currentTime;
					attackerData.normalHitCount = 1;
					if (test_server)
						attacker->tchat("first attack for damage attack after hit starts");
				}
				else if (attackerData.normalHitCount < 9)
				{
					if (!attackerData.normalHitCount)
					{
						attackerData.lastHitStart = currentTime;
						elapsedTime = 0;
					}

					LPITEM currWep = attacker->GetWear(WEAR_WEAPON);
					if (currWep && currWep->GetSubType() == WEAPON_DAGGER)
						attackerData.normalHitCount += 2;
					else
						++attackerData.normalHitCount;

					if (test_server)
						attacker->tchat("new rune attack (total: %u) time elapsed since first hit: %lldms", attackerData.normalHitCount, elapsedTime);
				}
			}

			if (attackerData.normalHitCount >= 9 && attacker->GetTarget() == victim)
			{
				attackerData.normalHitCount = 0;
				damage += damage * attacker->GetPoint(POINT_RUNE_BONUS_DAMAGE_AFTER_HIT) / 100;
				if (test_server)
					attacker->tchat("bonus damage on 9th hit from rune (bonus: %d%%)", attacker->GetPoint(POINT_RUNE_BONUS_DAMAGE_AFTER_HIT));
				
#ifdef ENABLE_RUNE_AFFECT_ICONS
				attacker->ChatPacket(CHAT_TYPE_COMMAND, "rune_affect_info %d", damage);
#endif
			}

			attackerData.isNewHit = false;
		}
		else if (victim->IsPC() && attacker->GetPoint(POINT_RUNE_3RD_ATTACK_BONUS) && (damageType == DAMAGE_TYPE_NORMAL || damageType == DAMAGE_TYPE_NORMAL_RANGE) && attackerData.isNewHit)
		{
			attackerData.isNewHit = false;

			LPITEM currWep = attacker->GetWear(WEAR_WEAPON);
			if (currWep && currWep->GetSubType() == WEAPON_DAGGER)
				attackerData.normalHitCount += 2;
			else
				++attackerData.normalHitCount;

			if (test_server)
				attacker->tchat("current hit count: %d", attackerData.normalHitCount);

			if (attackerData.normalHitCount >= 3)
			{
				attackerData.normalHitCount = 0;
				victim->Damage(attacker, attacker->GetPoint(POINT_RUNE_3RD_ATTACK_BONUS), DAMAGE_TYPE_RUNE);
				if (test_server)
					attacker->tchat("dealing 3rd attack's bonus rune damage: %d", attacker->GetPoint(POINT_RUNE_3RD_ATTACK_BONUS));
			}
		}
		else if (attacker->GetPoint(POINT_RUNE_FIRST_NORMAL_HIT_BONUS) && attackerData.isNewHit && !attacker->IsRuneOnCD(POINT_RUNE_FIRST_NORMAL_HIT_BONUS) &&
			(damageType == DAMAGE_TYPE_NORMAL || damageType == DAMAGE_TYPE_NORMAL_RANGE) && damage > 0)
		{
			int32_t heal_forumla = damage * 20 / 100;
			attacker->SetRuneCD(POINT_RUNE_FIRST_NORMAL_HIT_BONUS, attacker->GetPoint(POINT_RUNE_FIRST_NORMAL_HIT_BONUS));
			attackerData.isNewHit = false;

			attacker->StartRuneEvent(false);
			attacker->PointChange(POINT_HP, heal_forumla);
			if (test_server)
				attacker->tchat("receive first hit rune bonus");

#ifdef ENABLE_RUNE_AFFECT_ICONS
				attacker->ChatPacket(CHAT_TYPE_COMMAND, "rune_affect_info %d", heal_forumla);
#endif
		}
		else if (attacker->GetPoint(POINT_RUNE_MSHIELD_PER_SKILL) && attackerData.isNewSkill && attacker->GetParty() && !attacker->IsRuneOnCD(POINT_RUNE_MSHIELD_PER_SKILL) &&
			(damageType == DAMAGE_TYPE_MAGIC || damageType == DAMAGE_TYPE_RANGE || damageType == DAMAGE_TYPE_MELEE))
		{
			attackerData.isNewSkill = false;
			attacker->SetRuneCD(POINT_RUNE_MSHIELD_PER_SKILL, attacker->GetPoint(POINT_RUNE_MSHIELD_PER_SKILL));

			if (test_server)
				attacker->tchat("shielding my party members with 500 magic shield");

			ShieldParty f;
			f.except = attacker;
			attacker->GetParty()->ForEachNearMember(f);
		}
		else if (victim->IsPC() && attacker->GetPoint(POINT_RUNE_DAMAGE_AFTER_3) && !attacker->IsRuneOnCD(POINT_RUNE_DAMAGE_AFTER_3))
		{
			bool isValidHit = false;
			bool isSkillHit = false;
			switch (damageType)
			{
			case DAMAGE_TYPE_NORMAL:
			case DAMAGE_TYPE_NORMAL_RANGE:
				if (attackerData.isNewHit)
				{
					isValidHit = true;
					attackerData.isNewHit = false;
				}
				break;

			case DAMAGE_TYPE_MELEE:
			case DAMAGE_TYPE_RANGE:
			case DAMAGE_TYPE_MAGIC:
				if (attackerData.isNewSkill)
				{
					isValidHit = true;
					attackerData.isNewSkill = false;
					isSkillHit = true;
				}
				break;
			}

			if (isValidHit)
			{
				auto currentTime = std::chrono::high_resolution_clock::now();
				long long elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - attackerData.lastHitStart).count();

				if (elapsedTime > 3000)
				{
					attackerData.lastHitStart = currentTime;
					attackerData.normalHitCount = 0;
					attackerData.skillHitCount = 0;
					elapsedTime = 0;
				}

				if (attackerData.normalHitCount + attackerData.skillHitCount < 3)
				{
					if (isSkillHit)
						++attackerData.skillHitCount;
					else
					{
						LPITEM currWep = attacker->GetWear(WEAR_WEAPON);
						if (currWep && currWep->GetSubType() == WEAPON_DAGGER)
							attackerData.normalHitCount += 2;
						else
							++attackerData.normalHitCount;
					}
				}

				if (test_server)
					attacker->tchat("new hit %s (total: %u skill + %u normal) elapsed time: %lld", isSkillHit ? "skill" : "attack", attackerData.skillHitCount, attackerData.normalHitCount, elapsedTime);
			}

			if (attackerData.normalHitCount + attackerData.skillHitCount >= 3 && victim == attacker->GetTarget())
			{
				if (test_server)
					attacker->tchat("sending bonus 1k true damage to selected target");

				attackerData.normalHitCount = 0;
				attackerData.skillHitCount = 0;
				attacker->SetRuneCD(POINT_RUNE_DAMAGE_AFTER_3, attacker->GetPoint(POINT_RUNE_DAMAGE_AFTER_3));
				victim->Damage(attacker, 1000, DAMAGE_TYPE_RUNE);
			}
		}
		else if (victim->IsPC() && attacker->GetPoint(POINT_RUNE_SHIELD_PER_HIT) && !attackerData.shield.attackShieldFull && (attackerData.isNewHit || attackerData.isNewSkill))
		{
			DWORD maxShield = attacker->GetPoint(POINT_RUNE_SHIELD_PER_HIT);
			DWORD shieldBonus = 0;
			LPITEM currWep = attacker->GetWear(WEAR_WEAPON);
			switch (damageType)
			{
			case DAMAGE_TYPE_NORMAL:
			case DAMAGE_TYPE_NORMAL_RANGE:
				if (attackerData.isNewHit)
				{
					if (currWep && currWep->GetSubType() == WEAPON_DAGGER)
						shieldBonus = 200;
					else
						shieldBonus = 100;
					attackerData.isNewHit = false;
				}
				break;
			case DAMAGE_TYPE_MAGIC:
			case DAMAGE_TYPE_MELEE:
			case DAMAGE_TYPE_RANGE:
				if (attackerData.isNewSkill)
				{
					shieldBonus = 200;
					attackerData.isNewSkill = false;
				}
				break;
			}


			shieldBonus = std::min(shieldBonus, maxShield - attackerData.shield.shieldHPByAttack);
			if (shieldBonus)
			{
				if (test_server)
					attacker->tchat("receive hit shield bonus (%u) old shield value (not total shield): %u", shieldBonus, attackerData.shield.shieldHPByAttack);

				attackerData.shield.shieldHPByAttack += shieldBonus;
				if (attackerData.shield.shieldHPByAttack >= maxShield)
				{
					attackerData.shield.attackShieldFull = true;
					attackerData.shield.shieldHPTotal += attackerData.shield.shieldHPByAttack;
				}
			}
		}
		else if (victim->IsPC() && attacker->GetPoint(POINT_RUNE_RESET_SKILL) && attackerData.isNewSkill && !attacker->IsRuneOnCD(POINT_RUNE_RESET_SKILL) &&
			(damageType == DAMAGE_TYPE_MAGIC || damageType == DAMAGE_TYPE_RANGE || damageType == DAMAGE_TYPE_MELEE) && attackerData.lastSkillVnum)
		{
#ifdef RUNE_26CHANGE
			attackerData.isNewSkill = false;

			if (attackerData.skillHitCount < 4)
			{
				++attackerData.skillHitCount;
				if (test_server)
					attacker->tchat("increasing skill use count (now: %u)", attackerData.skillHitCount);
			}
			else
			{
				if (test_server)
					attacker->tchat("5th skill used, increasing the damage by %d%%", 10);

				attacker->SetRuneCD(POINT_RUNE_RESET_SKILL, attacker->GetPoint(POINT_RUNE_RESET_SKILL));
				attackerData.lastSkillVnum = 0;
				attackerData.lastHitStart = std::chrono::high_resolution_clock::now();
				attackerData.skillHitCount = 0;
				damage += damage * 0.1;
			}
#else
			attackerData.isNewSkill = false;
			auto currentTime = std::chrono::high_resolution_clock::now();
			long long elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - attackerData.lastHitStart).count();
			if (elapsedTime > 10000)
			{
				attackerData.lastHitStart = currentTime;
				attackerData.skillHitCount = 0;
				if (test_server)
					attacker->tchat("restarting skill counting");
			}

			if (attackerData.skillHitCount < 3)
			{
				++attackerData.skillHitCount;
				if (test_server)
					attacker->tchat("increasing skill use count (now: %u)", attackerData.skillHitCount);
			}
			else
			{
				if (test_server)
					attacker->tchat("4th skill used in time, reseting the last used skill (vnum: %u)", attackerData.lastSkillVnum);

				attacker->ResetSkillTime(attackerData.lastSkillVnum);
				attacker->SetRuneCD(POINT_RUNE_RESET_SKILL, attacker->GetPoint(POINT_RUNE_RESET_SKILL));
				attackerData.lastSkillVnum = 0;
				attackerData.lastHitStart = currentTime;
				attackerData.skillHitCount = 0;
			}
#endif
		}
		else if (victim->IsPC() && attacker->GetPoint(POINT_RUNE_MAGIC_DAMAGE_AFTER_HIT) && !attacker->IsRuneOnCD(POINT_RUNE_MAGIC_DAMAGE_AFTER_HIT))
		{
			bool isValidHit = false;
			bool isSkillHit = false;
			switch (damageType)
			{
			case DAMAGE_TYPE_NORMAL:
			case DAMAGE_TYPE_NORMAL_RANGE:
				if (attackerData.isNewHit)
				{
					isValidHit = true;
					attackerData.isNewHit = false;
				}
				break;

			case DAMAGE_TYPE_MELEE:
			case DAMAGE_TYPE_RANGE:
			case DAMAGE_TYPE_MAGIC:
				if (attackerData.isNewSkill)
				{
					isValidHit = true;
					attackerData.isNewSkill = false;
					isSkillHit = true;
				}
				break;
			}

			if (isValidHit)
			{
				if (attackerData.normalHitCount + attackerData.skillHitCount < 4)
				{
					if (isSkillHit)
						++attackerData.skillHitCount;
					else
					{
						LPITEM currWep = attacker->GetWear(WEAR_WEAPON);
						if (currWep && currWep->GetSubType() == WEAPON_DAGGER)
							attackerData.normalHitCount += 2;
						else
							++attackerData.normalHitCount;
					}
				}

				if (test_server)
					attacker->tchat("new hit %s (total: %u skill + %u normal)", isSkillHit ? "skill" : "attack", attackerData.skillHitCount, attackerData.normalHitCount);
			}

			if (attackerData.normalHitCount + attackerData.skillHitCount >= 4 && victim == attacker->GetTarget())
			{
				int baseHP = attacker->GetRealPoint(POINT_MAX_HP);
				int bonusDamage = baseHP * 4 / 100;

				if (test_server)
					attacker->tchat("sending bonus %d magic damage to selected target (base hp: %d)", bonusDamage, baseHP);

#ifdef ENABLE_RUNE_AFFECT_ICONS
				attacker->ChatPacket(CHAT_TYPE_COMMAND, "rune_affect_info %d", bonusDamage);
#endif
				attackerData.normalHitCount = 0;
				attackerData.skillHitCount = 0;
				attacker->SetRuneCD(POINT_RUNE_MAGIC_DAMAGE_AFTER_HIT, attacker->GetPoint(POINT_RUNE_MAGIC_DAMAGE_AFTER_HIT));
				victim->Damage(attacker, bonusDamage, DAMAGE_TYPE_MAGIC);
			}
		}
		else if (victim->IsPC() && attacker->GetPoint(POINT_RUNE_MOVSPEED_AFTER_3) && !attacker->IsRuneOnCD(POINT_RUNE_MOVSPEED_AFTER_3))
		{
			bool isValidHit = false;
			bool isSkillHit = false;
			switch (damageType)
			{
			case DAMAGE_TYPE_NORMAL:
			case DAMAGE_TYPE_NORMAL_RANGE:
				if (attackerData.isNewHit)
				{
					isValidHit = true;
					attackerData.isNewHit = false;
				}
				break;

			case DAMAGE_TYPE_MELEE:
			case DAMAGE_TYPE_RANGE:
			case DAMAGE_TYPE_MAGIC:
				if (attackerData.isNewSkill)
				{
					isValidHit = true;
					attackerData.isNewSkill = false;
					isSkillHit = true;
				}
				break;
			}

			if (isValidHit)
			{
				auto currentTime = std::chrono::high_resolution_clock::now();
				long long elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - attackerData.lastHitStart).count();

				if (elapsedTime > 3000)
				{
					attackerData.lastHitStart = currentTime;
					attackerData.normalHitCount = 0;
					attackerData.skillHitCount = 0;
					elapsedTime = 0;
				}

				if (attackerData.normalHitCount + attackerData.skillHitCount < 3)
				{
					if (isSkillHit)
						++attackerData.skillHitCount;
					else
					{
						LPITEM currWep = attacker->GetWear(WEAR_WEAPON);
						if (currWep && currWep->GetSubType() == WEAPON_DAGGER)
							attackerData.normalHitCount += 2;
						else
							++attackerData.normalHitCount;
					}
				}

				if (test_server)
					attacker->tchat("new hit %s (total: %u skill + %u normal) elapsed time: %lld", isSkillHit ? "skill" : "attack", attackerData.skillHitCount, attackerData.normalHitCount, elapsedTime);

				if (attackerData.normalHitCount + attackerData.skillHitCount >= 3)
				{
					attacker->StartRuneEvent(false);
					attacker->SetRuneCD(POINT_RUNE_MOVSPEED_AFTER_3, 1500);
					if (test_server)
						attacker->tchat("adding bonus movspeed for 1.5sec");
				}
			}
		}
		else if (victim->IsPC() && attacker->GetPoint(POINT_RUNE_SLOW_ON_ATTACK) && attackerData.isNewHit && !attacker->IsRuneOnCD(POINT_RUNE_SLOW_ON_ATTACK) &&
			(damageType == DAMAGE_TYPE_NORMAL || damageType == DAMAGE_TYPE_NORMAL_RANGE) && damage > 0 &&
			!victim->FindAffect(AFFECT_RUNE_ATTACK_SLOW) && !victim->FindAffect(AFFECT_RUNE_MOVEMENT_SLOW))
		{
			attacker->SetRuneCD(POINT_RUNE_SLOW_ON_ATTACK, attacker->GetPoint(POINT_RUNE_SLOW_ON_ATTACK));
			attackerData.isNewHit = false;

			victim->AddAffect(AFFECT_RUNE_ATTACK_SLOW, POINT_RUNE_ATTACK_SLOW_PCT, 10, AFF_NONE, 2, 0, true);
			victim->AddAffect(AFFECT_RUNE_MOVEMENT_SLOW, POINT_RUNE_MOVEMENT_SLOW_PCT, 10, AFF_NONE, 2, 0, true);
			if (test_server)
				attacker->tchat("slowing opponent");
		}
		
		// not "else if" because probably it wouldnt work if we use main runes ...
		if(victim->IsPC() && attacker->GetPoint(POINT_RUNE_MOUNT_PARALYZE) && ( 
				damageType == DAMAGE_TYPE_NORMAL || 
				damageType == DAMAGE_TYPE_NORMAL_RANGE  ||
				damageType == DAMAGE_TYPE_MELEE ||
				damageType == DAMAGE_TYPE_RANGE ||
				damageType == DAMAGE_TYPE_MAGIC
			) && damage > 0)
		{
			victim->AddAffect(AFFECT_RUNE_MOUNT_PARALYZE, POINT_NONE, 0, AFF_MOUNT_PARALYZE, 5, 0, true);
		}
	}

	if (damage > 0)
	{
		if (victim->GetPoint(POINT_RUNE_OUT_OF_COMBAT_SPEED) && victimData.bonusMowSpeed)
			victim->StartRuneEvent(false);

		if (attacker->GetPoint(POINT_RUNE_OUT_OF_COMBAT_SPEED) && attackerData.bonusMowSpeed)
			attacker->StartRuneEvent(false);

		if (attacker->GetPoint(POINT_RUNE_COMBAT_CASTING_SPEED) || victim->GetPoint(POINT_RUNE_COMBAT_CASTING_SPEED))
		{
			std::vector<LPCHARACTER> checklist;
			if (victim->IsPC() && attacker->GetPoint(POINT_RUNE_COMBAT_CASTING_SPEED) && !attacker->IsRuneOnCD(POINT_RUNE_COMBAT_CASTING_SPEED))
				checklist.push_back(attacker);
			if (attacker->IsPC() && victim->GetPoint(POINT_RUNE_COMBAT_CASTING_SPEED) && !victim->IsRuneOnCD(POINT_RUNE_COMBAT_CASTING_SPEED))
				checklist.push_back(victim);

			if (!checklist.empty())
			{
				auto currentTime = std::chrono::high_resolution_clock::now();
				for (itertype(checklist) it = checklist.begin(); it != checklist.end(); ++it)
				{
					LPCHARACTER curr = *it;
					RuneCharData& currData = curr->GetRuneData();

					long long elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - currData.lastHitStart).count();
					long long timeSinceLastHit = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - currData.timeSinceLastHit).count();

					if (timeSinceLastHit > 4000)
					{
						if (test_server)
							curr->tchat("restarting combat timer cus last hit was %lldms before", timeSinceLastHit);
						currData.lastHitStart = currentTime;
						currData.timeSinceLastHit = currentTime;
					}
					else if (elapsedTime >= 10000)
					{
						curr->SetRuneCD(POINT_RUNE_COMBAT_CASTING_SPEED, 60000);
						curr->StartRuneEvent(false);
						if (test_server)
							curr->tchat("giving casting speed bonus after 10sec in combat (%lldms since combat start)", elapsedTime);
					}
					else if (test_server)
						curr->tchat("in combat for %lldms (time since last hit: %lld)", elapsedTime, timeSinceLastHit);
				}
			}
		}
	}

	// handle magic shield
	if (DAMAGE_TYPE_MAGIC == damageType && victimData.shield.magicShield && damage > 0)
	{
		DWORD damageToShield = std::min(victimData.shield.magicShield, (DWORD)damage);
		if (test_server)
		{
			attacker->tchat("victim's magic shield absorbed %u magic damage", damageToShield);
			victim->tchat("your magic shield absorbed %u magic damage (remaining magic shield: %u)", damageToShield, victimData.shield.magicShield - damageToShield);
		}
		victimData.shield.magicShield -= damageToShield;
		damage -= damageToShield;
	}

	// handle shield damage
	if (victimData.shield.shieldHPTotal && damage > 0)
	{
		DWORD damageToShield = std::min(victimData.shield.shieldHPTotal, (DWORD)damage);
		if (test_server)
		{
			attacker->tchat("victim's shield absorbed %u damage", damageToShield);
			victim->tchat("your shield absorbed %u damage (remaining shield: %u)", damageToShield, victimData.shield.shieldHPTotal - damageToShield);
		}
		victimData.shield.shieldHPTotal -= damageToShield;
		damage -= damageToShield;

		if (victimData.shield.shieldHPByAttack && victimData.shield.attackShieldFull)
		{
			victimData.shield.shieldHPByAttack -= std::min(victimData.shield.shieldHPByAttack, damageToShield);
			if (!victimData.shield.shieldHPByAttack)
				victimData.shield.attackShieldFull = false;
		}
	}
}
#endif
