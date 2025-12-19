#pragma once

class MaterialData;
struct Textures;

namespace engine
{
    void SetupTextures(const std::shared_ptr<MaterialData>& data, std::vector<Textures>& out);
}