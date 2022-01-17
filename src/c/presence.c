#include "gowhatsapp.h"
#include "constants.h"

void
gowhatsapp_handle_presence(PurpleAccount *account, char *remoteJid, char online, time_t last_seen) {
    char *status = GOWHATSAPP_STATUS_STR_ONLINE;
    if (online == 0) {
        if (purple_account_get_bool(account, GOWHATSAPP_FAKE_ONLINE_OPTION, TRUE)) {
            status = GOWHATSAPP_STATUS_STR_AWAY;
        } else {
            status = GOWHATSAPP_STATUS_STR_OFFLINE;
        }
    }
    purple_prpl_got_user_status(account, remoteJid, status, NULL);
    
    if (last_seen != 0) {
        PurpleBuddy *buddy = purple_blist_find_buddy(account, remoteJid);
        if (buddy){
            // TODO: narrowing time_t to int – year 2038 problem incoming
            // last_seen is a well known int in purple – cannot convert to string here
            purple_blist_node_set_int(&buddy->node, "last_seen", last_seen);
        }
    }
}

void
gowhatsapp_handle_profile_picture(gowhatsapp_message_t *gwamsg)
{
    purple_buddy_icons_set_for_user(gwamsg->account, gwamsg->remoteJid, gwamsg->blob, gwamsg->blobsize, NULL);
    PurpleBuddy *buddy = purple_blist_find_buddy(gwamsg->account, gwamsg->remoteJid);
    purple_blist_node_set_string(&buddy->node, "picture_date", gwamsg->text);
    // no g_free(gwamsg->blob) here – purple takes ownership
}

void 
gowhatsapp_tooltip_text(PurpleBuddy *buddy, PurpleNotifyUserInfo *info, gboolean full)
{
    int last_seen = purple_blist_node_get_int(&buddy->node, "last_seen");
    if (last_seen != 0) {
        time_t t = last_seen;
        const size_t bufsize = 100;
        char buf[bufsize];
        strftime(buf, bufsize, "%c", gmtime(&t));
        purple_notify_user_info_add_pair(info, "Last seen", buf);
    }
    const char *picture_date = purple_blist_node_get_string(&buddy->node, "picture_date");
    if (picture_date != NULL) {
        purple_notify_user_info_add_pair(info, "Picture date", picture_date);
    }
}