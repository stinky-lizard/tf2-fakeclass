
/*
 * Everything for keeping the skin on the ragdoll after death.
 * Technically works but is underwhelming.
 */

public void OnEntityCreated(int entity, const char[] classname)
{
	if (StrEqual(classname, "tf_ragdoll"))
	{	
		RequestFrame(RagdollMade, entity);
	}
}

void RagdollMade(int entity)
{
	int owner = GetEntProp(entity, Prop_Send, "m_iPlayerIndex");
	
	int iSkinItem = EntRefToEntIndex(g_playersData[owner].rSkinItem);
	if (g_playersData[owner].rSkinItem && iSkinItem != INVALID_ENT_REFERENCE)
	{
		SetVariantString(g_playersData[owner].skinPath);
		AcceptEntityInput(owner, "SetCustomModel");

		CreateTimer(0.1, ResetPlayerModelAfterDeath, owner);

	}
	
}

public Action ResetPlayerModelAfterDeath(Handle timer, int owner)
{
	SetVariantString(g_playersData[owner].animPath);
	AcceptEntityInput(owner, "SetCustomModel");
}
