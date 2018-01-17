/*
 * CUnitStateTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "mock/mock_BonusBearer.h"
#include "mock/mock_UnitInfo.h"
#include "mock/mock_UnitEnvironment.h"
#include "../../lib/battle/CUnitState.h"
#include "../../lib/CCreatureHandler.h"

static const int32_t DEFAULT_HP = 123;
static const int32_t DEFAULT_AMOUNT = 100;
static const int32_t DEFAULT_SPEED = 10;
static const BattleHex DEFAULT_POSITION = BattleHex(5, 5);
static const int DEFAULT_ATTACK = 58;
static const int DEFAULT_DEFENCE = 63;

class UnitStateTest : public testing::Test
{
public:
	UnitInfoMock infoMock;
	UnitEnvironmentMock envMock;
	BonusBearerMock bonusMock;

	const CCreature * pikeman;

	battle::CUnitStateDetached subject;

	bool hasAmmoCart;

	UnitStateTest()
		:infoMock(),
		envMock(),
		bonusMock(),
		subject(&infoMock, &bonusMock),
		hasAmmoCart(false)
	{
		pikeman = CreatureID(0).toCreature();
	}

	void setDefaultExpectations()
	{
		using namespace testing;
		bonusMock.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::STACKS_SPEED, Bonus::CREATURE_ABILITY, DEFAULT_SPEED, 0));

		bonusMock.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::CREATURE_ABILITY, DEFAULT_ATTACK, 0, PrimarySkill::ATTACK));
		bonusMock.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::CREATURE_ABILITY, DEFAULT_DEFENCE, 0, PrimarySkill::DEFENSE));

		bonusMock.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::STACK_HEALTH, Bonus::CREATURE_ABILITY, DEFAULT_HP, 0));

		EXPECT_CALL(infoMock, unitBaseAmount()).WillRepeatedly(Return(DEFAULT_AMOUNT));
		EXPECT_CALL(infoMock, unitType()).WillRepeatedly(Return(pikeman));

		EXPECT_CALL(envMock, unitHasAmmoCart(_)).WillRepeatedly(Return(hasAmmoCart));
	}

	void makeShooter(int32_t ammo)
	{
		bonusMock.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::SHOOTER, Bonus::CREATURE_ABILITY, 1, 0));
		bonusMock.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::SHOTS, Bonus::CREATURE_ABILITY, ammo, 0));
	}

	void initUnit()
	{
		subject.localInit(&envMock);
		subject.position = DEFAULT_POSITION;
	}
};

TEST_F(UnitStateTest, initialRegular)
{
	setDefaultExpectations();
	initUnit();

	EXPECT_TRUE(subject.alive());
	EXPECT_TRUE(subject.ableToRetaliate());
	EXPECT_FALSE(subject.isGhost());
	EXPECT_FALSE(subject.isDead());
	EXPECT_FALSE(subject.isTurret());
	EXPECT_TRUE(subject.isValidTarget(true));
	EXPECT_TRUE(subject.isValidTarget(false));

	EXPECT_FALSE(subject.isClone());
	EXPECT_FALSE(subject.hasClone());

	EXPECT_FALSE(subject.canCast());
	EXPECT_FALSE(subject.isCaster());
	EXPECT_FALSE(subject.canShoot());
	EXPECT_FALSE(subject.isShooter());

	EXPECT_EQ(subject.getCount(), DEFAULT_AMOUNT);
	EXPECT_EQ(subject.getFirstHPleft(), DEFAULT_HP);
	EXPECT_EQ(subject.getKilled(), 0);
	EXPECT_EQ(subject.getAvailableHealth(), DEFAULT_HP * DEFAULT_AMOUNT);
	EXPECT_EQ(subject.getTotalHealth(), subject.getAvailableHealth());

	EXPECT_EQ(subject.getPosition(), DEFAULT_POSITION);

	EXPECT_EQ(subject.getInitiative(), DEFAULT_SPEED);
	EXPECT_EQ(subject.getInitiative(123456), DEFAULT_SPEED);

	EXPECT_TRUE(subject.canMove());
	EXPECT_TRUE(subject.canMove(123456));
	EXPECT_FALSE(subject.defended());
	EXPECT_FALSE(subject.defended(123456));
	EXPECT_FALSE(subject.moved());
	EXPECT_FALSE(subject.moved(123456));
	EXPECT_TRUE(subject.willMove());
	EXPECT_TRUE(subject.willMove(123456));
	EXPECT_FALSE(subject.waited());
	EXPECT_FALSE(subject.waited(123456));

	EXPECT_EQ(subject.totalAttacks.getMeleeValue(), 1);
	EXPECT_EQ(subject.totalAttacks.getRangedValue(), 1);
}

TEST_F(UnitStateTest, canShoot)
{
	setDefaultExpectations();
	makeShooter(1);
	initUnit();

	EXPECT_FALSE(subject.canCast());
	EXPECT_FALSE(subject.isCaster());
	EXPECT_TRUE(subject.canShoot());
	EXPECT_TRUE(subject.isShooter());

	subject.afterAttack(true, false);

	EXPECT_FALSE(subject.canShoot());
	EXPECT_TRUE(subject.isShooter());
}

TEST_F(UnitStateTest, canShootWithAmmoCart)
{
	hasAmmoCart = true;
	setDefaultExpectations();
	makeShooter(1);
	initUnit();

	EXPECT_FALSE(subject.canCast());
	EXPECT_FALSE(subject.isCaster());
	EXPECT_TRUE(subject.canShoot());
	EXPECT_TRUE(subject.isShooter());

	subject.afterAttack(true, false);

	EXPECT_TRUE(subject.canShoot());
	EXPECT_TRUE(subject.isShooter());
}

TEST_F(UnitStateTest, getAttack)
{
	setDefaultExpectations();

	EXPECT_EQ(subject.getAttack(false), DEFAULT_ATTACK);
	EXPECT_EQ(subject.getAttack(true), DEFAULT_ATTACK);
}

TEST_F(UnitStateTest, getDefence)
{
	setDefaultExpectations();

	EXPECT_EQ(subject.getDefence(false), DEFAULT_DEFENCE);
	EXPECT_EQ(subject.getDefence(true), DEFAULT_DEFENCE);
}

TEST_F(UnitStateTest, attackWithFrenzy)
{
	setDefaultExpectations();

	bonusMock.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::IN_FRENZY, Bonus::SPELL_EFFECT, 50, 0));

	int expectedAttack = DEFAULT_ATTACK + 0.5 * DEFAULT_DEFENCE;

	EXPECT_EQ(subject.getAttack(false), expectedAttack);
	EXPECT_EQ(subject.getAttack(true), expectedAttack);
}

TEST_F(UnitStateTest, defenceWithFrenzy)
{
	setDefaultExpectations();

	bonusMock.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::IN_FRENZY, Bonus::SPELL_EFFECT, 50, 0));

	int expectedDefence = 0;

	EXPECT_EQ(subject.getDefence(false), expectedDefence);
	EXPECT_EQ(subject.getDefence(true), expectedDefence);
}

TEST_F(UnitStateTest, additionalAttack)
{
	setDefaultExpectations();

	{
		auto bonus = std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::ADDITIONAL_ATTACK, Bonus::SPELL_EFFECT, 41, 0);

		bonusMock.addNewBonus(bonus);
	}

	EXPECT_EQ(subject.totalAttacks.getMeleeValue(), 42);
	EXPECT_EQ(subject.totalAttacks.getRangedValue(), 42);
}

TEST_F(UnitStateTest, additionalMeleeAttack)
{
	setDefaultExpectations();

	{
		auto bonus = std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::ADDITIONAL_ATTACK, Bonus::SPELL_EFFECT, 41, 0);
		bonus->effectRange = Bonus::ONLY_MELEE_FIGHT;

		bonusMock.addNewBonus(bonus);
	}

	EXPECT_EQ(subject.totalAttacks.getMeleeValue(), 42);
	EXPECT_EQ(subject.totalAttacks.getRangedValue(), 1);
}

TEST_F(UnitStateTest, additionalRangedAttack)
{
	setDefaultExpectations();

	{
		auto bonus = std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::ADDITIONAL_ATTACK, Bonus::SPELL_EFFECT, 41, 0);
		bonus->effectRange = Bonus::ONLY_DISTANCE_FIGHT;

		bonusMock.addNewBonus(bonus);
	}

	EXPECT_EQ(subject.totalAttacks.getMeleeValue(), 1);
	EXPECT_EQ(subject.totalAttacks.getRangedValue(), 42);
}
