#define freakmodels_weapons_included

KeyValues g_playerWeaponData = null;

Action WeaponCommand(int client, int args)
{
    if (args < 1)
    {
        PrintWeaponHelp(client);
        return Plugin_Handled;
    }

    char argstr[5];
    GetCmdArg(1, argstr, sizeof(argstr));

    int rWeapon = EntIndexToEntRef(GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon"));

    if (argstr[0] == 'h' || argstr[0] == 'H')
    {
        ReplyToCommand(client, "Hiding your current active weapon.");
        SetEntityRenderMode(rWeapon, RENDER_TRANSCOLOR);
        SetEntityRenderColor(rWeapon, 0, 0, 0, 0);
    }
    else if (argstr[0] == 'u' || argstr[0] == 'U' || argstr[0] == 's' || argstr[0] == 'S')
    {
        ReplyToCommand(client, "Unhiding your current active weapon.");
        SetEntityRenderMode(rWeapon, RENDER_NORMAL);
        SetEntityRenderColor(rWeapon);
    }
    else if (argstr[0] == 'e' || argstr[0] == 'E')
    {
        if (args < 2)
        {
            ReplyToCommand(client, "Please enter a slot number to use with fm_weapon equip.");
            return Plugin_Handled;
        }

        GetCmdArg(2, argstr, sizeof(argstr));
        int slot = StringToInt(argstr);

        if (slot > 99 || slot < 1)
        {
            ReplyToCommand(client, "Slots only go between 1 and 99; you entered %i.", slot);
            return Plugin_Handled;
        }

        PlayerData pData = PlayerData(client);

        int equip = pData.GetEquippable(slot);

        if (!equip)
        {
            ReplyToCommand(client, "You don't have anything equipped in slot %i. Please equip something first.", slot);
            return Plugin_Handled;
        }
        
        if (!g_playerWeaponData) 
            g_playerWeaponData = new KeyValues("HookedWeapons");

        char userIdStr[MAXINTLENGTH];
        IntToString(pData.userId, userIdStr, sizeof(userIdStr));

        char rWeaponStr[MAXINTLENGTH];
        IntToString(rWeapon, rWeaponStr, sizeof(rWeaponStr));

        if (!g_playerWeaponData.JumpToKey(userIdStr))
        {
            //we don't have any hooked equips for this user
            g_playerWeaponData.JumpToKey(userIdStr, true);
            SDKHook(client, SDKHook_WeaponSwitchPost, SwitchedWeapon);
        }

        ArrayList hookedEquipsForWeapon = view_as<ArrayList>(g_playerWeaponData.GetNum(rWeaponStr));

        if (!hookedEquipsForWeapon)
        {
            //there aren't any equips hooked for this weapon
            hookedEquipsForWeapon = new ArrayList();
            g_playerWeaponData.SetNum(rWeaponStr, view_as<int>(hookedEquipsForWeapon));
        }

        bool weaponAlreadyHooked;
        // int equipIndex;

        if (g_playerWeaponData.GotoFirstSubKey(false))
        {
            do
            {
                ArrayList list = view_as<ArrayList>(g_playerWeaponData.GetNum(NULL_STRING));

                if (list && list.FindValue(equip) > -1)
                {
                    weaponAlreadyHooked = true;
                    list.Erase(list.FindValue(equip)); //we aren't doing anything else with it
                    break;
                } 

            } while (g_playerWeaponData.GotoNextKey(false));
            g_playerWeaponData.GoBack();
        }

        if (weaponAlreadyHooked)
        {
            //unhook it
            MakePlayerVisible(equip);
            // hookedEquipsForWeapon.Erase(equipIndex); // did this earlier
            ReplyToCommand(client, "The model in slot %i has been untied from your weapon.", slot);
        }
        else
        {
            //its not in the list, hook it
            hookedEquipsForWeapon.Push(equip);
            ReplyToCommand(client, "The model in slot %i has been tied to your weapon.", slot);
        }

        if (hookedEquipsForWeapon.Length == 0)
        {
            //they've unhooked all their equips for this weapon
            delete hookedEquipsForWeapon;

            g_playerWeaponData.DeleteKey(rWeaponStr);

            CheckPlayerHookStatus(client);
        }
        g_playerWeaponData.Rewind();
    }

    return Plugin_Handled;
}

void CheckPlayerHookStatus(int client)
{
    char userIdStr[MAXINTLENGTH];
    IntToString(GetClientUserId(client), userIdStr, sizeof(userIdStr));

    g_playerWeaponData.Rewind();
    if (!g_playerWeaponData.JumpToKey(userIdStr))
    {
        //they aren't in the list, we shouldn't be checking them
        SDKUnhook(client, SDKHook_WeaponSwitchPost, SwitchedWeapon);
        return;
    }

    if (!g_playerWeaponData.GotoFirstSubKey(false))
    {
        //they don't have any more hooked weapons
        SDKUnhook(client, SDKHook_WeaponSwitchPost, SwitchedWeapon);
        g_playerWeaponData.DeleteThis();
    }
    else
        //there was a first subkey
        g_playerWeaponData.GoBack();

    g_playerWeaponData.Rewind();
}

void PrintWeaponHelp(int client)
{
    ReplyToCommand(client, "Printing fm_weapon help in your console.");

    PrintToConsole(client, "Use fm_weapon to hide or unhide your weapon.\nThis command is only in its beginning stages at the moment; the hope for the future is to make it much more integrated with the rest of the plugin.\nNevertheless, you can do many things right now using this and fm_equip.\nUsage:\n- Use `fm_weapon hide` to hide your weapon.\n- Use `fm_weapon show` to show your weapon after hiding it.\n- Use `fm_weapon equip <slot>` to tie an equip slot to your current weapon, hiding it when you switch away.");
}

void SwitchedWeapon(int client, int weapon)
{
    LogMessage("SwitchedWeapon %i, %i", client, weapon);
    char[] export = new char[g_playerWeaponData.ExportLength];
    g_playerWeaponData.ExportToString(export, g_playerWeaponData.ExportLength);
    PrintToServer("%s", export);

    bool somethingWasDeleted = false;

    char userIdStr[MAXINTLENGTH];
    IntToString(GetClientUserId(client), userIdStr, sizeof(userIdStr));

    char rWeaponStr[MAXINTLENGTH];
    IntToString(EntIndexToEntRef(weapon), rWeaponStr, sizeof(rWeaponStr));

    if (g_playerWeaponData.JumpToKey(userIdStr))
    {
        if (g_playerWeaponData.GotoFirstSubKey(false))
        {
            char keyToDelete[MAXINTLENGTH];
            bool done;
            
            do
            {
                char sectionName[MAXINTLENGTH];
                g_playerWeaponData.GetSectionName(sectionName, sizeof(sectionName));

                if (StrEqual(rWeaponStr, sectionName))
                {
                    ArrayList hookedEquipsForWeapon = view_as<ArrayList>(g_playerWeaponData.GetNum(NULL_STRING));
                    
                    if (!hookedEquipsForWeapon) continue; //for deleted weapon hook lists

                    for (int i = 0; i < hookedEquipsForWeapon.Length; i++)
                    {
                        int rEquip = hookedEquipsForWeapon.Get(i);
                        if (IsValidEntity(rEquip)) 
                        {
                            MakePlayerVisible(rEquip);
                            //this should override all else
                            done = true;
                        }
                        else
                        {
                            //the equip was deleted somehow. unhook it
                            hookedEquipsForWeapon.Erase(i);
                            i--;
                            if (hookedEquipsForWeapon.Length == 0)
                            {
                                //that was the last equip
                                delete hookedEquipsForWeapon;
                                g_playerWeaponData.SetString(NULL_STRING, "deleted");
                                strcopy(keyToDelete, sizeof(keyToDelete), sectionName);
                                somethingWasDeleted = true;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    ArrayList hookedEquipsForWeapon = view_as<ArrayList>(g_playerWeaponData.GetNum(NULL_STRING));

                    if (!hookedEquipsForWeapon) continue; //for deleted weapon hook lists

                    for (int i = 0; i < hookedEquipsForWeapon.Length; i++)
                    {
                        int rEquip = hookedEquipsForWeapon.Get(i);
                        if (IsValidEntity(rEquip)) 
                        {
                            MakePlayerInvisible(rEquip);
                        }
                        else
                        {
                            //the equip was deleted somehow. unhook it
                            hookedEquipsForWeapon.Erase(i);
                            i--;
                            if (hookedEquipsForWeapon.Length == 0)
                            {
                                //that was the last equip
                                delete hookedEquipsForWeapon;
                                g_playerWeaponData.SetString(NULL_STRING, "deleted");
                                g_playerWeaponData.GetSectionName(keyToDelete, sizeof(keyToDelete));
                                somethingWasDeleted = true;
                                break;
                            }
                        }
                    }
                    
                }
            } while (g_playerWeaponData.GotoNextKey(false) && !done);

            g_playerWeaponData.GoBack();

            if (keyToDelete[0]) 
            {
                g_playerWeaponData.DeleteKey(keyToDelete);
                somethingWasDeleted = true;
            }
        }
        else
        {
            //they don't have any hooked weapons
            g_playerWeaponData.DeleteThis();
            somethingWasDeleted = true;
        }

        if (somethingWasDeleted) CheckPlayerHookStatus(client);

        g_playerWeaponData.Rewind();
    }
}

void UnhookWeapon(int entity)
{
    if (!HasEntProp(entity, Prop_Send, "m_hOwner") || !HasEntProp(entity, Prop_Send, "m_iState")) return; //not a weapon

    if (!g_playerWeaponData) return; //we havent done anything yet

    char rWeaponStr[MAXINTLENGTH];
    IntToString(EntIndexToEntRef(entity), rWeaponStr, sizeof(rWeaponStr));

    //we cant use m_hOwner/m_hOwnerEntity so we have to loop thru the kv & find if its in there
    g_playerWeaponData.Rewind();
    if (g_playerWeaponData.GotoFirstSubKey())
    {
        do
        {
            if (g_playerWeaponData.GetNum(rWeaponStr))
                //there is something & we're in the right place
                break;
        }
        while(g_playerWeaponData.GotoNextKey());
    }

    char userIdStr[MAXINTLENGTH];
    g_playerWeaponData.GetSectionName(userIdStr, sizeof(userIdStr));

    int owner = GetClientOfUserId(StringToInt(userIdStr));

    if (!IsValidClient(owner)) return;

    // if (g_playerWeaponData.JumpToKey(userIdStr))
    // {
    //the weapon owner has at least one hooked weapon
    ArrayList hookedEquipsForWeapon = view_as<ArrayList>(g_playerWeaponData.GetNum(rWeaponStr));

    if (hookedEquipsForWeapon)
    {
        //the deleted weapon is hooked
        //make all the equips visible
        for (int i = 0; i < hookedEquipsForWeapon.Length; i++)
        {
            MakePlayerVisible(hookedEquipsForWeapon.Get(i));
        }

        delete hookedEquipsForWeapon;
        g_playerWeaponData.DeleteKey(rWeaponStr);

        CheckPlayerHookStatus(owner);
    }
    else
    {
        g_playerWeaponData.DeleteKey(rWeaponStr);
    }

    // }

    g_playerWeaponData.Rewind();
}
