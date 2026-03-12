#include "AssetStore.h"

#include "../Logger/Logger.h"

#include <SDL2/SDL_image.h>

AssetStore::AssetStore() {
    Logger::Log("AssetStore constructor called!");
}

AssetStore::~AssetStore() {
    ClearAssets();
    Logger::Log("AssetStore destructor called!");
}

void AssetStore::ClearAssets() {
    for (auto texture : textures) {
        SDL_DestroyTexture(texture.second);
    }
    textures.clear();

    for (auto font : fonts) {
        TTF_CloseFont(font.second);
    }
    fonts.clear();
}

void AssetStore::AddTexture(SDL_Renderer *renderer, const std::string &assetId, const std::string &filePath) {
    SDL_Surface *surface = IMG_Load(filePath.c_str());
    if (!surface) {
        Logger::Error("Failed to load texture: " + filePath);
        return;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        Logger::Error("Failed to create texture from surface: " + filePath);
        return;
    }

    textures.emplace(assetId, texture);
    Logger::Log("New texture added to asset store with asset id: " + assetId);
}

SDL_Texture *AssetStore::GetTexture(const std::string &assetId) const {
    return textures.at(assetId);
}

void AssetStore::AddFont(const std::string &assetId, const std::string &filePath, int fontSize) {
    TTF_Font *font = TTF_OpenFont(filePath.c_str(), fontSize);
    if (!font) {
        Logger::Error("Failed to load font: " + filePath);
        return;
    }
    fonts.emplace(assetId, font);
}

TTF_Font *AssetStore::GetFont(const std::string &assetId) const {
    return fonts.at(assetId);
}
