#ifndef DOWNWELL_CORE_SHOP_FEEDBACK_H
#define DOWNWELL_CORE_SHOP_FEEDBACK_H
#include "ShopInteraction.h"
#include <cstdint>
namespace downwell {
// Shop Step's child mutations and presentation requests, not the keeper's
// subsequent Step/animation or audio playback. Resource IDs remain original.
struct ShopFeedback {
    enum class Message : std::uint8_t { none, welcome, hello, hoho, maido, poor,
        question, what, huh, thank };
    enum class Channel : std::uint8_t { merchant, ui };
    struct Sound { std::uint8_t resource = 0, volume = 0; Channel channel = Channel::merchant; };
    static constexpr unsigned soundCapacity = 8;
    Sound sounds[soundCapacity];
    unsigned soundCount = 0;
    bool overflow = false;
    Message message = Message::none;
    unsigned thankful = 0, goodbye = 0, confused = 0;
    bool resetKeeperImage = false;
    bool storedItem[3] = {true, true, true};
    bool itemPresent[3] = {true, true, true};

    void beginStep() {
        soundCount = 0; overflow = false; message = Message::none;
        resetKeeperImage = false;
    }
    void sound(unsigned id, unsigned volume, Channel channel) {
        if (soundCount == soundCapacity) { overflow = true; return; }
        sounds[soundCount++] = {static_cast<std::uint8_t>(id),
            static_cast<std::uint8_t>(volume), channel};
    }
    void say(Message text, unsigned id) {
        message = text; sound(id, 90, Channel::merchant);
    }
    void apply(ShopInteraction::Effect effect, unsigned cursor, const ShopInventory& stock) {
        using E = ShopInteraction::Effect;
        switch (effect) {
        case E::opened: thankful = goodbye = 0; break; // does not clear confused
        case E::welcome: say(Message::welcome, 101); break;
        case E::hello: say(Message::hello, 95); break;
        case E::hoho: say(Message::hoho, 96); break;
        case E::moveRight: case E::moveLeft: sound(109, 90, Channel::ui); break;
        case E::browse: sound(107, 90, Channel::ui); break;
        case E::purchased:
            if (cursor >= 1 && cursor <= 3 && stock.items[cursor - 1].remaining == 0)
                storedItem[cursor - 1] = itemPresent[cursor - 1] = false;
            thankful = 1; resetKeeperImage = true;
            say(Message::maido, 104); sound(105, 50, Channel::ui); break;
        case E::poor: say(Message::poor, 100); confused = 1; resetKeeperImage = true; break;
        case E::question: say(Message::question, 98); confused = 1; resetKeeperImage = true; break;
        case E::what: say(Message::what, 102); confused = 1; resetKeeperImage = true; break;
        case E::huh: say(Message::huh, 97); confused = 1; resetKeeperImage = true; break;
        case E::thank:
            say(Message::thank, 99); goodbye = 1; resetKeeperImage = true; break;
        case E::cancel: sound(108, 90, Channel::ui); break;
        case E::disableMask: break;
        }
    }
};
}
#endif
