#ifndef EVENTS_H
#define EVENTS_H

#include "../ECS/ECS.h"
#include "../Logger/Logger.h"
#include "Event.h"

#include <SDL2/SDL.h>

class CollisionEvent : public Event {
    public:
        Entity a;
        Entity b;

        CollisionEvent() = default;

        CollisionEvent(Entity a, Entity b) {
            this->a = a;
            this->b = b;
        }
};

class KeyPressedEvent : public Event {
    public:
        SDL_Keycode symbol;

        KeyPressedEvent(SDL_Keycode symbol) {
            this->symbol = symbol;
        }
};

#endif
