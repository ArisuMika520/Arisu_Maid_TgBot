#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <chrono>
#include <random>
#include <cstddef>

#include <tgbot/tgbot.h>

using namespace std;

const string BOT_TOKEN = "YOUR_BOT_TOKEN_HERE"; //替换成你的 TOKEN
const int VERIFICATION_TIMEOUT_MINUTES = 5;

enum class VerificationStatus 
{
    PendingStart,
    AwaitingAnswer
};

struct VerificationSession 
{
    int64_t groupId;
    int64_t groupMessageId;
    int correctAnswer;
    VerificationStatus status;
};

map<int64_t, VerificationSession> verification_sessions;
mutex sessions_mutex;

// --- 主函数 ---
int main() {
    TgBot::Bot bot(BOT_TOKEN);

    // 1. 事件监听器: 处理所有消息
    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
        if (message->newChatMembers.empty()) {
            if (message->chat->type == TgBot::Chat::Type::Private && (message->text.empty() || message->text[0] != '/') && !message->from->isBot) {
                int64_t user_id = message->from->id;
                VerificationSession session;
                bool session_found = false;

                {
                    lock_guard<mutex> lock(sessions_mutex);
                    auto it = verification_sessions.find(user_id);
                    if (it != verification_sessions.end() && it->second.status == VerificationStatus::AwaitingAnswer) {
                        session = it->second;
                        session_found = true;
                    }
                }

                if (session_found) {
                    try {
                        int user_answer = stoi(message->text);
                        if (user_answer == session.correctAnswer) {
                            bot.getApi().sendMessage(message->chat->id, "✅ 回答正确，验证通过！");
                            
                            auto perms_to_grant = make_shared<TgBot::ChatPermissions>();
                            perms_to_grant->canSendMessages = true;
                            perms_to_grant->canSendAudios = true;
                            perms_to_grant->canSendDocuments = true;
                            perms_to_grant->canSendPhotos = true;
                            perms_to_grant->canSendVideos = true;
                            perms_to_grant->canSendVideoNotes = true;
                            perms_to_grant->canSendVoiceNotes = true;
                            perms_to_grant->canSendPolls = true;
                            perms_to_grant->canSendOtherMessages = true;
                            perms_to_grant->canAddWebPagePreviews = true;
                            
                            bot.getApi().restrictChatMember(session.groupId, user_id, perms_to_grant, false, 0);

                            bot.getApi().deleteMessage(session.groupId, session.groupMessageId);
                            {
                                lock_guard<mutex> lock(sessions_mutex);
                                verification_sessions.erase(user_id);
                            }
                            cout << "用户 " << user_id << " 已成功验证。" << endl;
                        } else {
                            bot.getApi().sendMessage(message->chat->id, "❌ 回答错误，请重试。");
                        }
                    } catch (const exception& e) {
                        bot.getApi().sendMessage(message->chat->id, "请输入一个有效的数字。");
                    }
                }
            }
            return;
        }
        
        TgBot::Chat::Ptr chat = message->chat;
        for (const auto& user : message->newChatMembers) {
            if (user->isBot) continue;

            cout << "新用户: " << user->firstName << " (ID: " << user->id << ") 加入群组: " << chat->title << endl;

            try {
                auto perms_to_restrict = make_shared<TgBot::ChatPermissions>();
                bot.getApi().restrictChatMember(chat->id, user->id, perms_to_restrict, false, 0);

            } catch (TgBot::TgException& e) {
                cerr << "禁言失败: " << e.what() << endl;
                return;
            }

            string bot_username = bot.getApi().getMe()->username;
            string url = "https://t.me/" + bot_username + "?start=verify_" + to_string(user->id) + "_" + to_string(chat->id);
            
            auto keyboard = make_shared<TgBot::InlineKeyboardMarkup>();
            vector<TgBot::InlineKeyboardButton::Ptr> row;
            auto button = make_shared<TgBot::InlineKeyboardButton>();
            button->text = "👉 点击这里开始私聊验证";
            button->url = url;
            row.push_back(button);
            keyboard->inlineKeyboard.push_back(row);

            string text = "欢迎 " + user->firstName + "! 🤖\n为防机器人广告，请在 " + to_string(VERIFICATION_TIMEOUT_MINUTES) + " 分钟内点击下方按钮，通过私聊完成算术题验证。";
            TgBot::Message::Ptr sent_message = bot.getApi().sendMessage(chat->id, text, nullptr, nullptr, keyboard, "HTML");
            
            {
                lock_guard<mutex> lock(sessions_mutex);
                verification_sessions[user->id] = {chat->id, sent_message->messageId, 0, VerificationStatus::PendingStart};
            }

            thread([&bot, user_id = user->id, chat_id = chat->id, message_id = sent_message->messageId]() {
                this_thread::sleep_for(chrono::minutes(VERIFICATION_TIMEOUT_MINUTES));
                lock_guard<mutex> lock(sessions_mutex);
                auto it = verification_sessions.find(user_id);
                if (it != verification_sessions.end()) {
                    try {
                        cout << "用户 " << user_id << " 超时，正在移出..." << endl;
                        bot.getApi().banChatMember(chat_id, user_id);
                        bot.getApi().deleteMessage(chat_id, message_id);
                    } catch (TgBot::TgException& e) {
                        cerr << "超时移出用户时出错: " << e.what() << endl;
                    }
                    verification_sessions.erase(it);
                }
            }).detach();
        }
    });

    // 2. 事件监听器: 处理 /start 命令
    bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
        if (message->chat->type != TgBot::Chat::Type::Private) return;

        string text = message->text;
        string payload_prefix = "/start verify_";
        if (text.rfind(payload_prefix, 0) == 0) {
            string payload = text.substr(payload_prefix.length());
            try {
                size_t first_underscore = payload.find('_');
                int64_t user_id = stoll(payload.substr(0, first_underscore));
                int64_t group_id = stoll(payload.substr(first_underscore + 1));
                if (message->from->id != user_id) {
                    bot.getApi().sendMessage(message->chat->id, "您不能替他人进行验证。");
                    return;
                }
                lock_guard<mutex> lock(sessions_mutex);
                auto it = verification_sessions.find(user_id);
                if (it != verification_sessions.end() && it->second.groupId == group_id) {
                    random_device rd;
                    mt19937 gen(rd());
                    uniform_int_distribution<> distrib(1, 20);
                    int num1 = distrib(gen); int num2 = distrib(gen);
                    it->second.correctAnswer = num1 + num2;
                    it->second.status = VerificationStatus::AwaitingAnswer;
                    string question = "请计算下面的算术题并发送答案：\n\n**" + to_string(num1) + " + " + to_string(num2) + " = ?**";
                    bot.getApi().sendMessage(message->chat->id, question, nullptr, nullptr, nullptr, "Markdown");
                } else {
                    bot.getApi().sendMessage(message->chat->id, "验证会话已过期或无效，请重新加入群组尝试。");
                }
            } catch (const exception& e) {
                bot.getApi().sendMessage(message->chat->id, "无效的验证链接。");
            }
        } else {
            bot.getApi().sendMessage(message->chat->id, "你好！我是一个群组验证机器人。");
        }
    });

    // --- 启动机器人 ---
    try {
        cout << "正在连接到Telegram服务器..." << endl;
        cout << "使用Token: " << BOT_TOKEN.substr(0, 10) << "..." << endl;
        
        // 测试API连接
        cout << "正在获取机器人信息..." << endl;
        auto me = bot.getApi().getMe();
        cout << "机器人启动成功！" << endl;
        cout << "机器人用户名: " << me->username << endl;
        cout << "机器人ID: " << me->id << endl;
        cout << "机器人名称: " << me->firstName << endl;
        cout << "开始监听消息..." << endl;
        
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            try {
                longPoll.start();
            } catch (TgBot::TgException& e) {
                cerr << "轮询错误: " << e.what() << endl;
                cout << "等待5秒后重试..." << endl;
                this_thread::sleep_for(chrono::seconds(5));
            }
        }
    } catch (TgBot::TgException& e) {
        cerr << "Telegram API错误: " << e.what() << endl;
        return 1;
    } catch (exception& e) {
        cerr << "程序运行时出错: " << e.what() << endl;
        return 1;
    }
    return 0;
}