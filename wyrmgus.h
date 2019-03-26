#ifndef __WYRMGUS_H__
#define __WYRMGUS_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <scene/main/node.h>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCampaign;
class CHairColor;
class CLiteraryText;
class CPlayer;
class CPlayerColor;
class CSkinColor;
class oamlGodotModule;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class Wyrmgus : public Node
{
	GDCLASS(Wyrmgus, Node)
	
public:
	static Wyrmgus *GetInstance();

private:
	static Wyrmgus *Instance;

public:
	void Run();
	String GetVersion() const;
	void LuaCommand(String command);
	
	void SetOamlModule(Node *oaml_module);
	
	oamlGodotModule *GetOamlModule() const
	{
		return this->OamlModule;
	}
	
	CHairColor *GetHairColor(String ident) const;
	CPlayerColor *GetPlayerColor(String ident) const;
	CSkinColor *GetSkinColor(String ident) const;
	
	CCampaign *GetCampaign(String ident) const;
	Array GetCampaigns() const;
	void SetCurrentCampaign(String campaign_ident);
	CCampaign *GetCurrentCampaign() const;
	
	Array GetAchievements() const;
	
	Array GetUnitUnitTypes() const;
	Array GetBuildingUnitTypes() const;
	Array GetItemUnitTypes() const;
	
	CPlayer *GetThisPlayer() const;
	
	CLiteraryText *GetLiteraryText(String ident) const;
	Array GetLiteraryTexts() const;
	
	String NumberToRomanNumeral(unsigned number) const;

private:
	oamlGodotModule *OamlModule = nullptr;
	
protected:
	static void _bind_methods();
};

#endif
