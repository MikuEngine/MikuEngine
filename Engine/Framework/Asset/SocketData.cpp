#include "EnginePCH.h"
#include "Framework/Asset/SocketData.h"
#include "Common/Utility/JsonHelper.h"
#include <fstream>

namespace engine
{
    void Socket::UpdateLocalMatrix()
    {
        localMatrix = Matrix::CreateScale(localScale) *
                      Matrix::CreateFromQuaternion(localRotation) *
                      Matrix::CreateTranslation(localPosition);
    }

    void SocketData::Create(const std::string& filePath)
    {
        std::ifstream file(filePath);
        if (!file.is_open()) return;

        nlohmann::ordered_json j;
        file >> j;

        if (j.is_array())
        {
            m_sockets.clear();
            m_socketMap.clear();

            for (const auto& item : j)
            {
                Socket socket;
                JsonGet(item, "name", socket.name);
                JsonGet(item, "parent_bone", socket.parentBoneName, std::string(""));
                JsonGet(item, "position", socket.localPosition, Vector3::Zero);
                JsonGet(item, "rotation", socket.localRotation, Quaternion::Identity);
                JsonGet(item, "scale", socket.localScale, Vector3::One);

                socket.UpdateLocalMatrix();
                m_sockets.push_back(socket);
            }

            for (auto& socket : m_sockets)
            {
                m_socketMap[socket.name] = &socket;
            }
        }
    }

    const Socket* SocketData::GetSocket(const std::string& name) const
    {
        auto it = m_socketMap.find(name);
        return (it != m_socketMap.end()) ? it->second : nullptr;
    }
}
