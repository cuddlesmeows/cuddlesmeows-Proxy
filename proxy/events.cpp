#include "events.h"
#include "gt.hpp"
#include "proton/hash.hpp"
#include "proton/rtparam.hpp"
#include "proton/variant.hpp"
#include "dialog.h"
#include "server.h"
#include "utils.h"
#include <thread>
#include <limits.h>
#include "HTTPRequest.hpp"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
using namespace events::out;
using namespace std;
bool events::out::variantlist(gameupdatepacket_t* packet) {
    variantlist_t varlist{};
    varlist.serialize_from_mem(utils::get_extended(packet));
    PRINTS("varlist: %s\n", varlist.print().c_str());
    return false;
}
std::vector<std::string> split(const std::string& str, const std::string& delim)
{
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;
    do
    {
        pos = str.find(delim, prev);
        if (pos == std::string::npos) pos = str.length();
        std::string token = str.substr(prev, pos - prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    } while (pos < str.length() && prev < str.length());
    return tokens;
}
bool events::out::pingreply(gameupdatepacket_t* packet) {
    //since this is a pointer we do not need to copy memory manually again
    packet->m_vec2_x = 1000.f;  //gravity
    packet->m_vec2_y = 250.f;   //move speed
    packet->m_vec_x = 64.f;     //punch range
    packet->m_vec_y = 64.f;     //build range
    packet->m_jump_amount = 0;  //for example unlim jumps set it to high which causes ban
    packet->m_player_flags = 0; //effect flags. good to have as 0 if using mod noclip, or etc.
    PRINTC("Sent A Ping Reply!\n");
    return false;
}

bool find_command(std::string chat, std::string name) {
    bool found = chat.find("/" + name) == 0;
    if (found)
        gt::send_log("`1" + chat);
    return found;
}

bool fastdrop = false;
bool fasttrash = false;

std::string mode = "pull";
bool events::out::generictext(std::string packet) {
    auto& world = g_server->m_world;
    rtvar var = rtvar::parse(packet);
    if (!var.valid())
        return false;
    if (var.get(0).m_key == "action" && var.get(0).m_value == "input") {
        if (var.size() < 2)
            return false;
        if (var.get(1).m_values.size() < 2)
            return false;

        if (!world.connected)
            return false;

        if (packet.find("roulette2|") != -1) {
            try {
                std::string aaa = packet.substr(packet.find("te2|") + 4, packet.size());
                std::string number = aaa.c_str();
            }
            catch (exception a) { gt::send_log("`4Critical Error: `2override detected"); }
        }

        if (packet.find("fastdrop|") != -1) {
            try {
                std::string aaa = packet.substr(packet.find("rop|") + 4, packet.size());
                std::string number = aaa.c_str();
                fastdrop = stoi(number);
            }
            catch (exception a) { gt::send_log("`4Critical Error: `2override detected"); }
        }


        
        if (packet.find("buttonClicked|wfloat") != -1) {

            std::string droppedshit = "add_spacer|small|\nadd_label_with_icon_button_list|small|`w%s : %s|left|findObject_|itemID_itemAmount|";

            int itemid;
            int count = 0;
            std::vector<std::pair<int, int>> ditems;
            std::string itemlist = "";
            auto& objMap = g_server->m_world;

            if (objMap.connected) {
                std::unordered_map<uint32_t, DroppedItem> updatedObjects;

                for (auto& object : objMap.objects) {
                    bool found = false;
                    for (int i = 0; i < ditems.size(); i++) {
                        if (ditems[i].first == object.second.itemID) {
                            ditems[i].second += object.second.count;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        ditems.push_back(std::make_pair(object.second.itemID, object.second.count));
                    }
                    else {
                        updatedObjects[object.first] = object.second;
                    }
                }
                objMap.objects = updatedObjects;

                if (ditems.empty()) {
                    gt::send_log("No dropped items found.");
                    return false;
                }

                for (int i = 0; i < ditems.size(); i++) {
                    itemlist += std::to_string(ditems[i].first) + "," + std::to_string(ditems[i].second) + ",";
                }
                std::string paket = "set_default_color|`o\nadd_label_with_icon|big|`wFloating Items``|left|6016|\n" + droppedshit + itemlist.substr(0, itemlist.size() - 1) + "\nadd_quick_exit|dropped||Back|\n";
                variantlist_t liste{ "OnDialogRequest" };
                liste[1] = paket;
                g_server->send(true, liste);
                return true;
            }
            else {
                gt::send_log("Not connected to the world.");
                return false;
            }
        }
            
        auto chat = var.get(1).m_values[1];

        if (find_command(chat, "test")) {
            gt::send_log("test succeds");
        }
        else if (find_command(chat, "proxy")) {
            std::string dialog;
            dialog =
                "\nadd_label_with_icon|big|`1raincuddlesmeow proxy|left|32|"
                "\nadd_spacer|small"
                "\nadd_textbox|`1/warp `0- `1 Warps to another world!|left|2480|"
                "\nadd_textbox|`1/fastdrop `0- `1 The command to enable and disable Fast Drop!|left|2480|"
                "\nadd_textbox|`1/name `0- `1 Changes your in-game username!|left|2480|"
                "\nadd_textbox|`1/skin `0- `1 Changes your in-game skin color!|left|2480|"
                "\nadd_quick_exit|"
                "\nadd_button|OK|OK|left|";
            variantlist_t liste{ "OnDialogRequest" };
            liste[1] = dialog;
            g_server->send(true, liste);
            return true;
        }
        else if (find_command(chat, "skin ")) {
            int skin = atoi(chat.substr(6).c_str());
            variantlist_t va{ "OnChangeSkin" };
            va[1] = skin;
            g_server->send(true, va, world.local.netid, -1);
            return true;
        }
            
        else if (find_command(chat, "name ")) {
            std::string name = "``" + chat.substr(6) + "``";
            variantlist_t va{ "OnNameChanged" };
            va[1] = name;
            g_server->send(true, va, world.local.netid, -1);
            gt::send_log("`#name set to: " + name);
            return true;
        }

        else if (find_command(chat, "fastdrop")) {
            fastdrop = !fastdrop;
            std::string stateMessage = fastdrop ? "`1Fast drop is now enabled." : "`1Fast drop is now disabled.";
            gt::send_log(stateMessage);

            return true;
        }

        else if (find_command(chat, "warp ")) {
            g_server->send(false, "action|join_request\nname|" + chat.substr(5), 3);
            gt::send_log("`1Warping Player to `#" + chat.substr(5));
            return true;
        }

        else if (find_command(chat, "growscan")) {
            std::string paket;
            paket =
                "\nset_default_color|`o"
                "\nadd_label_with_icon|big|`9Growscan|left|6016|"
                "\nadd_spacer|small|"
                "\nadd_button|wfloat|`9Floating items``|noflags|0|0|"
                "\nadd_button|wblock|`9Word blocks``|noflags|0|0|"
                "\nadd_spacer|small|"
                "\nadd_quick_exit|";
            variantlist_t liste{ "OnDialogRequest" };
            liste[1] = paket;
            g_server->send(true, liste);
            return true;
        }
        else if (find_command(chat, "wfloat")) {
            std::string droppedshit = "add_spacer|small|\nadd_label_with_icon_button_list|small|`w%s : %s|left|findObject_|itemID_itemAmount|";

            int itemid;
            int count = 0;
            std::vector<std::pair<int, int>> ditems;
            std::string itemlist = "";
            auto& objMap = g_server->m_world;

            if (objMap.connected) {
                std::unordered_map<uint32_t, DroppedItem> updatedObjects;

                for (auto& object : objMap.objects) {
                    bool found = false;
                    for (int i = 0; i < ditems.size(); i++) {
                        if (ditems[i].first == object.second.itemID) {
                            ditems[i].second += object.second.count;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        ditems.push_back(std::make_pair(object.second.itemID, object.second.count));
                    }
                    else {
                        updatedObjects[object.first] = object.second;
                    }
                }
                objMap.objects = updatedObjects;

                if (ditems.empty()) {
                    gt::send_log("No dropped items found.");
                    return false;
                }

                for (int i = 0; i < ditems.size(); i++) {
                    itemlist += std::to_string(ditems[i].first) + "," + std::to_string(ditems[i].second) + ",";
                }
                std::string paket = "set_default_color|`o\nadd_label_with_icon|big|`wFloating Items``|left|6016|\n" + droppedshit + itemlist.substr(0, itemlist.size() - 1) + "\nadd_quick_exit|dropped||Back|\n";
                variantlist_t liste{ "OnDialogRequest" };
                liste[1] = paket;
                g_server->send(true, liste);
                return true;
            }
            else {
                gt::send_log("Not connected to the world.");
                return false;
            }
        }
        return false;
    }

    if (packet.find("game_version|") != -1) {
        rtvar var = rtvar::parse(packet);
        var.set("meta", g_server->meta);
        packet = var.serialize();
        packet += "requestedName|\n";
        packet += "tankIDPass|\n";
        gt::in_game = false;
        PRINTS("Login information is being spoofed...\n");
        g_server->send(false, packet);
        return true;
    }
    return false;
}

bool events::out::gamemessage(std::string packet) {
    PRINTS("Game message: %s\n", packet.c_str());
    if (packet == "action|quit") {
        g_server->quit();
        return true;
    }

    return false;
}

bool events::out::state(gameupdatepacket_t* packet) {
    if (!g_server->m_world.connected)
        return false;

    g_server->m_world.local.pos = vector2_t{ packet->m_vec_x, packet->m_vec_y };

    if (gt::ghost)
        return true;
    return false;
}

bool events::in::variantlist(gameupdatepacket_t* packet) {
    variantlist_t varlist{};
    auto extended = utils::get_extended(packet);
    extended += 4; //since it casts to data size not data but too lazy to fix this
    varlist.serialize_from_mem(extended);
    PRINTC("varlist: %s\n", varlist.print().c_str());
    auto func = varlist[0].get_string();

    //probably subject to change, so not including in switch statement.
    if (func.find("OnSuperMainStartAcceptLogon") != -1)
        gt::in_game = true;

    switch (hs::hash32(func.c_str())) {
    case fnv32("OnRequestWorldSelectMenu"): {
        auto& world = g_server->m_world;
        world.players.clear();
        world.local = {};
        world.connected = false;
        world.name = "EXIT";
    } break;
    case fnv32("OnSendToServer"): g_server->redirect_server(varlist); return true;
    case fnv32("OnConsoleMessage"): {
        varlist[1] = "`1[raincuddlesmeow]`` " + varlist[1].get_string();
        g_server->send(true, varlist);
        return true;
    } break;
    case fnv32("OnDialogRequest"): {
        auto content = varlist[1].get_string();

        if (fastdrop == true) {
            std::string itemid = content.substr(content.find("embed_data|itemID|") + 18, content.length() - content.find("embed_data|itemID|") - 1);
            std::string count = content.substr(content.find("count||") + 7, content.length() - content.find("count||") - 1);
            if (content.find("embed_data|itemID|") != -1) {
                if (content.find("Drop") != -1) {
                    g_server->send(false, "action|dialog_return\ndialog_name|drop_item\nitemID|" + itemid + "|\ncount|" + count);
                    return true;
                }
            }
        }
    } break;
    case fnv32("OnRemove"): {
        auto text = varlist.get(1).get_string();
        if (text.find("netID|") == 0) {
            auto netid = atoi(text.substr(6).c_str());

            if (netid == g_server->m_world.local.netid)
                g_server->m_world.local = {};

            auto& players = g_server->m_world.players;
            for (size_t i = 0; i < players.size(); i++) {
                auto& player = players[i];
                if (player.netid == netid) {
                    players.erase(std::remove(players.begin(), players.end(), player), players.end());
                    break;
                }
            }
        }
    } break;
    case fnv32("OnSpawn"): {
        std::string meme = varlist.get(1).get_string();
        rtvar var = rtvar::parse(meme);
        auto name = var.find("name");
        auto netid = var.find("netID");
        auto onlineid = var.find("onlineID");
        if (name && netid && onlineid) {
            player ply{};
            ply.mod = false;
            ply.invis = false;
            ply.name = name->m_value;
            ply.country = var.get("country");
            auto pos = var.find("posXY");
            if (pos && pos->m_values.size() >= 2) {
                auto x = atoi(pos->m_values[0].c_str());
                auto y = atoi(pos->m_values[1].c_str());
                ply.pos = vector2_t{ float(x), float(y) };
            }
            ply.userid = var.get_int("userID");
            ply.netid = var.get_int("netID");
            if (meme.find("type|local") != -1) {
                //set mod state to 1 (allows infinite zooming, this doesnt ban cuz its only the zoom not the actual long punch)
                var.find("mstate")->m_values[0] = "1";
                g_server->m_world.local = ply;
            }
            g_server->m_world.players.push_back(ply);
            auto str = var.serialize();
            utils::replace(str, "onlineID", "onlineID|");
            varlist[1] = str;
            PRINTC("new: %s\n", varlist.print().c_str());
            g_server->send(true, varlist, -1, -1);
            return true;
        }
    } break;
    }
    return false;
}
bool events::in::generictext(std::string packet) {
    PRINTC("Generic text: %s\n", packet.c_str());
    return false;
}

bool events::in::gamemessage(std::string packet) {
    PRINTC("Game message: %s\n", packet.c_str());
    return false;
}

bool events::in::sendmapdata(gameupdatepacket_t* packet) {
    g_server->m_world = {};
    auto extended = utils::get_extended(packet);
    extended += 4;
    auto data = extended + 6;
    auto name_length = *(short*)data;

    char* name = new char[name_length + 1];
    memcpy(name, data + sizeof(short), name_length);
    char none = '\0';
    memcpy(name + name_length, &none, 1);

    g_server->m_world.name = std::string(name);
    g_server->m_world.connected = true;
    delete[] name;
    printf("world name is %s\n", g_server->m_world.name.c_str());

    return false;
}

bool events::in::state(gameupdatepacket_t* packet) {
    if (!g_server->m_world.connected)
        return false;
    if (packet->m_player_flags == -1)
        return false;
    return false;
}
bool events::in::tracking(std::string packet) {
    std::PRINTC("Tracking packet: %s\n", packet.c_str());
    if (packet.find("eventName|102_PLAYER.AUTHENTICATION") != -1) {
        string wlbalance = packet.substr(packet.find("Worldlock_balance|") + 18, packet.length() - packet.find("Worldlock_balance|") - 1);

        if (wlbalance.find("PLAYER.") != -1) {
            gt::send_log("`1World Lock Balance: `20");
        }
        else {
            gt::send_log("`1World Lock Balance: `2" + wlbalance);
        }
    }
    if (packet.find("eventName|100_MOBILE.START") != -1) {
        rtvar var = rtvar::parse(packet);
        string gem = packet.substr(packet.find("Gems_balance|") + 13, packet.length() - packet.find("Gems_balance|") - 1);
        string level = packet.substr(packet.find("Level|") + 6, packet.length() - packet.find("Level|") - 1);
        string uidd = packet.substr(packet.find("GrowId|") + 7, packet.length() - packet.find("GrowId|") - 1);
        gt::send_log("`1Gems Balance: `2" + gem);
        gt::send_log("`1Account Level: `2" + level);
    }
    PRINTC("Tracking packet: %s\n", packet.c_str());
    return true;
}