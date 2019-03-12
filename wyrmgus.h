#ifndef __WYRMGUS_H__
#define __WYRMGUS_H__

#include <oamlGodotModule/oamlGodotModule.h>
#include <scene/main/node.h>

class CCampaign;
class CHairColor;
class CPlayer;
class CPlayerColor;
class CSkinColor;
class oamlGodotModule;

class Wyrmgus : public Node
{
	GDCLASS(Wyrmgus, Node)
	
public:
	static Wyrmgus *GetInstance();

private:
	static Wyrmgus *Instance;

public:
	int Run();
	String GetVersion() const;
	void LuaCommand(String command);
	
	void SetOamlModule(Ref<oamlGodotModule> oaml_module)
	{
		this->OamlModule = oaml_module;
	}
	
	Ref<oamlGodotModule> GetOamlModule() const
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

private:
	Ref<oamlGodotModule> OamlModule;
	
protected:
	static void _bind_methods();
};

#endif
